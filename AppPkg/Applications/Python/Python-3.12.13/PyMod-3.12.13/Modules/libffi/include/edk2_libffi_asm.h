/** @file
    Neutralize CET macros for monolithic Python312.inf .S preprocess (PP_RESP).
    Host GCC may expand _CET_ENDBR to opcodes the EDK assembler rejects.
    Include this last from unix64.S / win64.S.

    Copyright (c) 2011-2026, Intel Corporation. All rights reserved.<BR>
    SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef EDK2_LIBFFI_ASM_H
#define EDK2_LIBFFI_ASM_H
#ifdef __CET__
#undef __CET__
#endif
#undef _CET_ENDBR
#define _CET_ENDBR
#undef _CET_NOTRACK
#define _CET_NOTRACK
#endif
