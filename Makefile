# Copyright © 2017 Jakub Wilk <jwilk@jwilk.net>
# SPDX-License-Identifier: MIT

CC ?= gcc
CFLAGS ?= -g -O2
CFLAGS += -Wall -Wextra

ifeq "$(shell id -u)" "0"
SUDO =
else
SUDO = sudo
endif

PREFIX = /usr/local
DESTDIR =

.PHONY: all
all: netaway

netaway: netaway.c

.PHONY: install
install: netaway
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m755 $(<) $(DESTDIR)$(PREFIX)/bin/$(<)
	$(SUDO) setcap cap_sys_admin+ep $(DESTDIR)$(PREFIX)/bin/$(<)

.PHONY: clean
clean:
	rm -f netaway *.o

# vim:ts=4 sts=4 sw=4 noet
