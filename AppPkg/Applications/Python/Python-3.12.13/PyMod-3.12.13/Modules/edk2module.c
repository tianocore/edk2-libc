/** @file
    UEFI CHIPSEC / platform helper module.

    Copyright (c) 2011-2026, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    https://opensource.org/license/bsd-2-clause-patent.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

/* edk2module.c — platform-level UEFI operations (MSR, CPUID, PCI, port I/O,
 * physical memory, SMI, UEFI variables) for firmware tooling such as CHIPSEC.
 *
 * Python 3.6.8 shipped a single module named `edk2` that was two things at
 * once: an `os` implementation for UEFI, and a set of platform primitives. In
 * 3.12.13 that module is split. The `os` half became `uefi`
 * (PyMod-3.12.13/Modules/posixmodule.c, MODNAME "uefi"), which is upstream
 * CPython adapted for UEFI. This file is the other half, and it keeps the name
 * `edk2` so that the tools consuming those primitives import it unchanged.
 *
 * These functions deliberately do NOT live in posixmodule.c. `uefi` is the `os`
 * module, and importlib._bootstrap_external imports it while the interpreter is
 * still starting, so anything added there executes during startup: a failure
 * would not disable a function, it would stop Python from starting, on exactly
 * the sort of platform where there is then no Python left to investigate with.
 * This module is reached only by an explicit `import edk2`, so the worst case
 * is an import that raises while the interpreter stays usable. The inittab in
 * PyMod-3.12.13/Modules/config.c is a static table that is scanned only on
 * import, so the entry costs nothing at boot. Keep it that way: nothing in
 * here may be required for startup.
 *
 * Built only from Python312.inf (FULL). MIN does not compile this file, and the
 * matching config.c entry is gated on BUILD_PYTHON312_FULL to keep the MIN link
 * closed.
 *
 * See Python312_Chipsec_Platform_API_Port.md for the port plan, the 3.6.8
 * defects that are deliberately not reproduced here, and the phase ordering.
 */

/* Required before Python.h for the y# in writemem: since 3.10 a '#' format
 * without this is a SystemError at call time rather than a compile error, so it
 * is invisible until the function is actually used. Every other module in this
 * tree that uses a '#' format does the same. Historically it selected between
 * int and Py_ssize_t lengths, which is the same ambiguity that made 3.6.8's
 * writemem parse s# into an `int len` and corrupt its stack frame. */
#define PY_SSIZE_T_CLEAN

#include "Python.h"

#include <Uefi.h>
#include <Pi/PiDxeCis.h>
#include <Protocol/MpService.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <efi/edk2excep.h>
#include "edk2fault.h"

PyDoc_STRVAR(module_doc,
"Platform-level UEFI operations: MSR, CPUID, PCI config, port I/O, physical\n\
memory, software SMI and UEFI variables.\n\
\n\
allocphysmem allocates physically contiguous pages below a maximum physical\n\
address via Boot Services; freephysmem releases a prior allocphysmem result.\n\
\n\
This is the platform half of the module that Python 3.6.8 called `edk2`. The\n\
operating-system half of that module — open, read, stat, listdir and the rest\n\
— is now the `uefi` module, which is what `os` is built on. Code looking for\n\
file and process operations wants `uefi` (or just `os`); code looking for\n\
firmware primitives is in the right place.\n\
\n\
Guarded memory access raises uefi.FaultError, re-exposed here as\n\
edk2.FaultError, when an access takes a CPU fault that would otherwise have\n\
stopped the machine.");

static int edk2_check_bs(void);

/* ---------------------------------------------------------------------------
 * MSR and CPUID
 *
 * Signatures follow Python 3.6.8's edk2module.c exactly, including the split
 * 32-bit halves, because consumers call these positionally and the return
 * shape is their ABI. A single 64-bit value would be nicer and is not on offer.
 *
 * No Py_BEGIN_ALLOW_THREADS around any of these, unlike 3.6.8. Threading is
 * stubbed in this build (efi/src/dummy_pthread.c), so releasing the GIL around
 * one instruction buys nothing and adds a moving part.
 * ------------------------------------------------------------------------- */

PyDoc_STRVAR(edk2_rdmsr__doc__,
"rdmsr(msr) -> (lower_32bits, higher_32bits)\n\
\n\
Read the given MSR on the current processor and return it as two 32-bit\n\
halves. Use rdmsr_ex to target a specific processor.\n\
\n\
No validation is possible: a reserved or unimplemented MSR raises #GP, which\n\
is a CPU fault and not catchable here, so it stops the machine.");

static PyObject *
edk2_rdmsr(PyObject *self, PyObject *args)
{
    unsigned int msr;
    UINT64 data;

    if (!PyArg_ParseTuple(args, "I:rdmsr", &msr))
        return NULL;

    data = AsmReadMsr64(msr);

    return Py_BuildValue("(II)",
                         (unsigned int)(data & 0xFFFFFFFFu),
                         (unsigned int)(data >> 32));
}

PyDoc_STRVAR(edk2_wrmsr__doc__,
"wrmsr(msr, lower_32bits, higher_32bits) -> None\n\
\n\
Write higher_32bits:lower_32bits to the given MSR on the current processor.\n\
\n\
There is no undo and no confirmation. Writing an MSR the firmware or a driver\n\
relies on can destabilise the platform in ways that only appear later.");

static PyObject *
edk2_wrmsr(PyObject *self, PyObject *args)
{
    unsigned int msr, eax, edx;

    if (!PyArg_ParseTuple(args, "III:wrmsr", &msr, &eax, &edx))
        return NULL;

    AsmWriteMsr64(msr, ((UINT64)edx << 32) | (UINT64)eax);

    Py_RETURN_NONE;
}

PyDoc_STRVAR(edk2_cpuid__doc__,
"cpuid(eax, ecx) -> (eax, ebx, ecx, edx)\n\
\n\
Execute CPUID with the given leaf in eax and subleaf in ecx on the current\n\
processor, and return all four result registers.\n\
\n\
ecx is significant only for leaves that define a subleaf; pass 0 otherwise.");

static PyObject *
edk2_cpuid(PyObject *self, PyObject *args)
{
    unsigned int leaf, subleaf;
    UINT32 eax = 0, ebx = 0, ecx = 0, edx = 0;

    if (!PyArg_ParseTuple(args, "II:cpuid", &leaf, &subleaf))
        return NULL;

    AsmCpuidEx(leaf, subleaf, &eax, &ebx, &ecx, &edx);

    return Py_BuildValue("(IIII)",
                         (unsigned int)eax, (unsigned int)ebx,
                         (unsigned int)ecx, (unsigned int)edx);
}

/* ---------------------------------------------------------------------------
 * MP Services — lazy locate (§6.1). Used only by rdmsr_ex / wrmsr_ex / cpuid_ex.
 * ------------------------------------------------------------------------- */

#define EDK2_AP_FUNCTION_TIMEOUT_US  5000000

typedef struct {
    UINT32 msr;
    UINT64 data;
} edk2_ap_msr_args;

typedef struct {
    UINT32 eax;
    UINT32 ecx;
    UINT32 rax_value;
    UINT32 rbx_value;
    UINT32 rcx_value;
    UINT32 rdx_value;
} edk2_ap_cpuid_args;

