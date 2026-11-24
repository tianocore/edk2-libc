/** @file
    UEFI console / readline glue.

    Copyright (c) 2011-2026, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    https://opensource.org/license/bsd-2-clause-patent.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <setjmp.h>

#include "Python.h"

#include  <Uefi.h>
#include  <Library/UefiLib.h>

#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleTextInEx.h>
#include <Protocol/SimpleTextOut.h>
#include <Protocol/Cpu.h>
#include <Protocol/Shell.h>
#include <Protocol/Rng.h>

#include "efi/edk2main.h"

#ifdef PY_UEFI_BOOT_TRACE
#define PY312_CONSOLE_TRACE(Step) Print(L"Python312 boot: " Step L"\n")
#else
#define PY312_CONSOLE_TRACE(Step) ((void)0)
#endif

extern char *(*PyOS_ReadlineFunctionPointer)(FILE *, FILE *, const char *);
extern int (*PyOS_InputHook)(void);
static PyObject *py_console_readline_hook = NULL;

static EFI_EVENT g_console_timer = NULL;
static uint64_t g_console_timer_counter = 0;


static
VOID EFIAPI
incr_console_timer(IN EFI_EVENT Event, IN  VOID *Context)
{
   g_console_timer_counter++;
}

static EFI_STATUS start_console_timer(void)
{
   EFI_STATUS status;

   if (g_console_timer != NULL) {
      return EFI_SUCCESS;
   }

   status = g_edk2_globals.system_table->BootServices->CreateEvent(
      EVT_TIMER | EVT_NOTIFY_SIGNAL,
      TPL_CALLBACK,
      incr_console_timer,
      NULL,
      &g_console_timer
   );

   status = g_edk2_globals.system_table->BootServices->SetTimer(
      g_console_timer,
      TimerPeriodic,
      EFI_TIMER_PERIOD_MILLISECONDS (1)
   );

   return status;
}

static PyObject *
py_console_get_timer(PyObject *self)
{
   return PyLong_FromUnsignedLongLong(g_console_timer_counter);   
}

static void
edk2_console_stop_timer(void)
{
   EFI_BOOT_SERVICES *bs;

   if (g_console_timer == NULL) {
      PY312_CONSOLE_TRACE(L"stop_timer: already off");
      return;
   }

   if (g_edk2_globals.system_table == NULL) {
      g_console_timer = NULL;
      PY312_CONSOLE_TRACE(L"stop_timer: dropped (no systab)");
      return;
   }

   bs = g_edk2_globals.system_table->BootServices;
   PY312_CONSOLE_TRACE(L"stop_timer: TimerCancel");
   bs->SetTimer(
      g_console_timer,
      TimerCancel,
      0
   );

   PY312_CONSOLE_TRACE(L"stop_timer: CloseEvent");
   bs->CloseEvent(g_console_timer);
   g_console_timer = NULL;
   PY312_CONSOLE_TRACE(L"stop_timer: done");
}

static void
edk2_console_drain_input(void)
{
   EFI_KEY_DATA key;

   if (g_edk2_globals.console_in == NULL) {
      return;
   }

   while (g_edk2_globals.console_in->ReadKeyStrokeEx(
             g_edk2_globals.console_in,
             &key
          ) == EFI_SUCCESS) {
   }
}

static EFI_STATUS
edk2_console_ensure_input(void)
{
   EFI_STATUS status;

   if (g_edk2_globals.console_in != NULL) {
      return EFI_SUCCESS;
   }
   if (g_edk2_globals.system_table == NULL ||
       g_edk2_globals.image_handle == NULL) {
      return EFI_NOT_READY;
   }

   status = g_edk2_globals.system_table->BootServices->OpenProtocol(
      g_edk2_globals.system_table->ConsoleInHandle,
      &gEfiSimpleTextInputExProtocolGuid,
      (VOID **)&g_edk2_globals.console_in,
      g_edk2_globals.image_handle,
      NULL,
      EFI_OPEN_PROTOCOL_GET_PROTOCOL
   );
   return status;
}

void
edk2_console_detach_readline(void)
{
   PY312_CONSOLE_TRACE(L"edk2_console_detach_readline enter");
   edk2_console_stop_timer();
   py_console_readline_hook = NULL;
   PyOS_ReadlineFunctionPointer = NULL;
   PyOS_InputHook = NULL;
   edk2_console_drain_input();
   if (g_edk2_globals.console_in != NULL &&
       g_edk2_globals.system_table != NULL &&
       g_edk2_globals.image_handle != NULL) {
      g_edk2_globals.system_table->BootServices->CloseProtocol(
         g_edk2_globals.system_table->ConsoleInHandle,
         &gEfiSimpleTextInputExProtocolGuid,
         g_edk2_globals.image_handle,
         NULL
      );
      g_edk2_globals.console_in = NULL;
   }
   PY312_CONSOLE_TRACE(L"edk2_console_detach_readline leave");
}

void
edk2_console_prepare_for_launch(void)
{
   edk2_console_detach_readline();
   edk2_console_drain_input();
   /* Do not Reset ConIn here; Reset(TRUE) hangs after REPL exit and
    * Reset(FALSE) has hung the next Shell launch on VS2022. Drain only. */
}

