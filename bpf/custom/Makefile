# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2021 Authors of Cilium

include ../../Makefile.defs

.PHONY: all clean

BPF = bpf_custom.o
BPF_CUSTOM_PROG ?= bytecount.h

include ../Makefile.bpf

all: $(BPF)

%.ll: %.c $(LIB)
	@$(ECHO_CC)
	$(QUIET) ${CLANG} ${CLANG_FLAGS} -DBPF_CUSTOM_PROG=${BPF_CUSTOM_PROG} -c $< -o $@

$(BPF): %.o: %.ll
	@$(ECHO_CC)
	$(QUIET) ${LLC} ${LLC_FLAGS} -filetype=obj -o $@ $(patsubst %.o,%.ll,$@)

clean:
	@$(ECHO_CLEAN)
	$(QUIET)rm -fr *.o *.ll *.i