static EFI_MP_SERVICES_PROTOCOL *edk2_mp;
static UINTN edk2_mp_bsp;
static UINTN edk2_mp_num_procs;
/* 0 = not yet looked up, 1 = available, -1 = absent */
static int edk2_mp_ready;

static VOID EFIAPI
edk2_ap_msr_read(IN VOID *context)
{
    edk2_ap_msr_args *args = (edk2_ap_msr_args *)context;

    args->data = AsmReadMsr64(args->msr);
}

static VOID EFIAPI
edk2_ap_msr_write(IN VOID *context)
{
    edk2_ap_msr_args *args = (edk2_ap_msr_args *)context;

    AsmWriteMsr64(args->msr, args->data);
}

static VOID EFIAPI
edk2_ap_cpuid(IN VOID *context)
{
    edk2_ap_cpuid_args *args = (edk2_ap_cpuid_args *)context;

    AsmCpuidEx(args->eax, args->ecx,
               &args->rax_value, &args->rbx_value,
               &args->rcx_value, &args->rdx_value);
}

static int
edk2_mp_ensure(void)
{
    EFI_STATUS status;
    UINTN enabled;

    if (edk2_mp_ready < 0) {
        PyErr_SetString(PyExc_OSError,
                        "EFI MP Services protocol is not available");
        return -1;
    }
    if (edk2_mp_ready > 0)
        return 0;
    if (edk2_check_bs() < 0)
        return -1;

    status = gBS->LocateProtocol(&gEfiMpServiceProtocolGuid,
                                 NULL,
                                 (VOID **)&edk2_mp);
    if (EFI_ERROR(status)) {
        edk2_mp_ready = -1;
        PyErr_SetString(PyExc_OSError,
                        "EFI MP Services protocol is not available");
        return -1;
    }

    status = edk2_mp->WhoAmI(edk2_mp, &edk2_mp_bsp);
    if (EFI_ERROR(status)) {
        edk2_mp = NULL;
        edk2_mp_ready = -1;
        PyErr_SetString(PyExc_OSError,
                        "EFI MP Services WhoAmI failed");
        return -1;
    }

    status = edk2_mp->GetNumberOfProcessors(edk2_mp,
                                            &edk2_mp_num_procs,
                                            &enabled);
    if (EFI_ERROR(status)) {
        edk2_mp = NULL;
        edk2_mp_ready = -1;
        PyErr_SetString(PyExc_OSError,
                        "EFI MP Services GetNumberOfProcessors failed");
        return -1;
    }

    edk2_mp_ready = 1;
    return 0;
}

static int
edk2_mp_startup_ap(EFI_AP_PROCEDURE procedure,
                   UINTN processor_number,
                   VOID *context)
{
    EFI_STATUS status;
    BOOLEAN finished;

    finished = FALSE;
    status = edk2_mp->StartupThisAP(edk2_mp,
                                    procedure,
                                    processor_number,
                                    NULL,
                                    EDK2_AP_FUNCTION_TIMEOUT_US,
                                    context,
                                    &finished);
    if (EFI_ERROR(status)) {
        PyErr_SetString(PyExc_OSError, "Could not start the requested cpu");
        return -1;
    }
    if (!finished) {
        PyErr_SetString(PyExc_OSError,
                        "Timeout while running the function on the given cpu");
        return -1;
    }
    return 0;
}

PyDoc_STRVAR(edk2_rdmsr_ex__doc__,
"rdmsr_ex(cpu, msr) -> (lower_32bits, higher_32bits)\n\
\n\
Read the given MSR on processor cpu. cpu must be less than the number of\n\
processors reported by MP Services. On the current BSP, the read runs locally;\n\
on other processors StartupThisAP is used.");

static PyObject *
edk2_rdmsr_ex(PyObject *self, PyObject *args)
{
    unsigned int cpu, msr;
    UINT64 data;
    edk2_ap_msr_args ap_args;

    if (!PyArg_ParseTuple(args, "II:rdmsr_ex", &cpu, &msr))
        return NULL;
    if (edk2_mp_ensure() < 0)
        return NULL;

    if (cpu >= edk2_mp_num_procs) {
        PyErr_SetString(PyExc_ValueError, "Invalid cpu number provided");
        return NULL;
    }

    if ((UINTN)cpu == edk2_mp_bsp) {
        data = AsmReadMsr64(msr);
    } else {
        ap_args.msr = msr;
        ap_args.data = 0;
        if (edk2_mp_startup_ap(edk2_ap_msr_read, (UINTN)cpu, &ap_args) < 0)
            return NULL;
        data = ap_args.data;
    }

    return Py_BuildValue("(II)",
                         (unsigned int)(data & 0xFFFFFFFFu),
                         (unsigned int)(data >> 32));
}

PyDoc_STRVAR(edk2_wrmsr_ex__doc__,
"wrmsr_ex(cpu, msr, lower_32bits, higher_32bits) -> None\n\
\n\
Write to the given MSR on processor cpu. Same cpu validation as rdmsr_ex.");

static PyObject *
edk2_wrmsr_ex(PyObject *self, PyObject *args)
{
    unsigned int cpu, msr, eax, edx;
    edk2_ap_msr_args ap_args;

    if (!PyArg_ParseTuple(args, "IIII:wrmsr_ex", &cpu, &msr, &eax, &edx))
        return NULL;
    if (edk2_mp_ensure() < 0)
        return NULL;

    if (cpu >= edk2_mp_num_procs) {
        PyErr_SetString(PyExc_ValueError, "Invalid cpu number provided");
        return NULL;
    }

    ap_args.msr = msr;
    ap_args.data = ((UINT64)edx << 32) | (UINT64)eax;

    if ((UINTN)cpu == edk2_mp_bsp)
        AsmWriteMsr64(msr, ap_args.data);
    else if (edk2_mp_startup_ap(edk2_ap_msr_write, (UINTN)cpu, &ap_args) < 0)
        return NULL;

    Py_RETURN_NONE;
}

PyDoc_STRVAR(edk2_cpuid_ex__doc__,
"cpuid_ex(cpu, eax, ecx) -> (eax, ebx, ecx, edx)\n\
\n\
Execute CPUID on processor cpu with the given leaf and subleaf.");

static PyObject *
edk2_cpuid_ex(PyObject *self, PyObject *args)
{
    unsigned int cpu, leaf, subleaf;
    edk2_ap_cpuid_args ap_args;

    if (!PyArg_ParseTuple(args, "III:cpuid_ex", &cpu, &leaf, &subleaf))
        return NULL;
    if (edk2_mp_ensure() < 0)
        return NULL;

    if (cpu >= edk2_mp_num_procs) {
        PyErr_SetString(PyExc_ValueError, "Invalid cpu number provided");
        return NULL;
    }

    ap_args.eax = leaf;
    ap_args.ecx = subleaf;
    ap_args.rax_value = 0;
    ap_args.rbx_value = 0;
    ap_args.rcx_value = 0;
    ap_args.rdx_value = 0;

    if ((UINTN)cpu == edk2_mp_bsp)
        edk2_ap_cpuid(&ap_args);
    else if (edk2_mp_startup_ap(edk2_ap_cpuid, (UINTN)cpu, &ap_args) < 0)
        return NULL;

    return Py_BuildValue("(IIII)",
                         (unsigned int)ap_args.rax_value,
                         (unsigned int)ap_args.rbx_value,
                         (unsigned int)ap_args.rcx_value,
                         (unsigned int)ap_args.rdx_value);
}