static void
edk2_console_restore_firmware_console(void)
{
   edk2_console_drain_input();
}

void
edk2_console_restore_for_shell(void)
{
   edk2_console_restore_firmware_console();
}

void
edk2_console_handoff_to_shell(void)
{
   PY312_CONSOLE_TRACE(L"edk2_console_handoff_to_shell enter");
   edk2_console_detach_readline();
   edk2_console_drain_input();
   PY312_CONSOLE_TRACE(L"edk2_console_handoff_to_shell leave");
}

static void
py_console_free(void *user_data)
{
   edk2_console_detach_readline();
}

static PyObject *
py_console_input_reset(PyObject *self)
{
   EFI_STATUS status;

   status = g_edk2_globals.console_in->Reset(
      g_edk2_globals.console_in,
      1
   );

   return PyLong_FromUnsignedLong(status);   
}

static PyObject *
py_console_getkeys(PyObject *self, PyObject *args, PyObject *kwargs)
{
   static char *kwlist[] = {"wait", "timeout", NULL};
   EFI_STATUS status;

   int wait = 1;
   uint64_t timeout = 0;

   UINTN events_num = 1;
   UINTN event_index;
   EFI_EVENT events[2] = { NULL, NULL };
   
   if (!PyArg_ParseTupleAndKeywords(args, kwargs, "iK", kwlist, &wait, &timeout))
      return NULL;

   status = edk2_console_ensure_input();
   if (EFI_ERROR(status)) {
      PyErr_Format(PyExc_SystemError,
                   "EDK2 input console unavailable (EFI status %x)", status);
      return NULL;
   }

   if( g_edk2_globals.console_in == NULL ) {
      PyErr_Format(PyExc_SystemError, "EDK2 input console is closed. "
                                      "You are running on EFIv1 that is "
                                      "not yet supported");
      return NULL;
   }

   events[0] = g_edk2_globals.console_in->WaitForKeyEx;
   
   if(wait) {      
      if( timeout > 0 ) {
         status = g_edk2_globals.system_table->BootServices->CreateEvent(
            EVT_TIMER,
            TPL_APPLICATION,
            NULL,
            NULL,
            events+1
         );
         
         if(status != EFI_SUCCESS) {
            PyErr_Format(PyExc_SystemError,
                         "Unable to create timeout event. EFI Status: %x ",
                         status);
            return NULL;            
         }

         status = g_edk2_globals.system_table->BootServices->SetTimer(
            events[1],
            TimerRelative,
            timeout
         );

         if(status != EFI_SUCCESS) {
            PyErr_Format(PyExc_SystemError,
                         "Unable to set timer. EFI Status: %x ",
                         status);
            g_edk2_globals.system_table->BootServices->CloseEvent(events[1]);
            return NULL;            
         }
         
         events_num += 1;
      }
               
   }
   
   PyObject* keys = PyList_New(0);

   do {
      EFI_KEY_DATA key;

      if(wait) {
         g_edk2_globals.system_table->BootServices->WaitForEvent (
            events_num,
            events,
            &event_index
         );

         if(event_index > 0)
            break;
      }
         
      status = g_edk2_globals.console_in->ReadKeyStrokeEx(
         g_edk2_globals.console_in,
         &key
      );

      if( status == EFI_SUCCESS ) {
         PyObject* keystroke = PyDict_New();
         PyDict_SetItemString(
            keystroke, "scan_code",
            PyLong_FromUnsignedLong(key.Key.ScanCode)
         );
         PyDict_SetItemString(
            keystroke, "unicode_char",
            PyLong_FromLong(key.Key.UnicodeChar)
         );
         PyDict_SetItemString(
            keystroke, "shift_state",
            PyLong_FromUnsignedLong(key.KeyState.KeyShiftState)
         );
         PyDict_SetItemString(
            keystroke, "toggle_state",
            PyLong_FromUnsignedLong(key.KeyState.KeyToggleState)
         );

         PyList_Append(keys, keystroke);
      }      
   } while(status == EFI_SUCCESS);

   if(events_num > 1) 
      g_edk2_globals.system_table->BootServices->CloseEvent(events[1]);
   
   return keys;
}

