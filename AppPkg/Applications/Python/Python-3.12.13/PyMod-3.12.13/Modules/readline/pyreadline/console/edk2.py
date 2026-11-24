# -*- Python -*-
#*****************************************************************************
#       Copyright (C) 2003-2006 Gary Bishop.
#       Copyright (C) 2006  Jorgen Stenarson. <jorgen.stenarson@bostream.nu>
#
#  Distributed under the terms of the BSD License.  The full license is in
#  the file COPYING, distributed as part of this software.
#*****************************************************************************
from __future__ import print_function, unicode_literals, absolute_import
'''Cursor control and color for the EDK2 console.

'''


import sys,os
import traceback
import re
import edk2console

import pyreadline.unicode_helper as unicode_helper

from pyreadline.logger import log
from pyreadline.unicode_helper import ensure_unicode, ensure_str
from pyreadline.keysyms import make_KeyPress, KeyPress
from pyreadline.console.ansi import AnsiState, AnsiWriter
import pyreadline.keysyms.winconstants as c32

def nolog(string):
    pass
    
log = nolog

class EfiShiftState(object):
    SHIFT_STATE_VALID = 0x80000000
    RIGHT_SHIFT_PRESSED = 0x00000001
    LEFT_SHIFT_PRESSED = 0x00000002
    RIGHT_CONTROL_PRESSED = 0x00000004
    LEFT_CONTROL_PRESSED = 0x00000008
    RIGHT_ALT_PRESSED = 0x00000010
    LEFT_ALT_PRESSED = 0x00000020
    RIGHT_LOGO_PRESSED = 0x00000040
    LEFT_LOGO_PRESSED = 0x00000080
    MENU_KEY_PRESSED = 0x00000100
    SYS_REQ_PRESSED = 0x00000200

class Console(object):
    '''Console driver for EDK2.

    '''

    def __init__(self):
        '''Initialize the Console object.
        '''
        self.defaultstate = AnsiState()
        self.ansiwriter = AnsiWriter(self.defaultstate)
        
        self.softspace = 0 # this is for using it as a file-like object
        self.serial = 0
        self.keyq = []

        mode_ex = edk2console.get_output_mode_ex()
        self.attr = mode_ex['attr']        
        self.saveattr = mode_ex['attr']
        self.idle_hook = None
        
    def pos(self, x=None, y=None):
        '''Move or query the window cursor.'''
        if x is None:
            return edk2console.get_cursor_pos()
        else:
            return edk2console.set_cursor_pos(x, y)

    def home(self):
        '''Move to home.'''
        self.pos(0, 0)


    # This pattern should match all characters that change the cursor position differently
    # than a normal character.
    motion_char_re = re.compile('([\n\r\t\010\007])')

    def write_scrolling(self, text, attr=None):
        '''write text at current cursor position while watching for scrolling.

        If the window scrolls because you are at the bottom of the screen
        buffer, all positions that you are storing will be shifted by the
        scroll amount. For example, I remember the cursor position of the
        prompt so that I can redraw the line but if the window scrolls,
        the remembered position is off.

        This variant of write tries to keep track of the cursor position
        so that it will know when the screen buffer is scrolled. It
        returns the number of lines that the buffer scrolled.

        '''
        text = ensure_unicode(text)
        x, y = self.pos()
        w, h = self.size()
        scroll = 0 # the result
        # split the string into ordinary characters and funny characters
        chunks = self.motion_char_re.split(text)
        for chunk in chunks:
            n = self.write_color(chunk, attr)
            if len(chunk) == 1: # the funny characters will be alone
                if chunk[0] == '\n': # newline
                    x = 0
                    y += 1
                elif chunk[0] == '\r': # carriage return
                    x = 0
                elif chunk[0] == '\t': # tab
                    x = 8 * (int(x / 8) + 1)
                    if x > w: # newline
                        x -= w
                        y += 1
                elif chunk[0] == '\007': # bell
                    pass
                elif chunk[0] == '\010':
                    x -= 1
                    if x < 0:
                        y -= 1 # backed up 1 line
                else: # ordinary character
                    x += 1
                if x == w: # wrap
                    x = 0
                    y += 1
                if y == h: # scroll
                    scroll += 1
                    y = h - 1
            else: # chunk of ordinary characters
                x += n
                l = int(x / w) # lines we advanced
                x = x % w # new x value
                y += l
                if y >= h: # scroll
                    scroll += y - h + 1
                    y = h - 1
        return scroll

    def write_color(self, text, attr=None):
        if attr is None:
            attr = self.attr
        text = ensure_unicode(text)
        n, res= self.ansiwriter.write_color(text, attr)
        for attr,chunk in res:
            log("console.attr:%s"%(attr))
            log("console.chunk:%s"%(chunk))
            edk2console.set_output_attr(attr.get_winattr())
            edk2console.puts(chunk)
        return n

    def write_plain(self, text, attr=None):
        '''write text at current cursor position.'''
        text = ensure_unicode(text)
        log('write("%s", %s)' %(text, attr))
        if attr is None:
            attr = self.attr
        edk2console.set_output_attr(attr)
        edk2console.puts(text)
        return len(text)

    # make this class look like a file object
    def write(self, text):
        text = ensure_unicode(text)
        log('write("%s")' % text)
        return self.write_color(text)

    #write = write_scrolling

    def isatty(self):
        return True

    def flush(self):
        pass

    def page(self, attr=None, fill=' '):
        '''Fill the entire screen.'''
        if attr is None:
            attr = self.attr
        if len(fill) != 1:
            raise ValueError
        edk2console.set_cursor_pos(0, 0)
        mode = edk2console.get_output_mode()
        (_, width, height) = edk2console.get_output_size(mode)
        edk2console.set_output_attr(attr)
        for y in range(height):
            edk2console.puts(unicode(fill[0]) * width)

        self.attr = attr

    def text(self, x, y, text, attr=None):
        '''Write text at the given position.'''
        if attr is None:
            attr = self.attr

        mode_ex = edk2console.get_output_mode_ex()
        edk2console.set_cursor_pos(x, y)
        edk2console.set_output_attr(attr)
        edk2console.puts(unicode(text))

        edk2console.set_output_attr(mode_ex['attr'])
        edk2console.set_cursor_pos(mode_ex['cursor_column'], mode_ex['cursor_row'])

    blank_space = u' ' * 100
    
    def clear_to_end_of_window(self):
        mode_ex = edk2console.get_output_mode_ex()
        (_, width, height) = edk2console.get_output_size(mode_ex['mode'])
                                                             
        edk2console.set_output_attr(self.attr)
        edk2console.puts(self.blank_space[0:width-mode_ex['cursor_column']-1])
        