/* ---------------------------------------------------------------------------
 * Port I/O
 *
 * Three things are checked here that 3.6.8 did not check, each of which is a
 * real failure rather than pedantry:
 *
 * 1. The port number. 3.6.8 did `addrs = (short)(addr & 0xffff)` into a
 *    *signed* short, so every port from 0x8000 up became negative and then
 *    sign-extended to an enormous UINTN inside IoRead8 — i.e. the whole upper
 *    half of the I/O space addressed the wrong thing. A ValueError beats both
 *    that and the silent truncation the mask was there for.
 * 2. Alignment. MdePkg's IoRead16/IoRead32 ASSERT on a misaligned port, and
 *    this build has PcdDebugPropertyMask 0x0F (asserts enabled), so a
 *    misaligned access does not misbehave subtly — it stops the machine in
 *    DebugAssert(). Checking it here turns that into an exception.
 * 3. The value, for writes. 3.6.8 masked with & 0xFF, which silently writes
 *    something other than what the caller asked for. OverflowError matches
 *    uefi.mem_write, which made the same call for the same reason.
 *
 * An unimplemented port is still not detectable: reads return 0xFF..., writes
 * go nowhere, and neither faults.
 * ------------------------------------------------------------------------- */

static int
edk2_check_port(unsigned int port, unsigned int size)
{
    if (size != 1 && size != 2 && size != 4) {
        PyErr_Format(PyExc_ValueError, "size must be 1, 2 or 4, not %u", size);
        return -1;
    }
    if (port > 0xFFFF) {
        PyErr_Format(PyExc_ValueError,
                     "I/O port must be 0x0000-0xFFFF, not 0x%x", port);
        return -1;
    }
    if (port % size != 0) {
        PyErr_Format(PyExc_ValueError,
                     "port 0x%x is not %u-byte aligned; a misaligned port I/O "
                     "access trips an ASSERT in IoLib and stops the machine",
                     port, size);
        return -1;
    }
    return 0;
}

PyDoc_STRVAR(edk2_readio__doc__,
"readio(port, size) -> int\n\
\n\
Read size bytes (1, 2 or 4) from the given I/O port.\n\
\n\
port must be 0x0000-0xFFFF and size-aligned. An unimplemented port typically\n\
reads back all ones rather than failing.");

static PyObject *
edk2_readio(PyObject *self, PyObject *args)
{
    unsigned int port, size;
    UINT32 value = 0;

    if (!PyArg_ParseTuple(args, "II:readio", &port, &size))
        return NULL;
    if (edk2_check_port(port, size) < 0)
        return NULL;

    switch (size) {
    case 1: value = IoRead8((UINTN)port);  break;
    case 2: value = IoRead16((UINTN)port); break;
    case 4: value = IoRead32((UINTN)port); break;
    }

    return PyLong_FromUnsignedLong((unsigned long)value);
}

PyDoc_STRVAR(edk2_writeio__doc__,
"writeio(port, size, value) -> None\n\
\n\
Write value as size bytes (1, 2 or 4) to the given I/O port.\n\
\n\
port must be 0x0000-0xFFFF and size-aligned, and value must fit in size --\n\
a value too large is an OverflowError rather than a silent truncation, since\n\
a truncated write to hardware is worse than a refused one.");

static PyObject *
edk2_writeio(PyObject *self, PyObject *args)
{
    unsigned int port, size;
    unsigned long long value;

    if (!PyArg_ParseTuple(args, "IIK:writeio", &port, &size, &value))
        return NULL;
    if (edk2_check_port(port, size) < 0)
        return NULL;
    if (value >> (size * 8) != 0) {
        PyErr_Format(PyExc_OverflowError,
                     "value does not fit in %u byte(s)", size);
        return NULL;
    }

    switch (size) {
    case 1: IoWrite8((UINTN)port,  (UINT8 )value); break;
    case 2: IoWrite16((UINTN)port, (UINT16)value); break;
    case 4: IoWrite32((UINTN)port, (UINT32)value); break;
    }

    Py_RETURN_NONE;
}

/* ---------------------------------------------------------------------------
 * PCI configuration space
 *
 * Via PciLib, which AppPkg.dsc maps to BasePciLibCf8 — so every access here
 * goes through the legacy 0xCF8/0xCFC mechanism, and that has consequences the
 * caller has to know about:
 *
 *   - Register offsets stop at 0xFF. Extended configuration space (0x100-0xFFF,
 *     where PCIe capabilities live) is simply not reachable by CF8, and
 *     BasePciCf8Lib enforces that with an ASSERT: its address check is
 *     `ASSERT (((A) & (~0xffff0ff | (M))) == 0)`, and PCI_LIB_ADDRESS puts the
 *     register in bits 0..11, so an offset of 0x100 or more sets bits 8..11 and
 *     trips it. Asserts are live in this build. Reaching extended space means
 *     remapping PciLib in the DSC to an ECAM implementation, not changing this
 *     file — which is why the check below is a range error naming the limit
 *     rather than something that pretends to work.
 *   - That same M parameter is 1 for the 16-bit accessors and 3 for the 32-bit
 *     ones, so a misaligned register offset is an ASSERT too, exactly as in
 *     port I/O.
 *
 * 3.6.8 took `unsigned char` for bus, dev, fun and off. For the offset that
 * accidentally avoided the first ASSERT — 0x100 wrapped to 0x00 — by reading a
 * completely different register and returning it as if it were the one asked
 * for. It did nothing about the second. And bus/dev/fun truncation, combined
 * with the masking inside PCI_LIB_ADDRESS, meant an out-of-range device
 * silently became a *different, existing* device: harmless for a read, not for
 * writepci.
 *
 * Note the argument order of writepci: value comes BEFORE size, the opposite of
 * writeio above. That is inconsistent, it is not a mistake here, and it must not
 * be "fixed" — it is 3.6.8's published signature and callers pass positionally.
 * ------------------------------------------------------------------------- */

static int
edk2_check_pci(unsigned int bus, unsigned int dev, unsigned int func,
               unsigned int off, unsigned int size)
{
    if (size != 1 && size != 2 && size != 4) {
        PyErr_Format(PyExc_ValueError, "size must be 1, 2 or 4, not %u", size);
        return -1;
    }
    if (bus > 0xFF || dev > 0x1F || func > 7) {
        PyErr_Format(PyExc_ValueError,
                     "bus/device/function %u/%u/%u out of range "
                     "(max 255/31/7); PCI_LIB_ADDRESS would mask it down to a "
                     "different, possibly populated device",
                     bus, dev, func);
        return -1;
    }
    if (off > 0xFF) {
        PyErr_Format(PyExc_ValueError,
                     "register offset 0x%x exceeds 0xFF; extended configuration "
                     "space is unreachable through the CF8 mechanism that "
                     "PciLib is mapped to, and would trip an ASSERT in "
                     "BasePciCf8Lib", off);
        return -1;
    }
    if (off % size != 0) {
        PyErr_Format(PyExc_ValueError,
                     "register offset 0x%x is not %u-byte aligned; a misaligned "
                     "PCI config access trips an ASSERT in BasePciCf8Lib and "
                     "stops the machine", off, size);
        return -1;
    }
    return 0;
}

