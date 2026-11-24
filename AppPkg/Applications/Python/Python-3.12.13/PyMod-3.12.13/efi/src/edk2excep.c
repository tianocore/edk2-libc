/** @file
    UEFI exception / guarded memory access.

    Copyright (c) 2011-2026, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    https://opensource.org/license/bsd-2-clause-patent.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <setjmp.h>
#include <signal.h>
#include <string.h>

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>

#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleTextOut.h>
#include <Protocol/Cpu.h>
#include <Protocol/Shell.h>
#include <Protocol/Rng.h>

#include "efi/edk2main.h"
#include "efi/edk2excep.h"
#include "efi/edk2asm.h"

void
py_common_interrupt_entry();

static edk2_seh_context_t g_context[EDK2_SEH_CONTEXT_SIZE];
static int g_context_index = -1;

void *
edk2_seh_try()
{  
  edk2_seh_context_t *entry = NULL;

  /* Undo the increment before bailing out. The caller receives NULL and so will
   * not call edk2_seh_catch(), which is the only thing that decrements. Left
   * incremented, py_handle_exception() would take the recovery branch, index
   * g_context out of bounds, write a whole EFI_SYSTEM_CONTEXT_X64 past the end
   * of the array and longjmp through whatever followed it. */
  if(++g_context_index >= EDK2_SEH_CONTEXT_SIZE) {
    --g_context_index;
    return NULL;
  }

  entry = g_context + g_context_index;
  
  memset((char *)entry, '\0', sizeof(edk2_seh_context_t));

  entry->magic = EDK2_SEH_MAGIC;

  return &entry->ret_context;
}

void
edk2_seh_catch(INTN should_raise, uint64_t *exc_kind_out, uint8_t *buf, size_t bufsize)
{
  edk2_seh_context_t *entry = NULL;

  if(g_context_index < 0) 
    return;
  
  entry = g_context + g_context_index--;

  if(entry && entry->magic != EDK2_SEH_MAGIC)
    return;

  if (should_raise) {
    *exc_kind_out = entry->exc_kind;
    if (buf != NULL && bufsize > 0) {
      size_t src_size = sizeof(EFI_SYSTEM_CONTEXT_X64);
      size_t dst_size = (src_size < bufsize) ? src_size : bufsize;
      memcpy(buf, (uint8_t*)&entry->exc_context, dst_size);          
    }
  }

  memset((char *)entry, '\0', sizeof(edk2_seh_context_t));
}

/* Performs one guarded access. Returns 0 on success, 1 if it faulted (with
 * *kind_out and *ctx_out filled in), or -1 if the guard could not be installed.
 *
 * value is a pointer to volatile deliberately: it is the one object written
 * inside the guarded region and read after a longjmp. SetJump/LongJump restore
 * the callee-saved registers, so a non-volatile local assigned in the region
 * could be rolled back to its pre-setjmp value on the recovery path. Nothing
 * else here is written after setjmp, which is why nothing else needs it. This
 * is the easiest thing in the file to get subtly wrong, and it would fail
 * intermittently rather than outright.
 *
 * Do not split this function. setjmp has to be called by the frame that longjmp
 * returns to, so the setjmp, the access and the return belong in one frame.
 * Factoring out an "arm the guard" helper would compile, read fine, and arm the
 * guard against a frame that had already returned.
 *
 * Lived in posixmodule.c until the edk2 module needed it too; there is nothing
 * Python about it, so it belongs here next to edk2_seh_try/catch rather than
 * being duplicated. Types are spelled unsigned long long rather than uint64_t
 * to match the callers, whose values come from PyArg_ParseTuple's "K": the two
 * spellings need not be the same type under GCC, and a volatile
 * unsigned long long * would then not be assignable to a volatile uint64_t *. */
int
edk2_guarded_access(int is_write, unsigned long long addr, int size,
                    volatile unsigned long long *value,
                    uint64_t *kind_out, EFI_SYSTEM_CONTEXT_X64 *ctx_out)
{
  jmp_buf *jb = (jmp_buf *)edk2_seh_try();
  if (jb == NULL)
    return -1;

  if (setjmp(*jb) == 0) {
    if (is_write) {
      switch (size) {
      case 1: *(volatile uint8_t  *)(uintptr_t)addr = (uint8_t )*value; break;
      case 2: *(volatile uint16_t *)(uintptr_t)addr = (uint16_t)*value; break;
      case 4: *(volatile uint32_t *)(uintptr_t)addr = (uint32_t)*value; break;
      case 8: *(volatile uint64_t *)(uintptr_t)addr = (uint64_t)*value; break;
      }
    }
    else {
      switch (size) {
      case 1: *value = *(volatile uint8_t  *)(uintptr_t)addr; break;
      case 2: *value = *(volatile uint16_t *)(uintptr_t)addr; break;
      case 4: *value = *(volatile uint32_t *)(uintptr_t)addr; break;
      case 8: *value = *(volatile uint64_t *)(uintptr_t)addr; break;
      }
    }
    edk2_seh_catch(0, NULL, NULL, 0);
    return 0;
  }

  /* edk2_seh_catch() dereferences kind_out unconditionally when asked to
   * report, so both out-params are always passed. */
  edk2_seh_catch(1, kind_out, (uint8_t *)ctx_out, sizeof(*ctx_out));
  return 1;
}