static PyObject *
py_console_output_reset(PyObject *self)
{
   EFI_STATUS status;

   status = g_edk2_globals.system_table->ConOut->Reset(
      g_edk2_globals.system_table->ConOut,
      1
   );

   return PyLong_FromUnsignedLong(status);   
}

static PyObject *
py_console_puts(PyObject *self, PyObject *args)
{
   EFI_STATUS status;

   PyObject *str = NULL;
   
   if (!PyArg_ParseTuple(args, "U", &str))
      return NULL;

   status = g_edk2_globals.system_table->ConOut->OutputString(
      g_edk2_globals.system_table->ConOut,
      PyUnicode_AsWideCharString(str, NULL)
   );

   
   return PyLong_FromUnsignedLong(status);
}

static PyObject *
py_console_get_output_mode(PyObject *self)
{
   return PyLong_FromUnsignedLong(
      g_edk2_globals.system_table->ConOut->Mode->Mode
   );   
}

static PyObject *
py_console_get_output_mode_ex(PyObject *self)
{
   PyObject* mode = PyDict_New();
   PyDict_SetItemString(
      mode, "max_mode",
      PyLong_FromLong(g_edk2_globals.system_table->ConOut->Mode->MaxMode)
   );
   PyDict_SetItemString(
      mode, "mode",
      PyLong_FromLong(g_edk2_globals.system_table->ConOut->Mode->Mode)
   );
   PyDict_SetItemString(
      mode, "attr",
      PyLong_FromLong(g_edk2_globals.system_table->ConOut->Mode->Attribute)
   );
   PyDict_SetItemString(
      mode, "cursor_column",
      PyLong_FromLong(g_edk2_globals.system_table->ConOut->Mode->CursorColumn)
   );
   PyDict_SetItemString(
      mode, "cursor_row",
      PyLong_FromLong(g_edk2_globals.system_table->ConOut->Mode->CursorRow)
   );
   PyDict_SetItemString(
      mode, "cursor_visible",
      PyLong_FromLong(g_edk2_globals.system_table->ConOut->Mode->CursorVisible)
   );
   
   return mode;
}

static PyObject *
py_console_get_output_size(PyObject *self, PyObject *args)
{
   EFI_STATUS status;

   int mode;
   UINTN cols = 0;
   UINTN rows = 0;
   
   if (!PyArg_ParseTuple(args, "i", &mode))
      return NULL;

   status = g_edk2_globals.system_table->ConOut->QueryMode(
      g_edk2_globals.system_table->ConOut,
      (UINTN)mode,
      &cols,
      &rows
   );

   PyObject* res = PyTuple_New(3);
   PyTuple_SetItem(res, 0, PyLong_FromUnsignedLong(status));
   PyTuple_SetItem(res, 1, PyLong_FromUnsignedLong(cols));
   PyTuple_SetItem(res, 2, PyLong_FromUnsignedLong(rows));
   return res;
}

