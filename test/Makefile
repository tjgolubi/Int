# @file
# @copyright 2025 Terry Golubiewski, all rights reserved.
# @author Terry Golubiewski

export PROJDIR := $(abspath ..)
include $(SWDEV)/project.mk

TEST_INT_EXE=TestInt$(DBGSFX).$E
TARGET1=$(TEST_INT_EXE)
TARGETS=$(TARGET1)

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

LDLIBS=-lgtest -lgtest_main -lpthread
LIBPATH=$(GTEST_LIB)

SYSINCL:=$(addsuffix /include, $(GTEST_INC) $(GSL))
INCLUDE:=$(PROJ_INC)

include $(SWDEV)/$(COMPILER).mk
include $(SWDEV)/build.mk

.ONESHELL:

.PHONY: all clean scour test asan depend

all: $(TARGETS)

IntConv.txt: RunIntConv.bash IntConv.cpp ../Int.hpp
	@echo "Running $<"
	./RunIntConv.bash > $@ 2>&1
	x="$$?"
	tail -1 $@
	exit $$x

TestInt.txt: $(TEST_INT_EXE)
	@echo "Running $<"
	./$(TEST_INT_EXE) > $@ 2>&1
	x="$$?"
	tail -2 $@
	exit $$x

TEST_RESULTS:=IntConv.txt TestInt.txt

CLEAN+=$(TEST_RESULTS)

test: depend $(TEST_RESULTS)

asan:
	$(MAKE) clean
	$(MAKE) CXXFLAGS+=' -fsanitize=address,undefined -fno-omit-frame-pointer' LDFLAGS+=' -fsanitize=address,undefined -fno-omit-frame-pointer'

$(TARGET1): $(OBJ1) $(LIBS)
	$(LINK)
