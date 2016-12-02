###############################################################################
# Copyright (c) 2010 Linaro Limited
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#     Peter Maydell (Linaro) - initial implementation
#     Claudio Fontana (Linaro) - small refactoring and aarch64 support
###############################################################################

# import the variables generated by configure
include Makefile.in

CFLAGS ?= -g -Wall

PROG=risu
SRCS=risu.c comms.c risu_$(ARCH).c risu_reginfo_$(ARCH).c
HDRS=risu.h
BINS=test_$(ARCH).bin

# For dumping test patterns
RISU_BINS=$(wildcard *.risu.bin)
RISU_ASMS=$(patsubst %.bin,%.asm,$(RISU_BINS))

OBJS=$(SRCS:.c=.o)

all: $(PROG) $(BINS)

dump: $(RISU_ASMS)

$(PROG): $(OBJS)
	$(CC) $(STATIC) $(CFLAGS) -o $@ $^

%.risu.asm: %.risu.bin
	${OBJDUMP} -b binary -m $(ARCH) -D $^ > $@

# hand-coded tests
%.risu.bin: %.risu.elf
	$(OBJCOPY) -O binary $< $@

%.risu.elf: %.risu.S
	${AS} -o $@ $^

%.o: %.c $(HDRS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<

%_$(ARCH).bin: %_$(ARCH).elf
	$(OBJCOPY) -O binary $< $@

%_$(ARCH).elf: %_$(ARCH).s
	$(AS) -o $@ $<

clean:
	rm -f $(PROG) $(OBJS) $(BINS)