static PyObject *
py_console_set_output_mode(PyObject *self, PyObject *args)
{
   EFI_STATUS status;

   int mode;
   
   if (!PyArg_ParseTuple(args, "i", &mode))
      return NULL;

   status = g_edk2_globals.system_table->ConOut->SetMode(
      g_edk2_globals.system_table->ConOut,
      (UINTN)mode
   );

   return PyLong_FromUnsignedLong(status);
}

static PyObject *
py_console_set_output_attr(PyObject *self, PyObject *args)
{
   EFI_STATUS status;

   int attr;
   
   if (!PyArg_ParseTuple(args, "i", &attr))
      return NULL;

   status = g_edk2_globals.system_table->ConOut->SetAttribute(
      g_edk2_globals.system_table->ConOut,
      (UINTN)attr
   );

   return PyLong_FromUnsignedLong(status);
}

static PyObject *
py_console_clear_screen(PyObject *self)
{
   EFI_STATUS status;

   status = g_edk2_globals.system_table->ConOut->ClearScreen(
      g_edk2_globals.system_table->ConOut
   );

   return PyLong_FromUnsignedLong(status);   
}

static PyObject *
py_console_get_cursor_pos(PyObject *self)
{
   PyObject* res = PyTuple_New(2);
   PyTuple_SetItem(res, 0,
                   PyLong_FromUnsignedLong(
                      g_edk2_globals.system_table->ConOut->Mode->CursorColumn
                   )
   );
   PyTuple_SetItem(res, 1,
                   PyLong_FromUnsignedLong(
                      g_edk2_globals.system_table->ConOut->Mode->CursorRow
                   )
   );
   return res;
}

static PyObject *
py_console_set_cursor_pos(PyObject *self, PyObject *args)
{
   EFI_STATUS status;

   int col, row;
   
   if (!PyArg_ParseTuple(args, "ii", &col, &row))
      return NULL;

   status = g_edk2_globals.system_table->ConOut->SetCursorPosition(
      g_edk2_globals.system_table->ConOut,
      (UINTN)col,
      (UINTN)row
   );

   return PyLong_FromUnsignedLong(status);
}

static PyObject *
py_console_set_cursor_visibility(PyObject *self, PyObject *args)
{
   EFI_STATUS status;

   int visible;
   
   if (!PyArg_ParseTuple(args, "i", &visible))
      return NULL;

   status = g_edk2_globals.system_table->ConOut->EnableCursor(
      g_edk2_globals.system_table->ConOut,
      visible
   );

   return PyLong_FromUnsignedLong(status);
}

static
char *py_console_readline_callback(FILE *stdin_f, FILE *stdout_f,
                                   const char *prompt)   
{
   if(py_console_readline_hook == NULL)
      return NULL;

   PyGILState_STATE gstate;
   gstate = PyGILState_Ensure();
   
   PyObject *py_stdin = PyFile_FromFd(fileno(stdin_f),
                                      "<readline stdin>", "r",
                                      -1, NULL, NULL, NULL, 0);
   PyObject *py_stdout = PyFile_FromFd(fileno(stdout_f),
                                       "<readline stdout>", "w",
                                       -1, NULL, NULL, NULL, 0);
   PyObject *py_prompt = PyUnicode_FromString(prompt);

   PyObject *args = PyTuple_New(3);
   PyTuple_SetItem(args, 0, py_stdin);
   PyTuple_SetItem(args, 1, py_stdout);
   PyTuple_SetItem(args, 2, py_prompt);

   PyObject* result = PyObject_CallObject(py_console_readline_hook, args);

   Py_DECREF(args);

   char *res = NULL;
   Py_ssize_t size = 0;   
   
   if(result != NULL) {
      if(PyUnicode_Check(result)) {
         const char *utf8_result = PyUnicode_AsUTF8AndSize(result, &size);
         
         res = PyMem_RawMalloc(size+1);
         if(res != NULL) {
            memcpy(res, utf8_result, size);
            res[size] = 0;
         }
      } else {
         PyObject *data = PyByteArray_FromObject(result);
         
         size = PyByteArray_Size(data);
         res = PyMem_RawMalloc(size+1);
         if(res != NULL) {
            memcpy(res, PyByteArray_AsString(data), size);
            res[size] = 0;
         }         
      }
      Py_DECREF(result);
   }
   
   PyGILState_Release(gstate);
   
   return res;
}

