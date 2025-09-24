# @file
# @copyright 2025 Terry Golubiewski, all rights reserved.
# @author Terry Golubiewski

export PROJNAME:= Int
export PROJDIR := $(abspath ..)
include $(SWDEV)/project.mk

TEST_INT_EXE=TestInt$(DBGSFX).$E
TGT1=$(TEST_INT_EXE)
TARGETS=$(TGT1)

SRC1 := TestInt.cpp
SOURCE := $(SRC1)

APP:=$(abspath $(HOME)/App)
GSL:=$(APP)/GSL
GTEST:=$(APP)/googletest

#PROJ_SRC:=$(PROJDIR)/src
#PROJ_LIB:=$(PROJDIR)/lib
#PROJ_INC:=$(PROJDIR)/include
PROJ_INC:=$(PROJDIR)

GTEST_LIB:=$(GTEST)/build/lib
GTEST_INC:=$(GTEST)/googletest

SYSINCL:=$(addsuffix /include, $(GTEST_INC) $(GSL))
INCLUDE:=$(PROJ_INC)
LIBPATH:=$(GTEST_LIB)

SHELL:=/bin/bash
.SHELLFLAGS:=-eu -o pipefail -c
.ONESHELL:

include $(SWDEV)/$(COMPILER).mk
include $(SWDEV)/build.mk

LDLIBS+=-lgtest -lgtest_main -lpthread

.PHONY: all clean scour asan test success

all: $(TARGETS)

log/IntConv.log: RunIntConv.bash IntConv.cpp ../Int.hpp
	@set -v
	./RunIntConv.bash $(notdir $@)
	mkdir -p "$(dir $@)"
	mv "$(notdir $@)" "$@"

TEST_RESULTS:=IntConv.txt TestInt.txt

CLEAN+=$(TEST_RESULTS)

CLEAN += log

LOGFILES:=$(addprefix log/, IntConv.log TestInt.json)

log/%.json: %.$E
	@set -v
	./$< --gtest_output=json:$(notdir $@)
	mkdir -p $(dir $@)
	mv $(notdir $@) $@

success:
	@echo "All tests passed."

test: depend $(LOGFILES) success

asan:
	$(MAKE) clean
	$(MAKE) CXXFLAGS+=' -fsanitize=address,undefined -fno-omit-frame-pointer' LDFLAGS+=' -fsanitize=address,undefined -fno-omit-frame-pointer'

$(TGT1): $(OBJ1) $(LIBS)
	$(LINK)
