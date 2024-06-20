1. Introduction
---------------
This is a Best-Known Methods document capturing the details of build environment setup for compiling the Python UEFI
interpreter version 3.6.8 using GCC tool chain on Linux based operating systems. These BKMs have been tested with WSL2
(Windows Subsystem for Linux 2) based Ubuntu 20.04 LTS environment on Windows 10. Only 64-bit builds are possible due
to a limitation in the LibC support for GCC compiler tool chain on UEFI. If you are working on a Ubuntu 20.04 LTS system
you may skip the sections 1.1 and 1.2


1.1.  WSL2 Installation on Windows 10 OS
----------------------------------------

The WSL2 environment can be setup by following the instructions given in the below webpage
https://pureinfotech.com/install-windows-subsystem-linux-2-windows-10/

1.2.  Installing Ubuntu 20.04 LTS on WSL2 on Windows 10 OS
----------------------------------------------------------

The instructions provided in the below webpage will help in installing the Ubuntu 20.04 LTS OS environment on WSL2 on Windows 10 OS.
https://www.altisconsulting.com/au/insights/installing-ubuntu-bash-for-windows-10-wsl2-setup/

2. EDK2 build environment setup
-------------------------------

You may follow the instructions provided in the below webpage to setup the edk2 build environment.

https://github.com/tianocore/tianocore.github.io/wiki/Using-EDK-II-with-Native-GCC

For the purposes of describing the BKMs, we will be using the following paths.
Edk2 source tree: $HOME/src/edk2
Native GCC version >=4.x compiler installation: /usr/bin/gcc
Intel ASL Compiler installation:  /usr/bin/iasl


2.1.  Installing essential packages for GCC compiler
----------------------------------------------------

Install the below required essential packages to facilitate compilation of the edk2 repo using GCC compiler tool chain.
Several Ubuntu packages will be needed to set up the build environment for EDK II.

The following command will install all required packages::

        bash$ sudo apt-get update
        bash$ sudo apt install build-essential
        bash$ sudo apt install uuid-dev iasl git  nasm  python-is-python3

Package - Description
    * build-essential : Informational list of build-essential packages
    * uuid-dev : Universally Unique ID library (headers and static libraries)
    * iasl : Intel ASL compiler/decompiler (also provided by acpica-tools)
    * git : Support for git revision control system
    * nasm : General-purpose x86 assembler
    * python-is-python3 : Ubuntu 20.04 python command is 'python3' but edk2 tools use 'python'

Once the above packages are installed, then follow the instructions in the webpage to setup the rest of the build
environment
https://github.com/tianocore/tianocore.github.io/wiki/Common-instructions


2.2.  Cloning edk2 git repo
---------------------------

Get the edk2 source tree using Git commands.
Execute the below commands to clone the edk2 git repo::
        bash$ mkdir ~/src
        bash$ cd ~/src
        bash$ git clone https://github.com/tianocore/edk2

This will create a folder named edk2 as a clone of the edk2 git repo.
Execute the below commands to checkout / initialize all the git submodules::
        bash$ cd ~/src/edk2
        bash$ git submodule update --init

The above command initializes all the submodules in the git repo
The below commands help to setup the environment variables required for building the edk2 repo::

        bash$ make -C BaseTools
        bash$ . edksetup.sh

Run the below make command from ~/src folder i.e change working directory to ~/src and run the make command as given below ::

        bash$ cd ~/src
        bash$ make -C edk2/BaseTools

Setup build shell environment ::

        bash$ cd ~/src/edk2
        bash$ export EDK_TOOLS_PATH=$HOME/src/edk2/BaseTools
        bash$ . edksetup.sh BaseTools


Set Build Target Information
For the Conf/target.txt file, find the following lines:
ACTIVE_PLATFORM       = Nt32Pkg/Nt32Pkg.dsc

TOOL_CHAIN_TAG        = MYTOOLS

And change the corresponding lines to match these:

ACTIVE_PLATFORM       = MdeModulePkg/MdeModulePkg.dsc

