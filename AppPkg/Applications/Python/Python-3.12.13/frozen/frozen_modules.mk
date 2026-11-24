SHELL = bash

CPYTHON_DIR := $(ROOT_DIR)/build/cpython
CPYTHON_FREEZE := $(CPYTHON_DIR)/Programs/_freeze_module
CPYTHON_BSTRAP := $(CPYTHON_DIR)/_bootstrap_python

HEADERS = Python/frozen_modules/importlib._bootstrap.h \
Python/frozen_modules/importlib._bootstrap_external.h \
Python/frozen_modules/zipimport.h \
Python/frozen_modules/getpath.h \
Python/frozen_modules/abc.h \
Python/frozen_modules/codecs.h \
Python/frozen_modules/io.h \
Python/frozen_modules/_collections_abc.h \
Python/frozen_modules/_sitebuiltins.h \
Python/frozen_modules/genericpath.h \
Python/frozen_modules/ntpath.h \
Python/frozen_modules/posixpath.h \
Python/frozen_modules/os.h \
Python/frozen_modules/site.h \
Python/frozen_modules/stat.h \
Python/frozen_modules/importlib.util.h \
Python/frozen_modules/importlib.machinery.h \
Python/frozen_modules/runpy.h \
Python/frozen_modules/__hello__.h \
Python/frozen_modules/__phello__.h \
Python/frozen_modules/__phello__.ham.h \
Python/frozen_modules/__phello__.ham.eggs.h \
Python/frozen_modules/__phello__.spam.h \
Python/frozen_modules/frozen_only.h

.PHONY: all

all: Python/deepfreeze/deepfreeze.c

Python/frozen_modules/importlib._bootstrap.h: ./Lib/importlib/_bootstrap.py
	$(CPYTHON_FREEZE) importlib._bootstrap $< $@

Python/frozen_modules/importlib._bootstrap_external.h: ./Lib/importlib/_bootstrap_external.py
	$(CPYTHON_FREEZE) importlib._bootstrap_external $< $@

Python/frozen_modules/zipimport.h: ./Lib/zipimport.py
	$(CPYTHON_FREEZE) zipimport $< $@

Python/frozen_modules/getpath.h: ./Modules/getpath.py
	$(CPYTHON_FREEZE) getpath $< $@

Python/frozen_modules/abc.h: ./Lib/abc.py
	$(CPYTHON_BSTRAP) ./Programs/_freeze_module.py abc $< $@

Python/frozen_modules/codecs.h: ./Lib/codecs.py
	$(CPYTHON_BSTRAP) ./Programs/_freeze_module.py codecs $< $@

Python/frozen_modules/io.h: ./Lib/io.py
	$(CPYTHON_BSTRAP) ./Programs/_freeze_module.py io $< $@

Python/frozen_modules/_collections_abc.h: ./Lib/_collections_abc.py
	$(CPYTHON_BSTRAP) ./Programs/_freeze_module.py _collections_abc $< $@

Python/frozen_modules/_sitebuiltins.h: ./Lib/_sitebuiltins.py
	$(CPYTHON_BSTRAP) ./Programs/_freeze_module.py _sitebuiltins $< $@

Python/frozen_modules/genericpath.h: ./Lib/genericpath.py
	$(CPYTHON_BSTRAP) ./Programs/_freeze_module.py genericpath $< $@

Python/frozen_modules/ntpath.h: ./Lib/ntpath.py
	$(CPYTHON_BSTRAP) ./Programs/_freeze_module.py ntpath $< $@

Python/frozen_modules/posixpath.h: ./Lib/posixpath.py
	$(CPYTHON_BSTRAP) ./Programs/_freeze_module.py posixpath $< $@

Python/frozen_modules/os.h: ./Lib/os.py
	$(CPYTHON_BSTRAP) ./Programs/_freeze_module.py os $< $@

Python/frozen_modules/site.h: ./Lib/site.py 
	$(CPYTHON_BSTRAP) ./Programs/_freeze_module.py site $< $@

Python/frozen_modules/stat.h: ./Lib/stat.py
	$(CPYTHON_BSTRAP) ./Programs/_freeze_module.py stat $< $@

Python/frozen_modules/importlib.util.h: ./Lib/importlib/util.py 
	$(CPYTHON_BSTRAP) ./Programs/_freeze_module.py importlib.util $< $@

Python/frozen_modules/importlib.machinery.h: ./Lib/importlib/machinery.py 
	$(CPYTHON_BSTRAP) ./Programs/_freeze_module.py importlib.machinery $< $@

Python/frozen_modules/runpy.h: ./Lib/runpy.py 
	$(CPYTHON_BSTRAP) ./Programs/_freeze_module.py runpy $< $@

Python/frozen_modules/__hello__.h: ./Lib/__hello__.py
	$(CPYTHON_BSTRAP) ./Programs/_freeze_module.py __hello__ $< $@

Python/frozen_modules/__phello__.h: ./Lib/__phello__/__init__.py 
	$(CPYTHON_BSTRAP) ./Programs/_freeze_module.py __phello__ $< $@

Python/frozen_modules/__phello__.ham.h: ./Lib/__phello__/ham/__init__.py 
	$(CPYTHON_BSTRAP) ./Programs/_freeze_module.py __phello__.ham $< $@

Python/frozen_modules/__phello__.ham.eggs.h: ./Lib/__phello__/ham/eggs.py
	$(CPYTHON_BSTRAP) ./Programs/_freeze_module.py __phello__.ham.eggs $< $@

Python/frozen_modules/__phello__.spam.h: ./Lib/__phello__/spam.py 
	$(CPYTHON_BSTRAP) ./Programs/_freeze_module.py __phello__.spam $< $@

Python/frozen_modules/frozen_only.h: ./Tools/freeze/flag.py
	$(CPYTHON_BSTRAP) ./Programs/_freeze_module.py frozen_only $< $@

Python/deepfreeze/deepfreeze.c: ./Tools/build/deepfreeze.py $(HEADERS)
	$(CPYTHON_BSTRAP) $< \
	   Python/frozen_modules/importlib._bootstrap.h:importlib._bootstrap \
	   Python/frozen_modules/importlib._bootstrap_external.h:importlib._bootstrap_external \
           Python/frozen_modules/zipimport.h:zipimport \
           Python/frozen_modules/abc.h:abc \
           Python/frozen_modules/codecs.h:codecs \
           Python/frozen_modules/io.h:io \
           Python/frozen_modules/_collections_abc.h:_collections_abc \
           Python/frozen_modules/_sitebuiltins.h:_sitebuiltins \
           Python/frozen_modules/genericpath.h:genericpath \
           Python/frozen_modules/ntpath.h:ntpath \
           Python/frozen_modules/posixpath.h:posixpath \
           Python/frozen_modules/os.h:os \
           Python/frozen_modules/site.h:site \
           Python/frozen_modules/stat.h:stat \
           Python/frozen_modules/importlib.util.h:importlib.util \
           Python/frozen_modules/importlib.machinery.h:importlib.machinery \
           Python/frozen_modules/runpy.h:runpy \
           Python/frozen_modules/__hello__.h:__hello__ \
           Python/frozen_modules/__phello__.h:__phello__ \
           Python/frozen_modules/__phello__.ham.h:__phello__.ham \
           Python/frozen_modules/__phello__.ham.eggs.h:__phello__.ham.eggs \
           Python/frozen_modules/__phello__.spam.h:__phello__.spam \
           Python/frozen_modules/frozen_only.h:frozen_only \
           -o $@