static PyObject *
py_console_install_readline_hook(PyObject *self, PyObject *args)
{
   PyObject *hook = NULL;
   
   if (!PyArg_ParseTuple(args, "O", &hook))
      return NULL;

   py_console_readline_hook = hook;
   PyOS_ReadlineFunctionPointer = py_console_readline_callback;

   /* Do not start the 1 ms BootServices periodic timer: if it outlives the
    * app (unload race), it can hang the next Shell command or Shell exit. */
   
   return PyLong_FromLong(1);
}

static PyObject *
py_console_shutdown_interactive(PyObject *self)
{
   edk2_console_handoff_to_shell();
   Py_RETURN_NONE;
}

static PyMethodDef py_edk2console_methods[] = {
   {"input_reset", (PyCFunction)py_console_input_reset, METH_NOARGS,
    "Reset input console"},   
   
   {"getkeys", (PyCFunction)py_console_getkeys, METH_VARARGS | METH_KEYWORDS,
    "Get keystroke sequence from EDK2 console"},

   {"output_reset", (PyCFunction)py_console_output_reset, METH_NOARGS,
    "Reset output console"},   

   {"puts", (PyCFunction)py_console_puts, METH_VARARGS,
    "Output unicode string on console"},

   {"get_output_mode", (PyCFunction)py_console_get_output_mode, METH_NOARGS,
    "Get current output mode number"},   

   {"get_output_size", (PyCFunction)py_console_get_output_size, METH_VARARGS,
    "Get mode screen size"},   

   {"set_output_mode", (PyCFunction)py_console_set_output_mode, METH_VARARGS,
    "Set output mode (predefined screen size)"},   

   {"set_output_attr", (PyCFunction)py_console_set_output_attr, METH_VARARGS,
    "Set output attribute (bg/fg color)"},   

   {"clear_screen", (PyCFunction)py_console_clear_screen, METH_NOARGS,
    "Clear screen"},   

   {"get_cursor_pos", (PyCFunction)py_console_get_cursor_pos, METH_NOARGS,
    "Get current cursor column and row"},   

   {"set_cursor_pos", (PyCFunction)py_console_set_cursor_pos, METH_VARARGS,
    "Set current cursor column and row"},   

   {"set_cursor_visibility", (PyCFunction)py_console_set_cursor_visibility,
    METH_VARARGS,
    "Set cursor visibility"},   

   {"get_timer", (PyCFunction)py_console_get_timer, METH_NOARGS,
    "Get millisecond counter value"},   

   {"get_output_mode_ex", (PyCFunction)py_console_get_output_mode_ex,
    METH_NOARGS,
    "Get output mode with extended info"},   

   {"install_readline_hook", (PyCFunction)py_console_install_readline_hook,
    METH_VARARGS,
    "Install python readline hook"},

   {"shutdown_interactive", (PyCFunction)py_console_shutdown_interactive,
    METH_NOARGS,
    "Detach readline and restore firmware console for Shell"},
   
   {NULL, NULL}
};

PyDoc_STRVAR(module_doc,
"Console control for python running under UEFI");

static struct PyModuleDef edk2console_module = {
    PyModuleDef_HEAD_INIT,
    "edk2console",
    module_doc,
    -1,
    py_edk2console_methods,
    NULL,
    NULL,
    NULL,
    py_console_free
};

PyMODINIT_FUNC
PyInit_edk2console(void)
{
   PyObject *m = PyModule_Create(&edk2console_module);
   if (m == NULL)
      return NULL;

   return m;
}
