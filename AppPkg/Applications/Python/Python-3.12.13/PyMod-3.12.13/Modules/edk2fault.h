/** @file
    UEFI guarded-access / FaultError helpers.

    Copyright (c) 2011-2026, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    https://opensource.org/license/bsd-2-clause-patent.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

/* edk2fault.h — turning a recovered CPU fault into a Python exception.
 *
 * The guarded accesses themselves live in efi/src/edk2excep.c and contain no
 * Python at all. This is the other half: the one place that knows what a
 * FaultError instance looks like, so that uefi.mem_read and the edk2 module's
 * memory APIs raise the same thing with the same attributes rather than two
 * near-identical exceptions that drift.
 *
 * It is defined in posixmodule.c rather than in edk2module.c, and that is not
 * arbitrary. MIN builds compile posixmodule.c and do NOT compile edk2module.c,
 * but MIN still has uefi.mem_read and so still needs this function; putting it
 * the other way round would leave MIN with an unresolved symbol. The dependency
 * has to point this way for the same reason: `uefi` is the os module and is
 * imported during interpreter startup, so it must not depend on a module that
 * may not be in the image.
 */

#ifndef EDK2FAULT_H
#define EDK2FAULT_H

/* Include this after Python.h, not before: a translation unit that needs
 * PY_SSIZE_T_CLEAN must define it ahead of the first Python.h, and it cannot do
 * that if this header pulled Python.h in first. */
#include "Python.h"
#include <efi/edk2excep.h>

/* Set a FaultError of the given type as the current exception, with vector,
 * rip, cr2 and error_code attached as integer attributes. exc_type is passed in
 * rather than looked up because callers outside posixmodule.c have no posix
 * module state to find it in; both modules pass the same object, so
 * `edk2.FaultError is uefi.FaultError` holds.
 *
 * kind and ctx are the values edk2_guarded_access() or edk2_guarded_copy()
 * filled in when they returned 1. Silently does nothing on allocation failure,
 * in which case the pending MemoryError stands instead. */
extern void
uefi_set_fault_error(PyObject *exc_type, uint64_t kind,
                     const EFI_SYSTEM_CONTEXT_X64 *ctx);

#endif /* EDK2FAULT_H */
