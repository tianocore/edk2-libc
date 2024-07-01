#!/bin/bash

TOOL_CHAIN_TAG=$1
TARGET=$2
ARCH=$3
OUT_FOLDER=$4

function usage {
    echo
    echo Shell Script to create Python EFI Package.
    echo
    echo
    echo "Usage: $0 <ToolChain> <Target> <Architecture> <OutFolder>"
    echo "Where "
    echo "    ToolChain     :  name of the tool chain such as GCC5"
    echo "    Target        :  build type such as RELEASE, DEBUG"
    echo "    Architecture  :  Architecture such as X64"
    echo "    OutFolder     :  Output directory for creating the package"
    echo
}

function error {
    echo Failed to Create Python EFI Package
    echo Python.efi application is not available at "Build/AppPkg/$TARGET"_"$TOOL_CHAIN_TAG/$ARCH/"
    echo Follow the instructions in Py368ReadMe.txt to build Python interpreter
    echo Then use this script to create a Python EFI package
}


# Validate the ToolChain input
if [ -z $TOOL_CHAIN_TAG ]
then
    echo Invalid command line arguments passed, please see the below usage instructions
    usage
    exit
fi

# Validate the Target input
if [ -z $TARGET ]
then
    echo Invalid command line arguments passed, please see the below usage instructions
   usage
   exit
fi

# Validate the Architecture input
if [ -z $ARCH ]
then
    echo Invalid command line arguments passed, please see the below usage instructions
    usage
    exit
fi

# Validate the OutFolder input
if [ -z $OUT_FOLDER ]
then
    echo Invalid command line arguments passed, please see the below usage instructions
   usage
   exit
fi

echo current working directory `pwd`
echo changing working directory to $WORKSPACE
cd $WORKSPACE
ls

# check if the EDK2_LIBC_PATH environment variable set or not
if [ -z $EDK2_LIBC_PATH ]
then
    echo Warning: EDK2_LIBC_PATH environment variable is not set
    echo It should be set to edk2-libc folder path.
    echo.
    echo Assuming that edk2-libc contents are copied to edk2 folder
    echo at compilation time, setting this variable to edk2 path.
    export EDK2_LIBC_PATH=$WORKSPACE
fi

echo edk2 libc path $EDK2_LIBC_PATH

PYTHON_BIN="Build/AppPkg/$TARGET"_"$TOOL_CHAIN_TAG/$ARCH/Python.efi"
if [ ! -f $PYTHON_BIN ]
then
    error
    exit
fi


# create /EFI/Tools folder in the Output folder if it doesn't exists
if [ ! -d $OUT_FOLDER/EFI/Tools ]
then
    mkdir -p $OUT_FOLDER/EFI/Tools
fi

cp -f $PYTHON_BIN $OUT_FOLDER/EFI/Tools/

if [ ! -d $OUT_FOLDER/EFI/StdLib/lib/python36.8 ]
then
    mkdir -p $OUT_FOLDER/EFI/StdLib/lib/python36.8
fi

if [ ! -d $OUT_FOLDER/EFI/StdLib/etc ]
then
    mkdir -p $OUT_FOLDER/EFI/StdLib/etc
fi

cp -rf $EDK2_LIBC_PATH/AppPkg/Applications/Python/Python-3.6.8/Lib/*  $OUT_FOLDER/EFI/StdLib/lib/python36.8/
cp -rf $EDK2_LIBC_PATH/StdLib/Efi/StdLib/etc/*  $OUT_FOLDER/EFI/StdLib/etc/

if [ ${OUT_FOLDER:0:1} == "/" ]
then
    echo Python EFI package available at $OUT_FOLDER
else
    echo Python EFI package available at `pwd`/$OUT_FOLDER
fi
