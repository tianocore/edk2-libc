## @file
#   Intel(r) UEFI Application Development Kit for EDK II.
#   This package contains applications which depend upon Standard Libraries
#   from the StdLib package.
#
#   See the comments in the [LibraryClasses.IA32] and [BuildOptions] sections
#   for important information about configuring this package for your
#   environment.
#
#   Copyright (c) 2010 - 2024, Intel Corporation. All rights reserved.<BR>
#   SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
  PLATFORM_NAME                  = AppPkg
  PLATFORM_GUID                  = 0458dade-8b6e-4e45-b773-1b27cbda3e06
  PLATFORM_VERSION               = 0.01
  DSC_SPECIFICATION              = 0x00010006
  OUTPUT_DIRECTORY               = Build/AppPkg
  SUPPORTED_ARCHITECTURES        = IA32|X64|ARM|AARCH64|LOONGARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT

#
#  Debug output control
#
  DEFINE DEBUG_ENABLE_OUTPUT      = FALSE       # Set to TRUE to enable debug output
  DEFINE DEBUG_PRINT_ERROR_LEVEL  = 0x80000040  # Flags to control amount of debug output
  DEFINE DEBUG_PROPERTY_MASK      = 0

  # Python 3.12: FALSE = Iteration 1 MIN (no Phase 8). FULL: build -D BUILD_PYTHON312_FULL=TRUE
  DEFINE BUILD_PYTHON312_FULL     = FALSE

!include MdePkg/MdeLibs.dsc.inc

[PcdsFeatureFlag]

[PcdsFixedAtBuild]
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|$(DEBUG_PROPERTY_MASK)
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|$(DEBUG_PRINT_ERROR_LEVEL)

[LibraryClasses]
  #
  # Entry Point Libraries
  #
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  ShellCEntryLib|ShellPkg/Library/UefiShellCEntryLib/UefiShellCEntryLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  #
  # Common Libraries
  #
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  !if $(DEBUG_ENABLE_OUTPUT)
    DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
    DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  !else   ## DEBUG_ENABLE_OUTPUT
    DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  !endif  ## DEBUG_ENABLE_OUTPUT

  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PciCf8Lib|MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  PerformanceLib|MdeModulePkg/Library/DxePerformanceLib/DxePerformanceLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
  SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf

  ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf

  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf

###################################################################################################
#
# Components Section - list of the modules and components that will be processed by compilation
#                      tools and the EDK II tools to generate PE32/PE32+/Coff image files.
#
# Note: The EDK II DSC file is not used to specify how compiled binary images get placed
#       into firmware volume images. This section is just a list of modules to compile from
#       source into UEFI-compliant binaries.
#       It is the FDF file that contains information on combining binary files into firmware
#       volume images, whose concept is beyond UEFI and is described in PI specification.
#       Binary modules do not need to be listed in this section, as they should be
#       specified in the FDF file. For example: Shell binary (Shell_Full.efi), FAT binary (Fat.efi),
#       Logo (Logo.bmp), and etc.
#       There may also be modules listed in this section that are not required in the FDF file,
#       When a module listed here is excluded from FDF file, then UEFI-compliant binary will be
#       generated for it, but the binary will not be put into any firmware volume.
#
###################################################################################################

[Components]

#### Sample Applications.
  AppPkg/Applications/Hello/Hello.inf        # No LibC includes or functions.
  AppPkg/Applications/Main/Main.inf          # Simple invocation. No other LibC functions.
  AppPkg/Applications/Enquire/Enquire.inf    #
  AppPkg/Applications/ArithChk/ArithChk.inf  #

#### A simple fuzzer for OrderedCollectionLib, in particular for
#### BaseOrderedCollectionRedBlackTreeLib.
  AppPkg/Applications/OrderedCollectionTest/OrderedCollectionTest.inf {
    <LibraryClasses>
      OrderedCollectionLib|MdePkg/Library/BaseOrderedCollectionRedBlackTreeLib/BaseOrderedCollectionRedBlackTreeLib.inf
      DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
      DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
    <PcdsFeatureFlag>
      gEfiMdePkgTokenSpaceGuid.PcdValidateOrderedCollection|TRUE
    <PcdsFixedAtBuild>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x2F
      gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80400040
  }

#### Conditional compilation of python368.inf by passing -D BUILD_PYTHON368
#### through build command
  !if $(BUILD_PYTHON368)
    AppPkg/Applications/Python/Python-3.6.8/Python368.inf
  !endif

#### Conditional compilation of python312.inf by passing -D BUILD_PYTHON312
#### MIN (default): build with -D BUILD_PYTHON312 only (Phase 8 omitted).
#### FULL: add -D BUILD_PYTHON312_FULL=TRUE (zlib, ctypes, OpenSSL).
  !if $(BUILD_PYTHON312)
    !if $(BUILD_PYTHON312_FULL) == TRUE
      AppPkg/Applications/Python/Python-3.12.13/Python312.inf
    !else
      AppPkg/Applications/Python/Python-3.12.13/Python312_MIN.inf
    !endif
  !endif

#### Un-comment the following line to build Lua.
#  AppPkg/Applications/Lua/Lua.inf


##############################################################################
#
# Specify whether we are running in an emulation environment, or not.
# Define EMULATE if we are, else keep the DEFINE commented out.
#
# DEFINE  EMULATE = 1

##############################################################################
#
#  Include Boilerplate text required for building with the Standard Libraries.
#
##############################################################################
!include StdLib/StdLib.inc
!include AppPkg/Applications/Sockets/Sockets.inc

# Boot traces are OFF as of 2026-09-09. This block is DSC-level, so it applied
# to *every* module in the build - both StdLib/LibC (which is how Main.c's boot
# lines were compiled in) and Python312 itself. Because EDK2 appends DSC
# [BuildOptions] to each module's own INF [BuildOptions], removing
# /DPY_UEFI_BOOT_TRACE=1 from Python312.inf / Python312_MIN.inf alone would NOT
# have disabled the traces - this block would have kept defining it. Both have
# to go together, and both have to come back together.
#
# To re-enable: uncomment the four lines below, and add /DPY_UEFI_BOOT_TRACE=1
# to the MSFT CC_FLAGS in the INF you are testing. All trace code is still in
# the tree, #ifdef-guarded; nothing was deleted.
#
#!if $(BUILD_PYTHON312)
#[BuildOptions]
#  MSFT:*_*_*_CC_FLAGS = /DPY_UEFI_BOOT_TRACE=1
#!endif
