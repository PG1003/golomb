# This makefile is based on https://gist.github.com/maxtruxa/4b3929e118914ccef057f8a05c614b0f
# ... and mangled by PG1003

SRCDIR  = src
UTILDIR = util
TESTDIR = tests

# source files
SRCS := $(shell find $(UTILDIR) -type f -name '*.cpp')
TESTSRCS := $(shell find $(TESTDIR) -type f -name '*.cpp')

# intermediate directory for generated dependency and object files
OBJDIR := obj

# object files, auto generated from source files
OBJS := $(patsubst %,$(OBJDIR)/%.o,$(basename $(SRCS)))
TESTOBJS := $(patsubst %,$(OBJDIR)/%.o,$(basename $(TESTSRCS)))
# dependency files, auto generated from source files
DEPS := $(patsubst %,$(OBJDIR)/%.d,$(basename $(SRCS)))
TESTDEPS := $(patsubst %,$(OBJDIR)/%.d,$(basename $(TESTSRCS)))

# compilers (at least gcc and clang) don't create the subdirectories automatically
$(shell mkdir -p $(dir $(OBJS)) >/dev/null)
$(shell mkdir -p $(dir $(TESTOBJS)) >/dev/null)

# C++ compiler
CXX := g++
# linker
LD := g++

# C++ flags
CXXFLAGS := -std=c++20
# C/C++ flags
CPPFLAGS := -Wall -Wextra -Wpedantic -O3
# Extra include directories
INCLUDES = -I "./src"
# linker flags
LDFLAGS :=
# linker flags: libraries to link (e.g. -lfoo)
LDLIBS :=
# flags required for dependency generation; passed to compilers
DEPFLAGS = -MT $@ -MD -MP -MF $*.d

# compile C++ source files
COMPILE.cc = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(INCLUDES) -c -o $@
# link object files to binary
LINK.o = $(LD) $(LDFLAGS) $(LDLIBS) -o $@

.PHONY: all
all: golomb test

.PHONY: clean
clean:
	$(RM) -r $(OBJDIR)

.PHONY: golomb
golomb: $(OBJDIR)/golomb

$(OBJDIR)/golomb: $(OBJS)
	$(LINK.o) $^

.PHONY: test
test: $(OBJDIR)/test

$(OBJDIR)/test: $(TESTOBJS)
	$(LINK.o) $^

