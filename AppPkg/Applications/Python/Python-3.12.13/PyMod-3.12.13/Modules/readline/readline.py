# Copyright (c) 2011-2026, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials are licensed and made available under
# the terms and conditions of the BSD License that accompanies this distribution.
# The full text of the license may be found at
# https://opensource.org/license/bsd-2-clause-patent.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

# -*- Python -*-
#this file is needed in site-packages to emulate readline
#necessary for rlcompleter since it relies on the existance
#of a readline module
from __future__ import print_function, unicode_literals, absolute_import
import os

__all__ = [ 'parse_and_bind',
            'get_line_buffer',
            'insert_text',
            'clear_history',
            'read_init_file',
            'read_history_file',
            'write_history_file',
            'get_current_history_length',
            'get_history_length',
            'get_history_item',
            'set_history_length',
            'set_startup_hook',
            'set_pre_input_hook',
            'set_completer',
            'get_completer',
            'get_begidx',
            'get_endidx',
            'set_completer_delims',
            'get_completer_delims',
            'add_history',
            'callback_handler_install',
            'callback_handler_remove',
            'callback_read_char',] #Some other objects are added below


def _uefi_pyreadline_enabled():
    if os.name != 'uefi':
        return True
    _want = os.environ.get('PY_UEFI_READLINE', '')
    return _want in ('1', 'yes', 'true', 'YES', 'TRUE')


if os.name == 'uefi' and not _uefi_pyreadline_enabled():
    # Opt-in by design: importing pyreadline opens ConInEx and installs a REPL
    # input hook, which scripted and unattended runs must not get just because
    # something in their import graph pulled in readline. Keep every public
    # name a no-op until the caller asks for line editing.
    def dummy(completer=""):
        pass
    for funk in __all__:
        globals()[funk] = dummy

    class _ReadlineStub(object):
        disable_readline = True

    rl = _ReadlineStub()
else:
    from pyreadline.rlmain import Readline

    # create a Readline object to contain the state
    rl = Readline()

    if rl.disable_readline:
        def dummy(completer=""):
            pass
        for funk in __all__:
            globals()[funk] = dummy
    else:
        def GetOutputFile():
            '''Return the console object used by readline so that it can be used for printing in color.'''
            return rl.console
        __all__.append("GetOutputFile")

        import pyreadline.console.edk2 as console

        # make these available so this looks like the python readline module
        read_init_file = rl.read_init_file
        parse_and_bind = rl.parse_and_bind
        clear_history = rl.clear_history
        add_history = rl.add_history
        insert_text = rl.insert_text

        write_history_file = rl.write_history_file
        read_history_file = rl.read_history_file

        get_completer_delims = rl.get_completer_delims
        get_current_history_length = rl.get_current_history_length
        get_history_length = rl.get_history_length
        get_history_item = rl.get_history_item
        get_line_buffer = rl.get_line_buffer
        set_completer = rl.set_completer
        get_completer = rl.get_completer
        get_begidx = rl.get_begidx
        get_endidx = rl.get_endidx

        set_completer_delims = rl.set_completer_delims
        set_history_length = rl.set_history_length
        set_pre_input_hook = rl.set_pre_input_hook
        set_startup_hook = rl.set_startup_hook

        callback_handler_install=rl.callback_handler_install
        callback_handler_remove=rl.callback_handler_remove
        callback_read_char=rl.callback_read_char

        console.install_readline(rl.readline)

        import rlcompleter

        #Override completer from rlcompleter to disable automatic ( on callable
        completer_obj = rlcompleter.Completer()
        def nop(val, word):
            return word
        completer_obj._callable_postfix = nop
        rl.set_completer(completer_obj.complete)

        #activate tab completion
        rl.parse_and_bind("tab: complete")
        rl.read_history_file()
#    rl.console.idle_hook = rl.write_history_file

__all__.append("rl")
