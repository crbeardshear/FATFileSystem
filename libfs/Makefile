# Target library
lib := libfs.a

all: $(lib)

MAKEFLAGS	+= -rR

ifneq ($(V),1)
Q = @
V = 0
endif

# Current Dir
CUR_PWD := $(shell pwd)

# Compiler, targets for compilation
CC	:= gcc
LIB	:= ar rcs
LIBN	:= libfs.a
LIBCL	:= fs.o disk.o
LIBTARG	:= fs.o disk.o

CFLAGS	:= -Wall -Werror

# Debug
ifneq ($(D),1)
CFLAGS	+= -O2
else
CFLAGS	+= -O0
CFLAGS	+= -g
endif

$(LIBN): $(LIBCL)
	@echo "AR	$@"
	$(Q)$(LIB) $(LIBN) $(LIBTARG)

fs.o: fs.c
	@echo "CC	$@"
	$(Q)$(CC) $(CFLAGS) -c -o $@ $<

disk.o: disk.c
	@echo "CC	$@"
	$(Q)$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@echo "CLEAN	$(CUR_PWD)"
	$(Q)rm -rf $(LIBN) $(LIBCL)

