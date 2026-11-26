# Copyright (c) 2011-2026, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials are licensed and made available under
# the terms and conditions of the BSD License that accompanies this distribution.
# The full text of the license may be found at
# https://opensource.org/license/bsd-2-clause-patent.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

"""ssl — UEFI uses a minimal wrapper; other platforms use the full stdlib module."""
import os

if os.name == 'uefi':
    from ssl._uefi_min import *  # noqa: F403
else:
    from ssl._stdlib import *  # noqa: F403
