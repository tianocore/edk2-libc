/* AppPkg monolithic Python312.inf: .S preprocess uses PP_RESP (application
   cc flags). Host GCC may define __CET__ / expand _CET_ENDBR to opcodes the
   EDK assembler step rejects. edk2-py312 builds the same sources in standalone
   LibFFI.inf without PP_RESP. Include this last from unix64.S / win64.S. */
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