int
edk2_guarded_copy(void *dst, const void *src, size_t len,
                  uint64_t *kind_out, EFI_SYSTEM_CONTEXT_X64 *ctx_out)
{
  jmp_buf *jb = (jmp_buf *)edk2_seh_try();
  if (jb == NULL)
    return -1;

  if (setjmp(*jb) == 0) {
    /* One setjmp for the whole block rather than one per byte. The alternative
     * costs a SetJump per byte and buys nothing: a fault anywhere in the range
     * fails the whole call either way, since a partially filled buffer is not
     * something a caller can use.
     *
     * Byte at a time, and volatile, on purpose. memcpy() is free to use wide or
     * vector moves and to reorder, and a caller reaching into MMIO may be
     * talking to a device that cares about access width. Slower than memcpy and
     * that is the right trade here.
     *
     * All three locals are declared inside this block and none is read after
     * the longjmp, so the usual setjmp rule about non-volatile locals with
     * indeterminate values does not bite. Only the parameters are used on the
     * fault path, and nothing in this function writes to them. */
    volatile uint8_t *d = (volatile uint8_t *)dst;
    volatile const uint8_t *s = (volatile const uint8_t *)src;
    size_t i;

    for (i = 0; i < len; i++)
      d[i] = s[i];

    edk2_seh_catch(0, NULL, NULL, 0);
    return 0;
  }

  edk2_seh_catch(1, kind_out, (uint8_t *)ctx_out, sizeof(*ctx_out));
  return 1;
}


VOID
EFIAPI
py_handle_exception (
  EFI_EXCEPTION_TYPE  InterruptType,
  EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  if(g_context_index < 0) {  
    volatile int exc_trap = 1;
    void *p = g_edk2_globals.loaded_image->ImageBase;
    int signum = SIGABRT;
    
    switch(InterruptType) {
      case EXCEPT_X64_PAGE_FAULT:
        signum = SIGSEGV;
        break;
      case EXCEPT_X64_INVALID_OPCODE:
        signum = SIGILL;
        break;
      case EXCEPT_X64_FP_ERROR:
        signum = SIGFPE;
        break;          
    }

    /* Unconditional: this is the only notice anyone gets that the image took a
     * CPU fault, and the spin below never returns, so gating it on a trace
     * macro turns the fault into a silent hang. It was behind
     * PY_UEFI_BOOT_TRACE, which is MSFT-only, so GCC images trapped faults and
     * reported nothing at all. rip locates the fault; cr2 is the faulting
     * address for a page fault and stale otherwise. */
    Print(L"Python312 boot: unhandled CPU exception %d rip=%lx cr2=%lx\n",
          (int)InterruptType,
          (UINT64)SystemContext.SystemContextX64->Rip,
          (UINT64)SystemContext.SystemContextX64->Cr2);

    raise(signum);
    
    while (exc_trap) {
#if defined(_MSC_VER)
      (void)p;
      EDK2_RW_BARRIER();
      EDK2_PAUSE();
#else
      asm("mov %0, %%rax; "
          :
          : "r"(p) /* input */
          : "%rax" /* clobbered register */
      );
      __asm__ __volatile__("pause");
#endif
    }

    return;
  }

  edk2_seh_context_t *entry = g_context + g_context_index;

  entry->exc_kind = InterruptType;
  entry->exc_context = *SystemContext.SystemContextX64;

  /* py_common_interrupt_entry starts with cli, and it is the iretq on the normal
   * return path that puts RFLAGS (including IF) back. The longjmp below never
   * reaches that iretq, so without this interrupts stay masked for the rest of
   * the run and the firmware timer stops firing, which surfaces much later as
   * unrelated-looking console misbehaviour. Conditional on the faulting code's
   * own IF: a fault taken inside a critical section must not resume with
   * interrupts enabled. */
  if(entry->exc_context.Rflags & BIT9)
    EnableInterrupts();

  longjmp(entry->ret_context, 1);
}

