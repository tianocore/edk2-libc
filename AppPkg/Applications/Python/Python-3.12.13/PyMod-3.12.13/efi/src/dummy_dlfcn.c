/** @file
    UEFI Python support: dummy_dlfcn.

    Copyright (c) 2011-2026, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    https://opensource.org/license/bsd-2-clause-patent.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <stdlib.h>
#include <errno.h>

#include <dlfcn.h>

#include "Python.h"

static char error[100] = {0};

void*
dlsym(void *handle, const char *symbol)
{
   PyObject* mapping = (PyObject*)handle;

   if( !mapping ) {
      snprintf(error, sizeof(error),
               "Null mapping object");
      return NULL;            
   }
   
   if( !PyMapping_Check(mapping) ) {
      snprintf(error, sizeof(error),
               "Received __dlsyms__ which is not map");
      return NULL;            
   }

   PyObject* value_obj = PyMapping_GetItemString(mapping, symbol);
   if( !value_obj ) {
      snprintf(error, sizeof(error),
               "Symbol %s is not found", symbol);
      return NULL;            
   }

   return PyLong_AsVoidPtr(value_obj);
}


char *dlerror(void)
{
   return error;
}

void*
dlopen(const char *filename, int flags)
{
   if( filename == NULL )
      return NULL;

   PyObject* module = PyImport_ImportModule(filename);
   if( module == NULL ) {
      snprintf(error, sizeof(error), "Unable to import module: %s", filename);
      return NULL;
   }

   PyObject* dlsyms_name = PyUnicode_FromString("__dlsyms__");
   PyObject* dlsyms = PyObject_GetAttr(module, dlsyms_name);
   if( dlsyms == NULL ) {
      snprintf(error, sizeof(error),
               "Module %s does not have __dlsyms__", filename);
      return NULL;
   }
   if( !PyMapping_Check(dlsyms) ) {
      snprintf(error, sizeof(error),
               "Module %s does have __dlsyms__ which is not map", filename);
      return NULL;      
   }
   return dlsyms;
}

int dlclose(void *handle)
{
   return 0;
}

int dladdr(void *addr, Dl_info *info)
{
   info->dli_sname = NULL;
   info->dli_saddr = NULL;
   return 0;
}


