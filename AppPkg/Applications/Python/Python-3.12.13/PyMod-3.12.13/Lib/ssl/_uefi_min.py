# Copyright (c) 2011-2026, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials are licensed and made available under
# the terms and conditions of the BSD License that accompanies this distribution.
# The full text of the license may be found at
# https://opensource.org/license/bsd-2-clause-patent.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

"""Minimal ssl for UEFI (VS2022 Shell exit after full Lib/ssl.py import).

Loads _ssl only — no Lib/socket.py, enum._convert_, or SSLSocket/SSLObject graph.
"""
import os
import sys

if os.name != 'uefi':
    raise ImportError("_uefi_min is only for os.name == 'uefi'")

import _ssl
from _ssl import (  # re-export common C API / constants
    OPENSSL_VERSION_NUMBER,
    OPENSSL_VERSION_INFO,
    OPENSSL_VERSION,
    _SSLContext,
    MemoryBIO,
    SSLSession,
    SSLError,
    SSLZeroReturnError,
    SSLWantReadError,
    SSLWantWriteError,
    SSLSyscallError,
    SSLEOFError,
    SSLCertVerificationError,
    RAND_status,
    RAND_add,
    RAND_bytes,
    HAS_SNI,
    HAS_ECDH,
    HAS_NPN,
    HAS_ALPN,
    HAS_SSLv2,
    HAS_SSLv3,
    HAS_TLSv1,
    HAS_TLSv1_1,
    HAS_TLSv1_2,
    HAS_TLSv1_3,
    _DEFAULT_CIPHERS,
    _OPENSSL_API_VERSION,
    PROTOCOL_TLS,
    PROTOCOL_TLS_CLIENT,
    PROTOCOL_TLS_SERVER,
    CERT_NONE,
    CERT_OPTIONAL,
    CERT_REQUIRED,
)

CertificateError = SSLCertVerificationError
socket_error = OSError


class Purpose:
    """OID strings only (no OpenSSL txt2obj at import)."""
    SERVER_AUTH = '1.3.6.1.5.5.7.3.1'
    CLIENT_AUTH = '1.3.6.1.5.5.7.3.2'


SSLContext = _SSLContext


def _materialize_purpose(purpose):
    if isinstance(purpose, str):
        return purpose
    if isinstance(purpose, Purpose):
        return purpose.value
    return purpose


def create_default_context(purpose=Purpose.SERVER_AUTH, *, cafile=None,
                           capath=None, cadata=None):
    is_server = (purpose == Purpose.SERVER_AUTH)
    is_client = (purpose == Purpose.CLIENT_AUTH)
    purpose = _materialize_purpose(purpose)
    if is_server:
        # C _SSLContext(PROTOCOL_TLS_CLIENT) already sets CERT_REQUIRED + check_hostname.
        context = SSLContext(PROTOCOL_TLS_CLIENT)
    elif is_client:
        context = SSLContext(PROTOCOL_TLS_SERVER)
    else:
        raise ValueError(purpose)
    if cafile or capath or cadata:
        context.load_verify_locations(cafile, capath, cadata)
    if hasattr(context, 'keylog_filename'):
        keylogfile = os.environ.get('SSLKEYLOGFILE')
        if keylogfile and not sys.flags.ignore_environment:
            context.keylog_filename = keylogfile
    return context


def _create_unverified_context(protocol=None, *, cert_reqs=CERT_NONE,
                               check_hostname=False, purpose=Purpose.SERVER_AUTH,
                               certfile=None, keyfile=None,
                               cafile=None, capath=None, cadata=None):
    is_server = (purpose == Purpose.SERVER_AUTH)
    is_client = (purpose == Purpose.CLIENT_AUTH)
    purpose = _materialize_purpose(purpose)
    if is_server:
        if protocol is None:
            protocol = PROTOCOL_TLS_CLIENT
    elif is_client:
        if protocol is None:
            protocol = PROTOCOL_TLS_SERVER
    else:
        raise ValueError(purpose)
    context = SSLContext(protocol)
    context.check_hostname = check_hostname
    if cert_reqs is not None:
        context.verify_mode = cert_reqs
    if keyfile and not certfile:
        raise ValueError("certfile must be specified")
    if certfile or keyfile:
        context.load_cert_chain(certfile, keyfile)
    if cafile or capath or cadata:
        context.load_verify_locations(cafile, capath, cadata)
    if hasattr(context, 'keylog_filename'):
        keylogfile = os.environ.get('SSLKEYLOGFILE')
        if keylogfile and not sys.flags.ignore_environment:
            context.keylog_filename = keylogfile
    return context


_create_default_https_context = create_default_context
_create_stdlib_context = _create_unverified_context