//
// Error code flag indicating whether or not an error code will be
// pushed on the stack if an exception occurs.
//
// 1 means an error code will be pushed, otherwise 0
//
CONST UINT32 py_error_code_flag = 0x00027d00;

static uint64_t old_idtr[2] = {0};
static edk2_idt_entry_t *new_idt = NULL;
static uint8_t *new_idt_trampoline = NULL;
static UINTN trampoline_size_pages = 0;

EFI_STATUS
py_install_idt()
{
  EFI_STATUS status;
  uint8_t *old_idt;
  uint64_t old_idt_size = 0;
  int max_seh_exception = 0;
  // clang-format off
  uint8_t trampoline_code[] = {
      0x6a, 0, // push vector_num
      0x50, // push rax
      0x48, 0xb8, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, // movabs rax, jmp_addr
      0xff, 0xe0 // jmp rax
  };
  // clang-format on
  EFI_SYSTEM_TABLE *systab = g_edk2_globals.system_table;
  EFI_BOOT_SERVICES *bootsvc = systab->BootServices;

  const int x64_seh_exceptions[] = {EXCEPT_X64_DIVIDE_ERROR,
                                    EXCEPT_X64_OVERFLOW,
                                    EXCEPT_X64_BOUND,
                                    EXCEPT_X64_INVALID_OPCODE,
                                    EXCEPT_X64_INVALID_TSS,
                                    EXCEPT_X64_SEG_NOT_PRESENT,
                                    EXCEPT_X64_STACK_FAULT,
                                    EXCEPT_X64_GP_FAULT,
                                    EXCEPT_X64_PAGE_FAULT,
                                    EXCEPT_X64_FP_ERROR,
                                    EXCEPT_X64_ALIGNMENT_CHECK,
                                    EXCEPT_X64_MACHINE_CHECK,
                                    EXCEPT_X64_SIMD};
  const int x64_seh_exceptions_size = sizeof(x64_seh_exceptions) / sizeof(int);

  for (int i = 0; i < x64_seh_exceptions_size; i++) {
    max_seh_exception = MAX(max_seh_exception, x64_seh_exceptions[i]);
  }

  status = bootsvc->AllocatePages(AllocateAnyPages,
                                  EfiBootServicesData,
                                  1,
                                  (VOID *)&new_idt);

  if (EFI_ERROR(status)) return status;

  trampoline_size_pages =
     (max_seh_exception * sizeof(trampoline_code) + 4095) / 4096;

  status = bootsvc->AllocatePages(AllocateAnyPages,
                                  EfiBootServicesCode,
                                  trampoline_size_pages,
                                  (VOID *)&new_idt_trampoline);

  if (EFI_ERROR(status)) {
    bootsvc->FreePages((uint64_t)new_idt, 1);
    return status;
  }

  edk2_get_idtr((uint8_t *)old_idtr);

  old_idt = (uint8_t *)old_idtr[1];
  old_idt_size = old_idtr[0] >> 48;

  memcpy((uint8_t *)new_idt, old_idt, MIN(old_idt_size, 4096));

  for (int i = 0; i < x64_seh_exceptions_size; i++) {
    int id = x64_seh_exceptions[i];
    uint8_t *trampoline = new_idt_trampoline + i * sizeof(trampoline_code);
    memcpy(trampoline, trampoline_code, sizeof(trampoline_code));

    trampoline[1] = id;

    *(uint64_t *)(trampoline + 5) = (uint64_t)py_common_interrupt_entry;

    new_idt[id].offset_32_63 = (UINT32)(((uint64_t)trampoline >> 32) & 0xffffffff);
    new_idt[id].offset_16_31 = (UINT16)(((uint64_t)trampoline >> 16) & 0xffff);
    new_idt[id].offset_0_15 = (UINT16)((uint64_t)trampoline & 0xffff);
  }

  uint64_t new_idtr[2] = {old_idtr[0], (uint64_t)new_idt};

  edk2_set_idtr((uint8_t *)new_idtr);

  return status;
}

void
py_restore_idt()
{
  EFI_SYSTEM_TABLE *systab = g_edk2_globals.system_table;
  EFI_BOOT_SERVICES *bootsvc = systab->BootServices;
   
  if(old_idtr[1] != 0)
     edk2_set_idtr((uint8_t *)old_idtr);
  
  if(new_idt)
     bootsvc->FreePages((uint64_t)new_idt, 1);
      
  if(new_idt_trampoline)
     bootsvc->FreePages((uint64_t)new_idt_trampoline, trampoline_size_pages);
  
}