PyDoc_STRVAR(edk2_readpci__doc__,
"readpci(bus, dev, func, offset, size) -> int\n\
\n\
Read size bytes (1, 2 or 4) from PCI configuration space at bus:dev.func,\n\
register offset.\n\
\n\
offset must be 0x00-0xFF and size-aligned: PciLib is mapped to the CF8\n\
mechanism here, which cannot reach extended configuration space at all.\n\
\n\
An absent device is not an error -- it reads back as all ones, which is how\n\
you probe for one.");

static PyObject *
edk2_readpci(PyObject *self, PyObject *args)
{
    unsigned int bus, dev, func, off, size;
    UINT32 value = 0;
    UINTN address;

    if (!PyArg_ParseTuple(args, "IIIII:readpci", &bus, &dev, &func, &off, &size))
        return NULL;
    if (edk2_check_pci(bus, dev, func, off, size) < 0)
        return NULL;

    address = PCI_LIB_ADDRESS(bus, dev, func, off);
    switch (size) {
    case 1: value = PciRead8(address);  break;
    case 2: value = PciRead16(address); break;
    case 4: value = PciRead32(address); break;
    }

    return PyLong_FromUnsignedLong((unsigned long)value);
}

PyDoc_STRVAR(edk2_writepci__doc__,
"writepci(bus, dev, func, offset, value, size) -> None\n\
\n\
Write value as size bytes (1, 2 or 4) to PCI configuration space at\n\
bus:dev.func, register offset.\n\
\n\
Note that value precedes size here, unlike writeio(port, size, value). That\n\
is 3.6.8's signature and callers pass positionally, so it stays.\n\
\n\
Same offset restrictions as readpci. Out-of-range bus/dev/func is an error\n\
rather than a silent mask, because a masked write lands on a different and\n\
possibly real device.");

static PyObject *
edk2_writepci(PyObject *self, PyObject *args)
{
    unsigned int bus, dev, func, off, size;
    unsigned long long value;
    UINTN address;

    if (!PyArg_ParseTuple(args, "IIIIKI:writepci",
                          &bus, &dev, &func, &off, &value, &size))
        return NULL;
    if (edk2_check_pci(bus, dev, func, off, size) < 0)
        return NULL;
    if (value >> (size * 8) != 0) {
        PyErr_Format(PyExc_OverflowError,
                     "value does not fit in %u byte(s)", size);
        return NULL;
    }

    address = PCI_LIB_ADDRESS(bus, dev, func, off);
    switch (size) {
    case 1: PciWrite8(address,  (UINT8 )value); break;
    case 2: PciWrite16(address, (UINT16)value); break;
    case 4: PciWrite32(address, (UINT32)value); break;
    }

    Py_RETURN_NONE;
}

/* ---------------------------------------------------------------------------
 * Software SMI
 *
 * _swsmi lives in Modules/cpu.nasm (MSFT) or cpu_gcc.s (GCC), already linked
 * into every FULL and MIN image but unused until now. The assembly uses the
 * Microsoft x64 calling convention (first four args in rcx/rdx/r8/r9, rest on
 * the stack), so the C declaration must match on GCC too.
 *
 * 3.6.8 parsed seven unsigned ints and passed them to an entry point that loads
 * full 64-bit GPR values — anything with meaningful high halves was silently
 * wrong. Here smi_code_data stays unsigned int (only AX is written to 0xB2);
 * the six register arguments are unsigned long long (Py "K").
 *
 * There is no safe default test: any call enters firmware SMM. Acceptance is
 * import/signature/arity only unless the operator opts into a known-safe SMI for
 * their platform (port doc section 15).
 * ------------------------------------------------------------------------- */

#ifdef _MSC_VER
void
_swsmi(unsigned int smi_code_data, UINT64 rax_value, UINT64 rbx_value,
       UINT64 rcx_value, UINT64 rdx_value, UINT64 rsi_value, UINT64 rdi_value);
#else
void
_swsmi(unsigned int smi_code_data, UINT64 rax_value, UINT64 rbx_value,
       UINT64 rcx_value, UINT64 rdx_value, UINT64 rsi_value, UINT64 rdi_value)
    __attribute__((ms_abi));
#endif

PyDoc_STRVAR(edk2_swsmi__doc__,
"swsmi(smi_code_data, rax, rbx, rcx, rdx, rsi, rdi) -> None\n\
\n\
Trigger a software SMI through port 0xB2 with the given data byte and the GPR\n\
values the handler sees on entry (full 64-bit values, not 32-bit halves).\n\
\n\
This enters System Management Mode. There is no safe value on an arbitrary\n\
platform — a wrong SMI number can hang or reset the machine. CHIPSEC callers\n\
know their platform's SMI contract; this wrapper does not validate it.");

static PyObject *
edk2_swsmi(PyObject *self, PyObject *args)
{
    unsigned int smi_code_data;
    unsigned long long rax_value, rbx_value, rcx_value, rdx_value, rsi_value,
        rdi_value;

    if (!PyArg_ParseTuple(args, "IKKKKKK:swsmi", &smi_code_data, &rax_value,
                          &rbx_value, &rcx_value, &rdx_value, &rsi_value,
                          &rdi_value))
        return NULL;

    if (smi_code_data > 0xFFFFu) {
        PyErr_Format(PyExc_ValueError,
                     "smi_code_data must fit in 16 bits, not 0x%x",
                     smi_code_data);
        return NULL;
    }

    _swsmi(smi_code_data, (UINT64)rax_value, (UINT64)rbx_value,
           (UINT64)rcx_value, (UINT64)rdx_value, (UINT64)rsi_value,
           (UINT64)rdi_value);

    Py_RETURN_NONE;
}

/* ---------------------------------------------------------------------------
 * Physical memory
 *
 * These are the functions phases 1 and 2 were built for. In 3.6.8 readmem()
 * dereferenced a caller-supplied address byte by byte with nothing in the way,
 * so a wrong address did not raise — it took a page fault and stopped the
 * machine, on a box that by definition was being poked at because something was
 * already wrong with it. Here every access goes through edk2_guarded_copy() or
 * edk2_guarded_access(), and a fault becomes FaultError with the vector, rip
 * and cr2 attached.
 *
 * That is the single largest behavioural difference in this port, and it is why
 * the guarded path was built and verified on both toolchains before any of
 * these were written rather than after.
 *
 * The split addr_lo/addr_hi signature is 3.6.8's and stays. It exists because
 * the original had an IA32 build to serve; this one does not, but callers pass
 * positionally.
 *
 * A module-level static for the exception type, set in PyInit_edk2 from
 * uefi.FaultError. Single-phase init with m_size = -1 means there is exactly
 * one of these per image, matching the module.
 * ------------------------------------------------------------------------- */

static PyObject *edk2_fault_error = NULL;

static uint64_t
edk2_addr(unsigned int addr_lo, unsigned int addr_hi)
{
    return ((uint64_t)addr_hi << 32) | (uint64_t)addr_lo;
}

/* Shared tail for the four functions below: turn edk2_guarded_* return codes
 * into the right Python exception. Returns 0 if the caller should carry on. */
static int
edk2_fault_check(int rc, uint64_t kind, const EFI_SYSTEM_CONTEXT_X64 *ctx)
{
    if (rc < 0) {
        PyErr_Format(PyExc_RuntimeError,
                     "fault guard nesting depth (%d) exceeded",
                     EDK2_SEH_CONTEXT_SIZE);
        return -1;
    }
    if (rc > 0) {
        uefi_set_fault_error(edk2_fault_error, kind, ctx);
        return -1;
    }
    return 0;
}

