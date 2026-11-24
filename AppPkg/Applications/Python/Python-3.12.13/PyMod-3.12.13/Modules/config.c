/** @file
    Python Module configuration.

    Copyright (c) 2011-2026, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    https://opensource.org/license/bsd-2-clause-patent.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

/* -*- C -*- ***********************************************
Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
******************************************************************/

/* Module configuration */

/* This file contains the table of built-in modules.
   See create_builtin() in import.c. */

#include "Python.h"

#ifdef __cplusplus
extern "C" {
#endif

extern PyObject* PyInit_uefi(void);
extern PyObject* PyInit_atexit(void);
extern PyObject* PyInit_faulthandler(void);
extern PyObject* PyInit_uefi(void);
extern PyObject* PyInit__signal(void);
extern PyObject* PyInit__tracemalloc(void);
extern PyObject* PyInit__codecs(void);
extern PyObject* PyInit__collections(void);
extern PyObject* PyInit_errno(void);
extern PyObject* PyInit__io(void);
extern PyObject* PyInit_itertools(void);
extern PyObject* PyInit__sre(void);
extern PyObject* PyInit__thread(void);
extern PyObject* PyInit_time(void);
extern PyObject* PyInit__weakref(void);
extern PyObject* PyInit__abc(void);
extern PyObject* PyInit__functools(void);
extern PyObject* PyInit__locale(void);
extern PyObject* PyInit__operator(void);
extern PyObject* PyInit__stat(void);
extern PyObject* PyInit__symtable(void);
extern PyObject* PyInit_pwd(void);

/* -- ADDMODULE MARKER 1 -- */

extern PyObject* PyMarshal_Init(void);
extern PyObject* PyInit__imp(void);
extern PyObject* PyInit_gc(void);
extern PyObject* PyInit__ast(void);
extern PyObject* PyInit__tokenize(void);
extern PyObject* _PyWarnings_Init(void);
extern PyObject* PyInit__string(void);

#if UEFI_C_SOURCE
/* Modules that normally are dynamically loadable */
  extern PyObject* PyInit__struct(void);
  extern PyObject* PyInit_array(void);
  extern PyObject* PyInit__contextvars(void);
  extern PyObject* PyInit_math(void);
  extern PyObject* PyInit_cmath(void);
  extern PyObject* PyInit__elementtree(void);
  extern PyObject* PyInit__datetime(void);
  extern PyObject* PyInit__random(void);
  extern PyObject* PyInit__bisect(void);
  extern PyObject* PyInit__heapq(void);
  extern PyObject* PyInit__lsprof(void);
  extern PyObject* PyInit_unicodedata(void);
  extern PyObject* PyInit__opcode(void);
  extern PyObject* PyInit__asyncio(void);
  extern PyObject* PyInit__queue(void);
  extern PyObject* PyInit_select(void);
  extern PyObject* PyInit__xxsubinterpreters(void);
  extern PyObject* PyInit_audioop(void);
  extern PyObject* PyInit__csv(void);
  extern PyObject* PyInit__socket(void);
  //  extern PyObject* PyInit__sha256(void);
  //  extern PyObject* PyInit__sha512(void);
  extern PyObject* PyInit__md5(void);
  extern PyObject* PyInit__sha1(void);
  extern PyObject* PyInit__blake2(void);
  extern PyObject* PyInit__sha3(void);
  extern PyObject* PyInit_binascii(void);
  extern PyObject* PyInit__multibytecodec(void);
  extern PyObject* PyInit__decimal(void);
  extern PyObject* PyInit_xxlimited(void);
#if defined(BUILD_PYTHON312_FULL)
  extern PyObject* PyInit_zlib(void);
  extern PyObject* PyInit__ctypes(void);
  extern PyObject* PyInit__hashlib(void);
  extern PyObject* PyInit__ssl(void);
  /* Platform primitives for firmware tooling; PyMod-3.12.13/Modules/edk2module.c.
   * FULL only — that file is not in Python312_MIN.inf's [Sources], so the gate
   * is what keeps the MIN link closed. */
  extern PyObject* PyInit_edk2(void);
#endif
#ifndef UEFI_C_SOURCE
  extern PyObject* PyInit__ctypes_test(void);
#endif
  extern PyObject* PyInit__posixsubprocess(void);
  extern PyObject* PyInit_edk2console(void);
  extern PyObject* PyInit_pyexpat(void);
  extern PyObject* PyInit__typing(void);
  extern PyObject* PyInit__json(void);
  extern PyObject* PyInit__multiprocessing(void);
  extern PyObject* PyInit__zoneinfo(void);
  extern PyObject* PyInit__pickle(void);
  extern PyObject* PyInit__statistics(void);
  extern PyObject* PyInit__sha2(void);
  extern PyObject* PyInit__xxinterpchannels(void);
  extern PyObject* PyInit_mmap(void);
  extern PyObject* PyInit_termios(void);
#endif
   
struct _inittab _PyImport_Inittab[] = {
    {"atexit", PyInit_atexit},
    {"faulthandler", PyInit_faulthandler},
    {"uefi", PyInit_uefi},
    {"_signal", PyInit__signal},
    {"_tracemalloc", PyInit__tracemalloc},
    {"_codecs", PyInit__codecs},
    {"_collections", PyInit__collections},
    {"errno", PyInit_errno},
    {"_io", PyInit__io},
    {"itertools", PyInit_itertools},
    {"_sre", PyInit__sre},
    {"_thread", PyInit__thread},
    {"time", PyInit_time},
    {"_weakref", PyInit__weakref},
    {"_abc", PyInit__abc},
    {"_functools", PyInit__functools},
    {"_locale", PyInit__locale},
    {"_operator", PyInit__operator},
    {"_stat", PyInit__stat},
    {"_symtable", PyInit__symtable},
    {"pwd", PyInit_pwd},

/* -- ADDMODULE MARKER 2 -- */
/* -- ADDMODULE MARKER 2 -- */

    /* This module lives in marshal.c */
    {"marshal", PyMarshal_Init},

    /* This lives in import.c */
    {"_imp", PyInit__imp},

    /* This lives in Python/Python-ast.c */
    {"_ast", PyInit__ast},

    /* This lives in Python/Python-tokenizer.c */
    {"_tokenize", PyInit__tokenize},

    /* These entries are here for sys.builtin_module_names */
    {"builtins", NULL},
    {"sys", NULL},

    /* This lives in gcmodule.c */
    {"gc", PyInit_gc},

    /* This lives in _warnings.c */
    {"_warnings", _PyWarnings_Init},

    /* This lives in Objects/unicodeobject.c */
    {"_string", PyInit__string},

#if UEFI_C_SOURCE
    {"_struct", PyInit__struct},
    {"array", PyInit_array},
    {"_contextvars", PyInit__contextvars},
    {"math", PyInit_math},
    {"cmath", PyInit_cmath},
    {"_elementtree", PyInit__elementtree},
    {"_datetime", PyInit__datetime},
    {"_random", PyInit__random},
    {"_bisect", PyInit__bisect},
    {"_heapq", PyInit__heapq},
    {"_lsprof", PyInit__lsprof},
    {"unicodedata", PyInit_unicodedata},
    {"_opcode", PyInit__opcode},
    {"_asyncio", PyInit__asyncio},
    {"_queue", PyInit__queue},
    {"select", PyInit_select},
    {"_xxsubinterpreters", PyInit__xxsubinterpreters},
    {"_xxinterpchannels", PyInit__xxinterpchannels},
    {"audioop", PyInit_audioop},
    {"_csv", PyInit__csv},
    {"_socket", PyInit__socket},
    //    {"_sha256", PyInit__sha256},
    //    {"_sha512", PyInit__sha512},
    {"_md5", PyInit__md5},
    {"_sha1", PyInit__sha1},
    {"_blake2", PyInit__blake2},
    {"_sha3", PyInit__sha3},
    {"binascii", PyInit_binascii},
    {"_multibytecodec", PyInit__multibytecodec},
    {"_decimal", PyInit__decimal},
    {"xxlimited", PyInit_xxlimited},
#if defined(BUILD_PYTHON312_FULL)
    {"zlib", PyInit_zlib},
    {"_ctypes", PyInit__ctypes},
#ifndef UEFI_C_SOURCE
    {"_ctypes_test", PyInit__ctypes_test},
#endif
    {"_hashlib", PyInit__hashlib},
    {"_ssl", PyInit__ssl},
    {"edk2", PyInit_edk2},
#endif
    {"_posixsubprocess", PyInit__posixsubprocess},
    {"edk2console", PyInit_edk2console},
    {"pyexpat", PyInit_pyexpat},
    {"_typing", PyInit__typing},
    {"_json", PyInit__json},
    {"_multiprocessing", PyInit__multiprocessing},
    {"_zoneinfo", PyInit__zoneinfo},
    {"_pickle", PyInit__pickle},
    {"_statistics", PyInit__statistics},
    {"_sha2", PyInit__sha2},
    {"mmap", PyInit_mmap},
    {"termios", PyInit_termios},
#endif    
    /* Sentinel */
    {0, 0}
};


#ifdef __cplusplus
}
#endif