.PHONY: run_tests
run_tests: test golomb
	@echo "Running tests..."
	@echo "> Generic tests"
	@cd $(OBJDIR); ./test && : || { echo ">>> Generic tests failed!"; exit 1; }
	@echo "> Roundtrip unsigned 8"
	@cd $(OBJDIR); ./golomb -eu8 -k0 ../$(TESTDIR)/u8.bin u8.egc && : || { echo ">>> golomb encode u8 failed!";  exit 1; }
	@cd $(OBJDIR); ./golomb -du8 -k0 u8.egc u8.bin && : || { echo ">>> golomb decode u8 failed!";  exit 1; }
	@cd $(OBJDIR); cmp -s ../$(TESTDIR)/u8.bin u8.bin && : || { echo ">>> Roundtrip u8 failed!";  exit 1; }
	@echo "> Roundtrip signed 8"
	@cd $(OBJDIR); ./golomb -ei8 -k1 ../$(TESTDIR)/i8.bin i8.egc && : || { echo ">>> golomb encode i8 failed!";  exit 1; }
	@cd $(OBJDIR); ./golomb -di8 -k1 i8.egc i8.bin && : || { echo ">>> golomb decode i8 failed!";  exit 1; }
	@cd $(OBJDIR); cmp -s ../$(TESTDIR)/i8.bin i8.bin && : || { echo ">>> Roundtrip i8 failed!";  exit 1; }
	@echo "> Roundtrip unsigned 16"
	@cd $(OBJDIR); ./golomb -eu16 -k2 ../$(TESTDIR)/u16.bin u16.egc && : || { echo ">>> golomb encode u16 failed!";  exit 1; }
	@cd $(OBJDIR); ./golomb -du16 -k2 u16.egc u16.bin && : || { echo ">>> golomb decode u16 failed!";  exit 1; }
	@cd $(OBJDIR); cmp -s ../$(TESTDIR)/u16.bin u16.bin && : || { echo ">>> Roundtrip u16 failed!";  exit 1; }
	@echo "> Roundtrip signed 16"
	@cd $(OBJDIR); ./golomb -ei16 -k0 ../$(TESTDIR)/i16.bin i16.egc && : || { echo ">>> golomb encode i16 failed!";  exit 1; }
	@cd $(OBJDIR); ./golomb -di16 -k0 i16.egc i16.bin && : || { echo ">>> golomb decode i16 failed!";  exit 1; }
	@cd $(OBJDIR); cmp -s ../$(TESTDIR)/i16.bin i16.bin && : || { echo ">>> Roundtrip i16 failed!";  exit 1; }
	@echo "> Roundtrip unsigned 32"
	@cd $(OBJDIR); ./golomb -eu32 -k0 ../$(TESTDIR)/u32.bin u32.egc && : || { echo ">>> golomb encode u32 failed!";  exit 1; }
	@cd $(OBJDIR); ./golomb -du32 -k0 u32.egc u32.bin && : || { echo ">>> golomb decode u32 failed!";  exit 1; }
	@cd $(OBJDIR); cmp -s ../$(TESTDIR)/u32.bin u32.bin && : || { echo ">>> Roundtrip u32 failed!";  exit 1; }
	@echo "> Roundtrip signed 32"
	@cd $(OBJDIR); ./golomb -ei32 -k3 ../$(TESTDIR)/i32.bin i32.egc && : || { echo ">>> golomb encode i32 failed!";  exit 1; }
	@cd $(OBJDIR); ./golomb -di32 -k3 i32.egc i32.bin && : || { echo ">>> golomb decode i32 failed!";  exit 1; }
	@cd $(OBJDIR); cmp -s ../$(TESTDIR)/i32.bin i32.bin && : || { echo ">>> Roundtrip i32 failed!";  exit 1; }
	@echo "> Roundtrip unsigned 64"
	@cd $(OBJDIR); ./golomb -eu64 -k4 ../$(TESTDIR)/u64.bin u64.egc && : || { echo ">>> golomb encode u64 failed!";  exit 1; }
	@cd $(OBJDIR); ./golomb -du64 -k4 u64.egc u64.bin && : || { echo ">>> golomb decode u64 failed!";  exit 1; }
	@cd $(OBJDIR); cmp -s ../$(TESTDIR)/u64.bin u64.bin && : || { echo ">>> Roundtrip u64 failed!";  exit 1; }
	@echo "> Roundtrip signed 64"
	@cd $(OBJDIR); ./golomb -ei64 -k0 ../$(TESTDIR)/i64.bin i64.egc && : || { echo ">>> golomb encode i64 failed!";  exit 1; }
	@cd $(OBJDIR); ./golomb -di64 -k0 i64.egc i64.bin && : || { echo ">>> golomb decode i64 failed!";  exit 1; }
	@cd $(OBJDIR); cmp -s ../$(TESTDIR)/i64.bin i64.bin && : || { echo ">>> Roundtrip i64 failed!";  exit 1; }
	@echo "> Roundtrip unsigned 8 adaptive 0"
	@cd $(OBJDIR); ./golomb -eu8 -k0 -a0 ../$(TESTDIR)/u8.bin u8a0.egc && : || { echo ">>> golomb encode u8 a0 failed!";  exit 1; }
	@cd $(OBJDIR); ./golomb -du8 -k0 -a0 u8a0.egc u8a0.bin && : || { echo ">>> golomb decode u8 a0 failed!";  exit 1; }
	@cd $(OBJDIR); cmp -s ../$(TESTDIR)/u8.bin u8a0.bin && : || { echo ">>> Roundtrip u8 a0 failed!";  exit 1; }
	@echo "> Roundtrip signed 8 adaptive 1"
	@cd $(OBJDIR); ./golomb -ei8 -k1 -a1 ../$(TESTDIR)/i8.bin i8a1.egc && : || { echo ">>> golomb encode i8 a1 failed!";  exit 1; }
	@cd $(OBJDIR); ./golomb -di8 -k1 -a1 i8a1.egc i8a1.bin && : || { echo ">>> golomb decode i8 a1 failed!";  exit 1; }
	@cd $(OBJDIR); cmp -s ../$(TESTDIR)/i8.bin i8a1.bin && : || { echo ">>> Roundtrip i8 a1 failed!";  exit 1; }
	@echo "> Roundtrip unsigned 16 adaptive 2"
	@cd $(OBJDIR); ./golomb -eu16 -k2 -a2 ../$(TESTDIR)/u16.bin u16a2.egc && : || { echo ">>> golomb encode u16 a2 failed!";  exit 1; }
	@cd $(OBJDIR); ./golomb -du16 -k2 -a2 u16a2.egc u16a2.bin && : || { echo ">>> golomb decode u16 a2 failed!";  exit 1; }
	@cd $(OBJDIR); cmp -s ../$(TESTDIR)/u16.bin u16a2.bin && : || { echo ">>> Roundtrip u16 a2 failed!";  exit 1; }
	@echo "> Roundtrip signed 16 adaptive 3"
	@cd $(OBJDIR); ./golomb -ei16 -k3 -a3 ../$(TESTDIR)/i16.bin i16a3.egc && : || { echo ">>> golomb encode i16 a3 failed!";  exit 1; }
	@cd $(OBJDIR); ./golomb -di16 -k3 -a3 i16a3.egc i16a3.bin && : || { echo ">>> golomb decode i16 a3 failed!";  exit 1; }
	@cd $(OBJDIR); cmp -s ../$(TESTDIR)/i16.bin i16a3.bin && : || { echo ">>> Roundtrip i16 a3 failed!";  exit 1; }
	@echo "> Roundtrip unsigned 32 adaptive 3"
	@cd $(OBJDIR); ./golomb -eu32 -k3 -a3 ../$(TESTDIR)/u32.bin u32a3.egc && : || { echo ">>> golomb encode u32 a3 failed!";  exit 1; }
	@cd $(OBJDIR); ./golomb -du32 -k3 -a3 u32a3.egc u32a3.bin && : || { echo ">>> golomb decode u32 a3 failed!";  exit 1; }
	@cd $(OBJDIR); cmp -s ../$(TESTDIR)/u32.bin u32a3.bin && : || { echo ">>> Roundtrip u32 a3 failed!";  exit 1; }
	@echo "> Roundtrip signed 32 adaptive 2"
	@cd $(OBJDIR); ./golomb -ei32 -k2 -a2 ../$(TESTDIR)/i32.bin i32a2.egc && : || { echo ">>> golomb encode i32 a2 failed!";  exit 1; }
	@cd $(OBJDIR); ./golomb -di32 -k2 -a2 i32a2.egc i32a2.bin && : || { echo ">>> golomb decode i32 a2 failed!";  exit 1; }
	@cd $(OBJDIR); cmp -s ../$(TESTDIR)/i32.bin i32a2.bin && : || { echo ">>> Roundtrip i32 a2 failed!";  exit 1; }
	@echo "> Roundtrip unsigned 64 adaptive 1"
	@cd $(OBJDIR); ./golomb -eu64 -k1 -a1 ../$(TESTDIR)/u64.bin u64a1.egc && : || { echo ">>> golomb encode u64 a1 failed!";  exit 1; }
	@cd $(OBJDIR); ./golomb -du64 -k1 -a1 u64a1.egc u64a1.bin && : || { echo ">>> golomb decode u64 a1 failed!";  exit 1; }
	@cd $(OBJDIR); cmp -s ../$(TESTDIR)/u64.bin u64a1.bin && : || { echo ">>> Roundtrip u64 a1 failed!";  exit 1; }
	@echo "> Roundtrip signed 64 adaptive 0"
	@cd $(OBJDIR); ./golomb -ei64 -k0 -a0 ../$(TESTDIR)/i64.bin i64a0.egc && : || { echo ">>> golomb encode i64 a0 failed!";  exit 1; }
	@cd $(OBJDIR); ./golomb -di64 -k0 -a0 i64a0.egc i64a0.bin && : || { echo ">>> golomb decode i64 a0 failed!";  exit 1; }
	@cd $(OBJDIR); cmp -s ../$(TESTDIR)/i64.bin i64a0.bin && : || { echo ">>> Roundtrip i64 a0 failed!";  exit 1; }
	@echo ""
	@echo "...tests completed"
	@echo "      _"
	@echo "     /(|"
	@echo "    (  :"
	@echo "   __\  \  _____"
	@echo " (____)  '|"
	@echo "(____)|   |"
	@echo " (____).__|"
	@echo "  (___)__.|_____"
# https://asciiart.website/index.php?art=people/body%20parts/hand%20gestures


$(OBJS): $(SRCS)
$(OBJS): $(SRCS) $(DEPS)
	$(COMPILE.cc) $<

$(TESTOBJS): $(TESTSRCS)
$(TESTOBJS): $(TESTSRCS) $(TESTDEPS)
	$(COMPILE.cc) $<

.PRECIOUS: $(OBJDIR)/%.d
$(OBJDIR)/%.d: ;

-include $(DEPS) 
-include $(TESTDEPS) 
