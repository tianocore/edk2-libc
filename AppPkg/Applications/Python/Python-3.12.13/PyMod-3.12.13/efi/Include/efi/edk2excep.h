/** @file
    UEFI Python support: edk2excep.

    Copyright (c) 2011-2026, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    https://opensource.org/license/bsd-2-clause-patent.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#pragma once

#include <Uefi.h>
#include <setjmp.h>
#include <stdint.h>

/* EFI_EXCEPTION_TYPE and EFI_SYSTEM_CONTEXT_X64, both used below, live here and
 * are not reachable from Uefi.h. Until now this header compiled only because
 * its two includers happened to include Protocol/Cpu.h first; posixmodule.c
 * includes it without that, so make the dependency explicit rather than
 * ordering-dependent. */
#include <Protocol/DebugSupport.h>

#define EDK2_SEH_MAGIC 0x0053454820435458
#define EDK2_SEH_CONTEXT_SIZE 10

struct _edk2_seh_context;

typedef struct _edk2_seh_context {
  uint64_t magic;  
  jmp_buf ret_context;
  EFI_EXCEPTION_TYPE exc_kind;
  EFI_SYSTEM_CONTEXT_X64 exc_context;
} edk2_seh_context_t;
#if defined(_MSC_VER)
# pragma pack(push, 1)
typedef struct _excep_idt_entry {
  UINT16 offset_0_15;
  UINT16 segment;
  UINT8 ist_and_reserved;
  UINT8 type_dpl_p;
  UINT16 offset_16_31;
  UINT32 offset_32_63;
  UINT32 reserved2;
} edk2_idt_entry_t;
# pragma pack(pop)
#else
typedef struct _excep_idt_entry {
  uint64_t offset_0_15 : 16;
  uint64_t segment : 16;

  uint64_t ist : 3;
  uint64_t reserved1 : 5;
  uint64_t id : 3;
  uint64_t size : 2;
  uint64_t dpl : 2;
  uint64_t p : 1;
  uint64_t offset_16_31 : 16;

  uint64_t offset_32_63 : 32;
  uint64_t reserved2 : 32;
} edk2_idt_entry_t;
#endif

void
edk2_get_idtr(uint8_t *);
void
edk2_set_idtr(uint8_t *);
void *
edk2_seh_try();
void
edk2_seh_catch(INTN should_raise, uint64_t *exc_kind_out, uint8_t *buf, size_t bufsize);
/* One guarded 1/2/4/8-byte load or store. 0 = ok, 1 = faulted (kind_out and
 * ctx_out filled in), -1 = guard unavailable (nesting depth exhausted). */
int
edk2_guarded_access(int is_write, unsigned long long addr, int size,
                    volatile unsigned long long *value,
                    uint64_t *kind_out, EFI_SYSTEM_CONTEXT_X64 *ctx_out);
/* One guarded byte-at-a-time copy of len bytes. Either side may fault. Return
 * values match edk2_guarded_access. On a fault the destination holds however
 * much was copied before it, which is not useful and should be discarded. */
int
edk2_guarded_copy(void *dst, const void *src, size_t len,
                  uint64_t *kind_out, EFI_SYSTEM_CONTEXT_X64 *ctx_out);
EFI_STATUS
py_install_idt();
void
py_restore_idt();