PyDoc_STRVAR(edk2_readmem__doc__,
"readmem(addr_lo, addr_hi, length) -> bytes\n\
\n\
Read length bytes from the physical address formed by addr_hi:addr_lo.\n\
\n\
Raises FaultError if the read takes a CPU fault, so a wrong address is a\n\
Python exception rather than a dead machine. The read is byte at a time and\n\
volatile, which matters when the target is MMIO.\n\
\n\
On a fault nothing is returned -- a partially read buffer is not useful. Use\n\
uefi.mem_probe first if you want to test an address without an exception.");

static PyObject *
edk2_readmem(PyObject *self, PyObject *args)
{
    unsigned int addr_lo, addr_hi;
    Py_ssize_t length;
    PyObject *result;
    uint64_t kind = 0;
    EFI_SYSTEM_CONTEXT_X64 ctx;
    int rc;

    if (!PyArg_ParseTuple(args, "IIn:readmem", &addr_lo, &addr_hi, &length))
        return NULL;
    if (length < 0) {
        PyErr_SetString(PyExc_ValueError, "length must not be negative");
        return NULL;
    }

    /* Allocate the bytes object up front and read straight into it. 3.6.8
     * malloc'd a scratch buffer, copied out of it and — on allocation failure —
     * returned NULL with no exception set, which surfaces as a confusing
     * SystemError rather than MemoryError. This has neither problem. */
    result = PyBytes_FromStringAndSize(NULL, length);
    if (result == NULL)
        return NULL;
    if (length == 0)
        return result;

    rc = edk2_guarded_copy(PyBytes_AS_STRING(result),
                           (const void *)(uintptr_t)edk2_addr(addr_lo, addr_hi),
                           (size_t)length, &kind, &ctx);
    if (edk2_fault_check(rc, kind, &ctx) < 0) {
        Py_DECREF(result);
        return NULL;
    }
    return result;
}

PyDoc_STRVAR(edk2_readmem_dword__doc__,
"readmem_dword(addr_lo, addr_hi) -> int\n\
\n\
Read one 32-bit value from the physical address addr_hi:addr_lo.\n\
\n\
A single 4-byte access, not four byte accesses, which is what MMIO registers\n\
generally require. Raises FaultError if it faults.");

static PyObject *
edk2_readmem_dword(PyObject *self, PyObject *args)
{
    unsigned int addr_lo, addr_hi;
    volatile unsigned long long value = 0;
    uint64_t kind = 0;
    EFI_SYSTEM_CONTEXT_X64 ctx;
    int rc;

    if (!PyArg_ParseTuple(args, "II:readmem_dword", &addr_lo, &addr_hi))
        return NULL;

    rc = edk2_guarded_access(0, edk2_addr(addr_lo, addr_hi), 4, &value,
                             &kind, &ctx);
    if (edk2_fault_check(rc, kind, &ctx) < 0)
        return NULL;
    return PyLong_FromUnsignedLongLong((unsigned long long)value);
}

PyDoc_STRVAR(edk2_writemem__doc__,
"writemem(addr_lo, addr_hi, buf) -> None\n\
\n\
Write the bytes in buf to the physical address addr_hi:addr_lo.\n\
\n\
buf must be bytes, not str. 3.6.8 accepted str and wrote its UTF-8 encoding,\n\
which silently writes a different number of bytes than the string has\n\
characters as soon as one is non-ASCII.\n\
\n\
Raises FaultError if the write faults, in which case an unknown prefix of buf\n\
has already been written.");

static PyObject *
edk2_writemem(PyObject *self, PyObject *args)
{
    unsigned int addr_lo, addr_hi;
    const char *buf;
    Py_ssize_t length;
    uint64_t kind = 0;
    EFI_SYSTEM_CONTEXT_X64 ctx;
    int rc;

    if (!PyArg_ParseTuple(args, "IIy#:writemem", &addr_lo, &addr_hi,
                          &buf, &length))
        return NULL;
    if (length == 0)
        Py_RETURN_NONE;

    rc = edk2_guarded_copy((void *)(uintptr_t)edk2_addr(addr_lo, addr_hi),
                           buf, (size_t)length, &kind, &ctx);
    if (edk2_fault_check(rc, kind, &ctx) < 0)
        return NULL;
    Py_RETURN_NONE;
}

PyDoc_STRVAR(edk2_writemem_dword__doc__,
"writemem_dword(addr_lo, addr_hi, value) -> None\n\
\n\
Write one 32-bit value to the physical address addr_hi:addr_lo.\n\
\n\
A single 4-byte access. Raises FaultError if it faults.");

static PyObject *
edk2_writemem_dword(PyObject *self, PyObject *args)
{
    unsigned int addr_lo, addr_hi, val;
    volatile unsigned long long value;
    uint64_t kind = 0;
    EFI_SYSTEM_CONTEXT_X64 ctx;
    int rc;

    if (!PyArg_ParseTuple(args, "III:writemem_dword", &addr_lo, &addr_hi, &val))
        return NULL;

    value = val;
    rc = edk2_guarded_access(1, edk2_addr(addr_lo, addr_hi), 4, &value,
                             &kind, &ctx);
    if (edk2_fault_check(rc, kind, &ctx) < 0)
        return NULL;
    Py_RETURN_NONE;
}

/* ---------------------------------------------------------------------------
 * UEFI runtime variables (gRT->GetVariable / GetNextVariableName / SetVariable)
 *
 * Signatures and return tuple shapes match Python 3.6.8's edk2module.c so CHIPSEC
 * call sites stay positional. The shapes come from the 3.6.8 Py_BuildValue calls,
 * not its docstrings: GetNextVariableName returns (Status, Name, NameSize, GUID)
 * even though its docstring claims NameSize comes second.
 *
 * Names accept str or UTF-16LE bytes; GUIDs accept the string form or the 16
 * raw bytes of uuid.UUID.bytes_le, which is the EFI_GUID memory layout. CHIPSEC
 * passes str to GetVariable/SetVariable and bytes to GetNextVariableName, so
 * both spellings have to work on every entry point.
 * ------------------------------------------------------------------------- */

static int
edk2_check_rt(void)
{
    if (gRT == NULL || gRT->GetVariable == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "UEFI runtime services table is not available");
        return -1;
    }
    return 0;
}

