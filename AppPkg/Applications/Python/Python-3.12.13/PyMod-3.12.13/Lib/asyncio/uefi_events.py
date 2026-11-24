# Copyright (c) 2011-2026, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials are licensed and made available under
# the terms and conditions of the BSD License that accompanies this distribution.
# The full text of the license may be found at
# https://opensource.org/license/bsd-2-clause-patent.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

"""Selector event loop for UEFI"""

import warnings
import os

from . import base_events
from . import base_subprocess
from . import constants
from . import coroutines
from . import events
from . import exceptions
from . import futures
from . import selector_events
from . import tasks
from . import transports
from .log import logger


__all__ = (
    'SelectorEventLoop',
    'DefaultEventLoopPolicy',
)


class _UefiSelectorEventLoop(selector_events.BaseSelectorEventLoop):
    """UEFI event loop.

    """

    def __init__(self, selector=None):
        self._infd = None
        self._outfd = None
        super().__init__(selector)
        
    def _close_self_pipe(self):        
        if hasattr(self, '_outfd') and self._outfd is not None:
            self._remove_reader(self._outfd)            
            os.close(self._outfd)
            self._outfd = None
        if hasattr(self, '_infd') and  self._infd is not None:
            os.close(self._infd)
            self._infd = None

    def _make_self_pipe(self):
        assert(self._infd is None and self._outfd is None, 'pipe is open')        
        (self._infd, self._outfd) = os.pipe()
        self._add_reader(self._outfd, self._read_from_self)
        
    def _process_self_data(self, data):
        pass

    def _read_from_self(self):
        while True:
            try:
                data = os.read(self._outfd, 1024)
                if not data:
                    break
                self._process_self_data(data)
            except InterruptedError:
                continue
            except BlockingIOError:
                break

    def _write_to_self(self):
        infd = self._infd
        if infd is None:
            return

        try:
            os.write(infd, b'\0')
        except OSError:
            if self._debug:
                logger.debug("Fail to write a null byte into the "
                             "self-pipe socket",
                             exc_info=True)
        
    def close(self):
        super().close()

    def add_signal_handler(self, sig, callback, *args):
        pass

    def remove_signal_handler(self, sig):
        pass


class _UefiDefaultEventLoopPolicy(events.BaseDefaultEventLoopPolicy):
    """UEFI event loop policy with a watcher for child processes."""
    _loop_factory = _UefiSelectorEventLoop

    def __init__(self):
        super().__init__()

    def set_event_loop(self, loop):
        """Set the event loop.
        """

        super().set_event_loop(loop)



SelectorEventLoop = _UefiSelectorEventLoop
DefaultEventLoopPolicy = _UefiDefaultEventLoopPolicy
