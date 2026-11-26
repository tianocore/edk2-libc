# Copyright (c) 2011-2026, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials are licensed and made available under
# the terms and conditions of the BSD License that accompanies this distribution.
# The full text of the license may be found at
# https://opensource.org/license/bsd-2-clause-patent.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

"""The asyncio package, tracking PEP 3156."""

# flake8: noqa

import os
import sys

# This relies on each of the submodules having an __all__ variable.
from .base_events import *
from .coroutines import *
from .events import *
from .exceptions import *
from .futures import *
from .locks import *
from .protocols import *
from .runners import *
from .queues import *
from .streams import *
from .subprocess import *
from .tasks import *
from .taskgroups import *
from .timeouts import *
from .threads import *
from .transports import *

__all__ = (base_events.__all__ +
           coroutines.__all__ +
           events.__all__ +
           exceptions.__all__ +
           futures.__all__ +
           locks.__all__ +
           protocols.__all__ +
           runners.__all__ +
           queues.__all__ +
           streams.__all__ +
           subprocess.__all__ +
           tasks.__all__ +
           taskgroups.__all__ +
           threads.__all__ +
           timeouts.__all__ +
           transports.__all__)

if sys.platform == 'win32':  # pragma: no cover
    from .windows_events import *
    __all__ += windows_events.__all__
elif os.name == 'uefi':
    from .uefi_events import *  # pragma: no cover
    __all__ += uefi_events.__all__
else:
    from .unix_events import *  # pragma: no cover
    __all__ += unix_events.__all__