/* Caller frees *out with PyMem_Free in both branches. */
static int
edk2_arg_to_char16(PyObject *obj, CHAR16 **out)
{
    char *raw;
    Py_ssize_t len, chars;
    CHAR16 *buf;

    if (PyUnicode_Check(obj)) {
        wchar_t *wide = PyUnicode_AsWideCharString(obj, NULL);
        if (wide == NULL)
            return -1;
        *out = (CHAR16 *)wide;
        return 0;
    }
    if (!PyBytes_Check(obj)) {
        PyErr_SetString(PyExc_TypeError,
                        "VariableName must be str or UTF-16LE bytes");
        return -1;
    }
    if (PyBytes_AsStringAndSize(obj, &raw, &len) < 0)
        return -1;
    if ((len % (Py_ssize_t)sizeof(CHAR16)) != 0) {
        PyErr_SetString(PyExc_ValueError,
                        "VariableName bytes must be UTF-16LE (even length)");
        return -1;
    }
    chars = len / (Py_ssize_t)sizeof(CHAR16);
    buf = (CHAR16 *)PyMem_Malloc((size_t)(chars + 1) * sizeof(CHAR16));
    if (buf == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    if (len > 0)
        memcpy(buf, raw, (size_t)len);
    buf[chars] = 0;
    *out = buf;
    return 0;
}

static int
edk2_arg_to_guid(PyObject *obj, EFI_GUID *guid)
{
    const char *ascii;
    char *raw;
    Py_ssize_t len;

    if (PyBytes_Check(obj)) {
        if (PyBytes_AsStringAndSize(obj, &raw, &len) < 0)
            return -1;
        if (len != (Py_ssize_t)sizeof(EFI_GUID)) {
            PyErr_SetString(PyExc_ValueError,
                            "GUID bytes must be 16 bytes (uuid.UUID.bytes_le)");
            return -1;
        }
        memcpy(guid, raw, sizeof(EFI_GUID));
        return 0;
    }
    if (!PyUnicode_Check(obj)) {
        PyErr_SetString(PyExc_TypeError,
                        "GUID must be str or 16 bytes");
        return -1;
    }
    ascii = PyUnicode_AsUTF8AndSize(obj, &len);
    if (ascii == NULL)
        return -1;
    if (RETURN_ERROR(AsciiStrToGuid(ascii, guid))) {
        PyErr_SetString(PyExc_ValueError,
                        "GUID must be XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX");
        return -1;
    }
    return 0;
}

static PyObject *
edk2_guid_to_unicode(const EFI_GUID *guid)
{
    char buf[37];

    AsciiSPrint(buf, sizeof(buf),
                "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                guid->Data1, guid->Data2, guid->Data3,
                guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
                guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
    return PyUnicode_FromString(buf);
}

static void
edk2_guid_to_ascii(const EFI_GUID *guid, char *buf, UINTN buf_len)
{
    AsciiSPrint(buf, buf_len,
                "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                guid->Data1, guid->Data2, guid->Data3,
                guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
                guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
}

/* 3.6.8 returned (UINT32)Status. CHIPSEC maps small integers (5 = BUFFER_TOO_SMALL);
 * UINTN error codes like 0x8000000000000005 must not reach Python as 64-bit values. */
static unsigned int
edk2_status_for_python(EFI_STATUS status)
{
    return (unsigned int)(UINT32)status;
}

/* UTF-16LE CHAR16 buffer from GetNextVariableName; name_size is bytes (UEFI API). */
static PyObject *
edk2_char16_name_to_unicode(const CHAR16 *buf, UINTN name_size_bytes)
{
    Py_ssize_t max_units;
    Py_ssize_t nunits;

    max_units = (Py_ssize_t)(name_size_bytes / sizeof(CHAR16));
    if (max_units < 0)
        max_units = 0;
    nunits = 0;
    while (nunits < max_units && buf[nunits] != (CHAR16)0)
        nunits++;
    if (nunits == 0)
        return PyUnicode_New(0, 0);
    return PyUnicode_Decode((const char *)buf,
                            nunits * (Py_ssize_t)sizeof(CHAR16),
                            "utf-16-le", NULL);
}

PyDoc_STRVAR(edk2_GetVariable__doc__,
"GetVariable(VariableName, GUID, DataSize) -> (Status, Attributes, Data, DataSize)\n\
\n\
Read a UEFI variable. VariableName is str or UTF-16LE bytes, GUID is a string or\n\
uuid.UUID.bytes_le, and DataSize is the caller's buffer size in bytes. On\n\
EFI_SUCCESS, Data is bytes and the last element is the size returned. On\n\
EFI_BUFFER_TOO_SMALL, Data is empty and the last element is the required size.");

static PyObject *
edk2_GetVariable(PyObject *self, PyObject *args)
{
    PyObject *name_obj, *guid_obj, *result;
    PyObject *status_obj, *attr_obj, *data_obj, *size_obj;
    CHAR16 *name = NULL;
    EFI_GUID vendor_guid;
    unsigned long long data_size_in;
    UINTN data_size;
    UINT32 attributes = 0;
    EFI_STATUS status;
    char *data = NULL;
    Py_ssize_t data_len;

    if (!PyArg_ParseTuple(args, "OOK:GetVariable",
                          &name_obj, &guid_obj, &data_size_in))
        return NULL;
    if (edk2_check_rt() < 0)
        return NULL;
    if (edk2_arg_to_char16(name_obj, &name) < 0)
        return NULL;
    if (edk2_arg_to_guid(guid_obj, &vendor_guid) < 0) {
        PyMem_Free(name);
        return NULL;
    }

    data_size = (UINTN)data_size_in;
    if (data_size > 0) {
        data = (char *)malloc(data_size);
        if (data == NULL) {
            PyMem_Free(name);
            return PyErr_NoMemory();
        }
    }

    status = gRT->GetVariable(name, &vendor_guid, &attributes,
                              &data_size, data);
    PyMem_Free(name);

    if (status == EFI_SUCCESS && data != NULL && data_size > 0)
        data_len = (Py_ssize_t)data_size;
    else
        data_len = 0;

    data_obj = PyBytes_FromStringAndSize(
        (data_len > 0) ? data : "", data_len);
    free(data);
    if (data_obj == NULL)
        return NULL;

    /* Same rationale as GetNextVariableName: avoid Py_BuildValue varargs (IIy#K). */
    status_obj = PyLong_FromUnsignedLong((unsigned long)edk2_status_for_python(status));
    attr_obj = PyLong_FromUnsignedLong((unsigned long)attributes);
    size_obj = PyLong_FromUnsignedLongLong((unsigned long long)data_size);
    if (status_obj == NULL || attr_obj == NULL || size_obj == NULL) {
        Py_XDECREF(status_obj);
        Py_XDECREF(attr_obj);
        Py_XDECREF(size_obj);
        Py_DECREF(data_obj);
        return NULL;
    }
    result = PyTuple_Pack(4, status_obj, attr_obj, data_obj, size_obj);
    Py_DECREF(status_obj);
    Py_DECREF(attr_obj);
    Py_DECREF(data_obj);
    Py_DECREF(size_obj);
    return result;
}

PyDoc_STRVAR(edk2_GetNextVariableName__doc__,
"GetNextVariableName(NameSize, VariableName, GUID) -> (Status, Name, NameSize, GUID)\n\
\n\
Enumerate variables. Pass an empty Name to start; on success the returned Name\n\
and GUID are the next entry and NameSize is updated. NameSize is the\n\
VariableName buffer size in bytes (same as the UEFI API). VariableName is str\n\
or UTF-16LE bytes; GUID is a string or uuid.UUID.bytes_le.");

static PyObject *
edk2_GetNextVariableName(PyObject *self, PyObject *args)
{
    PyObject *name_obj, *guid_obj, *name_out, *result;
    PyObject *status_obj, *size_obj, *guid_str_obj;
    CHAR16 *name_buf = NULL;
    CHAR16 *name_wide = NULL;
    EFI_GUID vendor_guid;
    char guid_ascii[37];
    unsigned long long name_size_in;
    UINTN name_size;
    EFI_STATUS status;
    Py_ssize_t name_chars;

    if (!PyArg_ParseTuple(args, "KOO:GetNextVariableName",
                          &name_size_in, &name_obj, &guid_obj))
        return NULL;
    if (edk2_check_rt() < 0)
        return NULL;
    if (name_size_in < sizeof(CHAR16)) {
        PyErr_SetString(PyExc_ValueError,
                        "NameSize must be at least 2 (one UTF-16 code unit)");
        return NULL;
    }
    if (edk2_arg_to_char16(name_obj, &name_wide) < 0)
        return NULL;
    if (edk2_arg_to_guid(guid_obj, &vendor_guid) < 0) {
        PyMem_Free(name_wide);
        return NULL;
    }

    name_size = (UINTN)name_size_in;
    name_buf = (CHAR16 *)malloc(name_size);
    if (name_buf == NULL) {
        PyMem_Free(name_wide);
        return PyErr_NoMemory();
    }
    memset(name_buf, 0, name_size);

    name_chars = 0;
    while (name_wide[name_chars] != L'\0')
        name_chars++;
    if ((size_t)(name_chars + 1) * sizeof(CHAR16) > name_size) {
        PyMem_Free(name_wide);
        free(name_buf);
        PyErr_SetString(PyExc_ValueError,
                         "VariableName does not fit in NameSize bytes");
        return NULL;
    }
    memcpy(name_buf, name_wide, (size_t)(name_chars + 1) * sizeof(CHAR16));
    PyMem_Free(name_wide);

    status = gRT->GetNextVariableName(&name_size, name_buf, &vendor_guid);

    name_out = edk2_char16_name_to_unicode(name_buf, name_size);
    edk2_guid_to_ascii(&vendor_guid, guid_ascii, sizeof(guid_ascii));
    free(name_buf);
    if (name_out == NULL)
        return NULL;

    /* 3.6.8 Py_BuildValue("(IuKs)": avoid two U objects in one varargs call. */
    status_obj = PyLong_FromUnsignedLong((unsigned long)edk2_status_for_python(status));
    size_obj = PyLong_FromUnsignedLongLong((unsigned long long)name_size);
    guid_str_obj = PyUnicode_FromString(guid_ascii);
    if (status_obj == NULL || size_obj == NULL || guid_str_obj == NULL) {
        Py_XDECREF(status_obj);
        Py_XDECREF(size_obj);
        Py_XDECREF(guid_str_obj);
        Py_DECREF(name_out);
        return NULL;
    }
    result = PyTuple_Pack(4, status_obj, name_out, size_obj, guid_str_obj);
    Py_DECREF(status_obj);
    Py_DECREF(name_out);
    Py_DECREF(size_obj);
    Py_DECREF(guid_str_obj);
    return result;
}

PyDoc_STRVAR(edk2_SetVariable__doc__,
"SetVariable(VariableName, GUID, Attributes, Data, DataSize) -> (Status, DataSize, GUID)\n\
\n\
Write a UEFI variable. Data is any bytes-like object; DataSize may match\n\
len(Data) or specify a prefix length, and DataSize 0 deletes the variable.\n\
VariableName is str or UTF-16LE bytes; GUID is a string or uuid.UUID.bytes_le.\n\
Returns the GUID string (unchanged from input) for 3.6.8 tuple compatibility.");

static PyObject *
edk2_SetVariable(PyObject *self, PyObject *args)
{
    PyObject *name_obj, *guid_obj, *guid_out, *result;
    CHAR16 *name = NULL;
    EFI_GUID vendor_guid;
    unsigned int attributes;
    Py_buffer data;
    unsigned long long data_size_in;
    UINTN data_size;
    EFI_STATUS status;

    if (!PyArg_ParseTuple(args, "OOIy*K:SetVariable",
                          &name_obj, &guid_obj, &attributes,
                          &data, &data_size_in))
        return NULL;
    if (edk2_check_rt() < 0) {
        PyBuffer_Release(&data);
        return NULL;
    }
    if ((unsigned long long)data.len < data_size_in) {
        PyBuffer_Release(&data);
        PyErr_SetString(PyExc_ValueError,
                        "DataSize exceeds len(Data)");
        return NULL;
    }
    if (edk2_arg_to_char16(name_obj, &name) < 0) {
        PyBuffer_Release(&data);
        return NULL;
    }
    if (edk2_arg_to_guid(guid_obj, &vendor_guid) < 0) {
        PyBuffer_Release(&data);
        PyMem_Free(name);
        return NULL;
    }

    data_size = (UINTN)data_size_in;
    status = gRT->SetVariable(name, &vendor_guid, attributes,
                              data_size, data.buf);
    PyBuffer_Release(&data);
    PyMem_Free(name);

    guid_out = edk2_guid_to_unicode(&vendor_guid);
    if (guid_out == NULL)
        return NULL;
    result = Py_BuildValue("(IKU)",
                           edk2_status_for_python(status),
                           (unsigned long long)data_size,
                           guid_out);
    Py_DECREF(guid_out);
    return result;
}

/* ---------------------------------------------------------------------------
 * Physical memory below max_pa (gBS->AllocatePages / AllocateMaxAddress)
 *
 * 3.6.8 used malloc and returned the pointer in an unsigned int (§6.2). Here
 * pages are EfiBootServicesData, contiguous, and capped by max_pa. Return
 * value is a 64-bit virtual address in a one-tuple, matching the (va,) shape
 * callers expect. freephysmem is not in the original 19-name CHIPSEC surface
 * but is required so acceptance and scripts can release memory without leaking
 * for the rest of the boot.
 * ------------------------------------------------------------------------- */

typedef struct {
    EFI_PHYSICAL_ADDRESS  Address;
    UINTN                 Pages;
} edk2_physmem_slot;

static edk2_physmem_slot *edk2_physmem_slots;
static size_t edk2_physmem_slot_count;

static int
edk2_check_bs(void)
{
    if (gBS == NULL || gBS->AllocatePages == NULL || gBS->FreePages == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "UEFI boot services table is not available");
        return -1;
    }
    return 0;
}

static int
edk2_physmem_track(EFI_PHYSICAL_ADDRESS addr, UINTN pages)
{
    edk2_physmem_slot *next;

    next = (edk2_physmem_slot *)PyMem_Realloc(
        edk2_physmem_slots,
        (edk2_physmem_slot_count + 1) * sizeof(edk2_physmem_slot));
    if (next == NULL)
        return -1;
    edk2_physmem_slots = next;
    edk2_physmem_slots[edk2_physmem_slot_count].Address = addr;
    edk2_physmem_slots[edk2_physmem_slot_count].Pages = pages;
    edk2_physmem_slot_count++;
    return 0;
}

static int
edk2_physmem_lookup(EFI_PHYSICAL_ADDRESS addr, UINTN *pages_out)
{
    size_t i;

    for (i = 0; i < edk2_physmem_slot_count; i++) {
        if (edk2_physmem_slots[i].Address == addr) {
            *pages_out = edk2_physmem_slots[i].Pages;
            return (int)i;
        }
    }
    return -1;
}

static void
edk2_physmem_forget(size_t index)
{
    size_t remain;

    if (index >= edk2_physmem_slot_count)
        return;
    remain = edk2_physmem_slot_count - index - 1;
    if (remain > 0) {
        memmove(&edk2_physmem_slots[index],
                &edk2_physmem_slots[index + 1],
                remain * sizeof(edk2_physmem_slot));
    }
    edk2_physmem_slot_count--;
    if (edk2_physmem_slot_count == 0) {
        PyMem_Free(edk2_physmem_slots);
        edk2_physmem_slots = NULL;
    }
}

PyDoc_STRVAR(edk2_allocphysmem__doc__,
"allocphysmem(length, max_pa) -> (va,)\n\
\n\
Allocate length bytes of physically contiguous memory at or below max_pa\n\
(physical address limit). Uses Boot Services AllocatePages. va is the\n\
virtual address of the mapping (64-bit on X64).");

static PyObject *
edk2_allocphysmem(PyObject *self, PyObject *args)
{
    unsigned long long length_in, max_pa_in;
    UINTN length;
    UINTN pages;
    EFI_PHYSICAL_ADDRESS max_addr;
    EFI_STATUS status;

    if (!PyArg_ParseTuple(args, "KK:allocphysmem", &length_in, &max_pa_in))
        return NULL;
    if (edk2_check_bs() < 0)
        return NULL;
    if (length_in == 0) {
        PyErr_SetString(PyExc_ValueError, "length must be greater than zero");
        return NULL;
    }
    length = (UINTN)length_in;
    pages = EFI_SIZE_TO_PAGES(length);
    max_addr = (EFI_PHYSICAL_ADDRESS)max_pa_in;

    status = gBS->AllocatePages(AllocateMaxAddress,
                                EfiBootServicesData,
                                pages,
                                &max_addr);
    if (EFI_ERROR(status)) {
        PyErr_Format(PyExc_OSError,
                     "AllocatePages failed: %u",
                     (unsigned int)status);
        return NULL;
    }
    if (edk2_physmem_track(max_addr, pages) < 0) {
        gBS->FreePages(max_addr, pages);
        return PyErr_NoMemory();
    }

    return Py_BuildValue("(K)", (unsigned long long)max_addr);
}

PyDoc_STRVAR(edk2_freephysmem__doc__,
"freephysmem(va) -> None\n\
\n\
Free memory returned by allocphysmem. va must be exactly the address\n\
returned; other pointers raise ValueError.");

static PyObject *
edk2_freephysmem(PyObject *self, PyObject *args)
{
    unsigned long long va_in;
    EFI_PHYSICAL_ADDRESS addr;
    UINTN pages;
    int slot;
    EFI_STATUS status;

    if (!PyArg_ParseTuple(args, "K:freephysmem", &va_in))
        return NULL;
    if (edk2_check_bs() < 0)
        return NULL;

    addr = (EFI_PHYSICAL_ADDRESS)va_in;
    slot = edk2_physmem_lookup(addr, &pages);
    if (slot < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "va is not an allocphysmem allocation");
        return NULL;
    }

    status = gBS->FreePages(addr, pages);
    if (EFI_ERROR(status)) {
        PyErr_Format(PyExc_OSError,
                     "FreePages failed: %u",
                     (unsigned int)status);
        return NULL;
    }
    edk2_physmem_forget((size_t)slot);

    Py_RETURN_NONE;
}

/* Single-phase init with m_size = -1, following edk2console.c. This build has
 * one interpreter and no subinterpreter support, so per-module state buys
 * nothing over file statics here.
 *
 * Kept alphabetical for reading. Note this is not the order sorted(dir(edk2))
 * produces -- Python compares character by character, so wrmsr sorts after
 * writeio and writepci ('i' < 'm'). Do not "correct" the acceptance lists in
 * Python312_Chipsec_Platform_API_Port.md to match this table. */
static PyMethodDef edk2_methods[] = {
    {"GetNextVariableName", edk2_GetNextVariableName, METH_VARARGS,
     edk2_GetNextVariableName__doc__},
    {"GetVariable",         edk2_GetVariable,         METH_VARARGS,
     edk2_GetVariable__doc__},
    {"SetVariable",         edk2_SetVariable,         METH_VARARGS,
     edk2_SetVariable__doc__},
    {"allocphysmem",   edk2_allocphysmem,   METH_VARARGS, edk2_allocphysmem__doc__},
    {"cpuid",          edk2_cpuid,          METH_VARARGS, edk2_cpuid__doc__},
    {"cpuid_ex",       edk2_cpuid_ex,       METH_VARARGS, edk2_cpuid_ex__doc__},
    {"freephysmem",    edk2_freephysmem,    METH_VARARGS, edk2_freephysmem__doc__},
    {"rdmsr",          edk2_rdmsr,          METH_VARARGS, edk2_rdmsr__doc__},
    {"rdmsr_ex",       edk2_rdmsr_ex,       METH_VARARGS, edk2_rdmsr_ex__doc__},
    {"readio",         edk2_readio,         METH_VARARGS, edk2_readio__doc__},
    {"readmem",        edk2_readmem,        METH_VARARGS, edk2_readmem__doc__},
    {"readmem_dword",  edk2_readmem_dword,  METH_VARARGS, edk2_readmem_dword__doc__},
    {"readpci",        edk2_readpci,        METH_VARARGS, edk2_readpci__doc__},
    {"swsmi",          edk2_swsmi,          METH_VARARGS, edk2_swsmi__doc__},
    {"wrmsr",          edk2_wrmsr,          METH_VARARGS, edk2_wrmsr__doc__},
    {"wrmsr_ex",       edk2_wrmsr_ex,       METH_VARARGS, edk2_wrmsr_ex__doc__},
    {"writeio",        edk2_writeio,        METH_VARARGS, edk2_writeio__doc__},
    {"writemem",       edk2_writemem,       METH_VARARGS, edk2_writemem__doc__},
    {"writemem_dword", edk2_writemem_dword, METH_VARARGS, edk2_writemem_dword__doc__},
    {"writepci",       edk2_writepci,       METH_VARARGS, edk2_writepci__doc__},
    {NULL, NULL}            /* Sentinel */
};

static struct PyModuleDef edk2module = {
    PyModuleDef_HEAD_INIT,
    "edk2",
    module_doc,
    -1,
    edk2_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit_edk2(void)
{
    PyObject *m, *uefi, *fault_error;

    m = PyModule_Create(&edk2module);
    if (m == NULL)
        return NULL;

    /* Reuse uefi.FaultError rather than defining a second exception type, so
     * that `except uefi.FaultError` catches faults from either module and
     * `edk2.FaultError is uefi.FaultError` holds. One type, no hierarchy.
     *
     * `uefi` is a builtin that the interpreter loads during startup, so by the
     * time anything can import edk2 this is a sys.modules hit rather than work.
     *
     * Failing the import if it is missing is deliberate. FaultError absent
     * means the fault-recovery infrastructure that this module's memory APIs
     * depend on is not in the image, and an import that raises is far easier to
     * diagnose than memory APIs that stop the machine instead of reporting. */
    uefi = PyImport_ImportModule("uefi");
    if (uefi == NULL)
        goto error;

    fault_error = PyObject_GetAttrString(uefi, "FaultError");
    Py_DECREF(uefi);
    if (fault_error == NULL)
        goto error;

    /* AddObjectRef does not steal, so the local reference is still ours. */
    if (PyModule_AddObjectRef(m, "FaultError", fault_error) < 0) {
        Py_DECREF(fault_error);
        goto error;
    }

    /* Hand that reference to the file static instead of releasing it, so the
     * memory APIs can raise without a dictionary lookup per call and without
     * depending on the module attribute still being what we set — rebinding
     * edk2.FaultError from Python must not change what a fault raises. The
     * module is never unloaded, so holding it for the life of the image is the
     * intended lifetime rather than a leak. */
    edk2_fault_error = fault_error;

    return m;

error:
    Py_DECREF(m);
    return NULL;
}
