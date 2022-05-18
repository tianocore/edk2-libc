/** @file
    Python Module configuration.

    Copyright (c) 2011-2021, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

/* This file contains the table of built-in modules.
   See init_builtin() in import.c. */

#include "Python.h"

extern PyObject* PyInit_array(void);
extern PyObject* PyInit__ast(void);
extern PyObject* PyInit_binascii(void);
extern PyObject* init_bisect(void);
extern PyObject* PyInit_cmath(void);
extern PyObject* PyInit__codecs(void);
extern PyObject* PyInit__collections(void);
extern PyObject* PyInit__pickle(void);
extern PyObject* PyInit__csv(void);
extern PyObject* init_ctypes(void);
extern PyObject* PyInit__datetime(void);
extern PyObject* PyEdk2__Init(void);
extern PyObject* PyInit_errno(void);
extern PyObject* PyInit__functools(void);
extern PyObject* initfuture_builtins(void);
extern PyObject* PyInit_gc(void);
extern PyObject* init_heapq(void);
extern PyObject* init_hotshot(void);
extern PyObject* PyInit_imp(void);
extern PyObject* PyInit__io(void);
extern PyObject* PyInit_itertools(void);
extern PyObject* PyInit__json(void);
extern PyObject* init_lsprof(void);
extern PyObject* PyInit_math(void);
extern PyObject* PyInit__md5(void);
extern PyObject* initmmap(void);
extern PyObject* PyInit__operator(void);
extern PyObject* PyInit_parser(void);
extern PyObject* PyInit_pyexpat(void);
extern PyObject* PyInit__random(void);
extern PyObject* PyInit_select(void);
extern PyObject* PyInit__sha1(void);
extern PyObject* PyInit__sha256(void);
extern PyObject* PyInit__sha512(void);
extern PyObject* PyInit__sha3(void);
extern PyObject* PyInit__blake2(void);
extern PyObject* PyInit__signal(void);
extern PyObject* PyInit__socket(void);
extern PyObject* PyInit__sre(void);
extern PyObject* PyInit__struct(void);
extern PyObject* init_subprocess(void);
extern PyObject* PyInit__symtable(void);
extern PyObject* initthread(void);
extern PyObject* PyInit_time(void);
extern PyObject* PyInit_unicodedata(void);
extern PyObject* PyInit__weakref(void);
extern PyObject* init_winreg(void);
extern PyObject* PyInit_zlib(void);
extern PyObject* initbz2(void);
extern PyObject* PyInit_atexit(void);

extern PyObject* PyMarshal_Init(void);
extern PyObject* _PyWarnings_Init(void);

extern PyObject* PyInit__multibytecodec(void);
extern PyObject* PyInit__codecs_cn(void);
extern PyObject* PyInit__codecs_hk(void);
extern PyObject* PyInit__codecs_iso2022(void);
extern PyObject* PyInit__codecs_jp(void);
extern PyObject* PyInit__codecs_kr(void);
extern PyObject* PyInit__codecs_tw(void);

extern PyObject* PyInit__string(void);
extern PyObject* PyInit__stat(void);
extern PyObject* PyInit__opcode(void);
extern PyObject* PyInit_faulthandler(void);
// _ctypes
#if defined(UEFI_MSVC_32) || defined(UEFI_MSVC_64)
extern PyObject* PyInit__ctypes(void);
#endif

extern PyObject* init_sqlite3(void);

// EfiPy
extern PyObject* init_EfiPy(void);

// ssl
extern PyObject* PyInit__ssl(void);

struct _inittab _PyImport_Inittab[] = {
    {"_ast", PyInit__ast},
    {"_csv", PyInit__csv},
    {"_io", PyInit__io},
    {"_json", PyInit__json},
    {"_md5", PyInit__md5},
    {"_sha1", PyInit__sha1},
    {"_sha256", PyInit__sha256},
    {"_sha512", PyInit__sha512},
	{ "_sha3", PyInit__sha3 },
	{ "_blake2", PyInit__blake2 },
//    {"_socket", PyInit__socket},
    {"_symtable", PyInit__symtable},
	{"binascii", PyInit_binascii},
	{"cmath", PyInit_cmath},
	{"errno", PyInit_errno},
	{"faulthandler", PyInit_faulthandler},
	{"gc", PyInit_gc},
	{"math", PyInit_math},
    {"array", PyInit_array},
    {"atexit", PyInit_atexit},
    {"_datetime", PyInit__datetime},
    {"parser", PyInit_parser},
    {"pyexpat", PyInit_pyexpat},
    {"select", PyInit_select},
    {"_signal", PyInit__signal},
    {"unicodedata", PyInit_unicodedata},
	{ "zlib", PyInit_zlib },

    /* CJK codecs */
    {"_multibytecodec", PyInit__multibytecodec},

#ifdef WITH_THREAD
    {"thread", initthread},
#endif

    /* These modules are required for the full built-in help() facility provided by pydoc. */
    {"_codecs", PyInit__codecs},
    {"_collections", PyInit__collections},
    {"_functools", PyInit__functools},
    {"_random", PyInit__random},
    {"_sre", PyInit__sre},
    {"_struct", PyInit__struct},
    {"_weakref", PyInit__weakref},
    {"itertools", PyInit_itertools},
    {"_operator", PyInit__operator},
    {"time", PyInit_time},

    /* These four modules should always be built in. */
    {"edk2", PyEdk2__Init},
    {"_imp", PyInit_imp},
    {"marshal", PyMarshal_Init},

    /* These entries are here for sys.builtin_module_names */
    {"__main__", NULL},
    {"__builtin__", NULL},
    {"builtins", NULL},
    {"sys", NULL},
    {"exceptions", NULL},
    {"_warnings", _PyWarnings_Init},
    {"_string", PyInit__string},
    {"_stat", PyInit__stat},
    {"_opcode", PyInit__opcode},
#if defined(UEFI_MSVC_32) || defined(UEFI_MSVC_64)
    { "_ctypes", PyInit__ctypes },
#endif
    /* Sentinel */
    {0, 0}
};