TOOL_CHAIN_TAG        = GCC5

Install GCC5 on Ubuntu 20.04 LTS by downloading it from
https://askubuntu.com/questions/1235819/ubuntu-20-04-gcc-version-lower-than-gcc-7

This requires latest version of the nasm tool to be installed.
The nasm latest version >= 2.15.05

Debian package for nasm 2.15.05 is available at

https://ubuntu.pkgs.org/21.10/ubuntu-universe-amd64/nasm_2.15.05-1_amd64.deb.html

Download it using the wget command as given below::

        bash$ sudo wget http://archive.ubuntu.com/ubuntu/pool/universe/n/nasm/nasm_2.15.05-1_amd64.deb


Install the the above downloaded nasm debian package using the following command::

        bash$ sudo dpkg -i  nasm_2.15.05-1_amd64.deb

2.3.  Install additional packages required for building edk2 with GCC
---------------------------------------------------------------------

Install additional package required for building edk2 repo::

        bash$ sudo apt install libx11-dev
        bash$ sudo apt install libxext-dev


2.4  Clone edk2-libc and copy the contents to edk2
--------------------------------------------------

Now clone the edk2-libc git repo by following / executing the below commands::

        bash$ cd ~/src
        bash$ git clone https://github.com/tianocore/edk2-libc.git

Set the PACKAGES_PATH and EDK2_LIBC_PATH using the commands below::

        bash$ export PACKAGES_PATH=$HOME/src/edk2:$HOME/src/edk2-libc
        bash$ export EDK2_LIBC_PATH=$HOME/src/edk2-libc

Verify that you can build the hello world application from AppPkg under edk2-libc by running the following commands
Build hello.inf from edk2-libc applications::

        $bash build -p ../edk2-libc/AppPkg/AppPkg.dsc -m ../edk2-libc/AppPkg/Applications/Hello/Hello.inf


2.5.  Build Python Interpreter using GCC
----------------------------------------

Execute the below command to build the X64 version of the Python 3.6.8 interpreter using GCC tool chain::

        bash$ cd AppPkg/Applications/Python/Python-3.6.8/
        bash$ python srcprep.py
        bash$ cd ~/src/edk2
        bash$ build -a X64 -b RELEASE -p ../edk2-libc/AppPkg/AppPkg.dsc \
          -m ../edk2-libc/AppPkg/Applications/Python/Python-3.6.8/Python368.inf -D BUILD_PYTHON368
             or
        bash$ build -a X64 -b RELEASE -p AppPkg/AppPkg.dsc -D BUILD_PYTHON368


2.5  Create Python UEFI package with all dependencies
-----------------------------------------------------

To create an usable Python UEFI package with all the dependencies from the build environment,
you can use the bash shell script create_python_pkg.sh available under /AppPkg/Applications/Python/Python-3.6.8
folder. Ensure that EDK2_LIBC_PATH environment variable to edk2-libc folder path.

Ensure that EDK2_LIBC_PATH environment variable to edk2-libc folder path::

        bash$ echo $EDK2_LIBC_PATH

The environment variable should be set to the folder path for the edk2-libc cloned repo.

Use the following commands to create Python UEFI package::

        bash$ cd ~/src/edk2/AppPkg/Applications/Python/Python-3.6.8/
        bash$ chmod 777 create_python_pkg.sh
        ./create_python_pkg.sh GCC5 RELEASE X64 myPyUEFI

where GCC5 refers to the tool chain, RELEASE refers to the build type, X64 refers to the architecture
myPyUEFI refers to the name of the folder where the Python UEFI package needs to be created.

When you run the create_python_pkg.sh without any parameters it lists the help
information as shown below:

Shell Script to create Python EFI Package.

Usage: ./create_python_pkg.sh <ToolChain> <Target> <Architecture> <OutFolder>

Where
    * ToolChain     :  name of the tool chain such as GCC5
    * Target        :  build type such as RELEASE, DEBUG
    * Architecture  :  Architecture such as X64
    * OutFolder     :  Output directory for creating the package