#        for row in range(mode_ex['cursor_row']+1, height):
#            edk2console.set_cursor_pos(0, row)            
#            edk2console.puts(self.blank_space[0:width-1])
            
        edk2console.set_output_attr(mode_ex['attr'])
        edk2console.set_cursor_pos(mode_ex['cursor_column'], mode_ex['cursor_row'])

    def rectangle(self, rect, attr=None, fill=' '):
        '''Fill Rectangle.'''
        mode_ex = edk2console.get_output_mode_ex()
        
        x0, y0, x1, y1 = rect

        if attr is None:
            attr = self.attr
        for y in range(y0, y1):
            edk2console.set_cursor_pos(x0, y)
            edk2console.set_output_attr(attr)
            edk2console.puts(unicode(fill[0]) * (x1-x0))
            
        edk2console.set_output_attr(mode_ex['attr'])
        edk2console.set_cursor_pos(mode_ex['cursor_column'], mode_ex['cursor_row'])
            

    def scroll(self, rect, dx, dy, attr=None, fill=' '):
        pass

    def scroll_window(self, lines):
        edk2console.puts(u'\n')

    efi_scan_codes = {
        0x01: c32.VK_UP,
        0x02: c32.VK_DOWN,
        0x03: c32.VK_RIGHT,
        0x04: c32.VK_LEFT,
        0x05: c32.VK_HOME,
        0x06: c32.VK_END,
        0x07: c32.VK_INSERT,
        0x08: c32.VK_DELETE,
        0x09: c32.VK_PRIOR,
        0x0a: c32.VK_NEXT,
        0x0b: c32.VK_F1,
        0x0c: c32.VK_F2,
        0x0d: c32.VK_F3,
        0x0e: c32.VK_F4,
        0x0f: c32.VK_F5,
        0x10: c32.VK_F6,
        0x11: c32.VK_F7,
        0x12: c32.VK_F8,
        0x13: c32.VK_F9,
        0x14: c32.VK_F10,
        0x15: c32.VK_F11,
        0x16: c32.VK_F12,
        0x17: c32.VK_ESCAPE,
        0x68: c32.VK_F13,
        0x69: c32.VK_F14,
        0x6a: c32.VK_F15,
        0x6b: c32.VK_F16,
        0x6c: c32.VK_F17,
        0x6d: c32.VK_F18,
        0x6e: c32.VK_F19,
        0x6f: c32.VK_F20,
        0x70: c32.VK_F21,
        0x71: c32.VK_F22,
        0x72: c32.VK_F23,
        0x73: c32.VK_F24
    }

    special_chars = {
        ord(' '): c32.VK_SPACE,
        ord('\b'): c32.VK_BACK,
        ord('\t'): c32.VK_TAB,
        ord('\r'): c32.VK_RETURN
    }
        
    def getkeypress(self):
        '''Return next key press event from the queue, ignoring others.'''
        while True:
            for keystroke in edk2console.getkeys(wait=1, timeout=100000):
                unicode_char = keystroke['unicode_char']
                scan_code = keystroke['scan_code']
                shift_state = keystroke['shift_state']
                
                ctrl_pressed = (shift_state & (EfiShiftState.LEFT_CONTROL_PRESSED | \
                                               EfiShiftState.RIGHT_CONTROL_PRESSED) != 0)
                shift_pressed = (shift_state & (EfiShiftState.LEFT_SHIFT_PRESSED | \
                                                EfiShiftState.RIGHT_SHIFT_PRESSED) != 0)
                alt_pressed = ((shift_state & EfiShiftState.LEFT_ALT_PRESSED) != 0)

                state = 0
                if ctrl_pressed:
                    state |= 4
                if shift_pressed:
                    state |= 16
                if alt_pressed:
                    state |= 1

                if unicode_char == 0:
                    if scan_code in self.efi_scan_codes:
                        self.keyq.append(
                            event(self, unicode_char, state, self.efi_scan_codes[scan_code])
                            )
                else:
                    if unicode_char in self.special_chars:
                        self.keyq.append(
                            event(self, unicode_char, state, self.special_chars[unicode_char])
                            )
                    else:
                        if ctrl_pressed:
                            keysym = ord(chr(unicode_char & 0xff).upper())
                        else:
                            keysym = 0xff
                            
                        self.keyq.append(
                            event(self, unicode_char, state, keysym)
                            )

            if self.keyq:
                e = self.keyq.pop(0)
                return e
            elif self.idle_hook is not None:
                self.idle_hook()
                    
    def title(self, txt=None):
        pass
    
    def size(self, width=None, height=None):
        '''Set/get window size.'''
        if width is not None and height is not None:
            pass
        else:
            mode = edk2console.get_output_mode()
            (_, width, height) = edk2console.get_output_size(mode)
            return (width, height)

    def cursor(self, visible=None, size=None):
        '''Set cursor on or off.'''
        if visible is not None:
            edk2console.set_cursor_visibility(visible)

    def bell(self):
        pass

    def next_serial(self):
        '''Get next event serial number.'''
        self.serial += 1
        return self.serial


from .event import Event

class event(Event):
    '''Represent events from the console.'''
    def __init__(self, console, unicode_char, state, keycode):
        '''Initialize an event from the Windows input structure.'''
        self.type = '??'
        self.serial = console.next_serial()
        self.width = 0
        self.height = 0
        self.x = 0
        self.y = 0
        self.keysym = '??'
        self.width = None
        
        self.char = chr(unicode_char & 0xff)
        self.keycode = keycode
        self.state = state
        self.keyinfo = make_KeyPress(self.char,self.state,self.keycode)



def getconsole(buffer=1):
        """Get a console handle.

        If buffer is non-zero, a new console buffer is allocated and
        installed.  Otherwise, this returns a handle to the current
        console buffer"""

        c = Console()

        return c

readline_hook = None # the python hook goes here
readline_ref = None  # reference to the c-callable to keep it alive

def hook_wrapper(stdin, stdout, prompt):
    '''Wrap a Python readline so it behaves like GNU readline.'''
    try:
        # call the Python hook
        res = ensure_str(readline_hook(prompt))
        # make sure it returned the right sort of thing
        if res and not isinstance(res, bytes):
            raise TypeError('readline must return a string.')
    except KeyboardInterrupt:
        # GNU readline returns 0 on keyboard interrupt
        return 0
    except EOFError:
        # It returns an empty string on EOF
        res = ensure_str('')
    except:
        print('Readline internal error', file=sys.stderr)
        traceback.print_exc()
        res = ensure_str('\n')
    return res


def install_readline(hook):
    '''Set up things for the interpreter to call 
    our function like GNU readline.'''
    global readline_hook, readline_ref
    # save the hook so the wrapper can call it
    readline_hook = hook

    edk2console.install_readline_hook(hook_wrapper)

