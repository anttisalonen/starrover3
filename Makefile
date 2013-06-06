CC       ?= g++
AR       ?= ar
CFLAGS   ?= -O2 -g3 -Werror
CFLAGS   += -Wall -Wshadow

CFLAGS   += -Isrc
BINDIR    = bin


# main

MAINNAME    = main
MAINBIN     = $(BINDIR)/$(MAINNAME)
MAINSRCDIR  = src
MAINSRCFILES = main.c

MAINSRCS = $(addprefix $(MAINSRCDIR)/, $(MAINSRCFILES))
MAINOBJS = $(MAINSRCS:.c=.o)
MAINDEPS = $(MAINSRCS:.c=.dep)

.PHONY: clean all

all: $(MAINBIN)

$(BINDIR):
	mkdir -p $(BINDIR)

$(MAINBIN): $(BINDIR) $(MAINOBJS)
	$(CC) $(LDFLAGS) $(MAINOBJS) -o $(MAINBIN)

%.dep: %.c
	@rm -f $@
	@$(CC) -MM $(CFLAGS) $< > $@.P
	@sed 's,\($(notdir $*)\)\.o[ :]*,$(dir $*)\1.o $@ : ,g' < $@.P > $@
	@rm -f $@.P

clean:
	find src/ -name '*.o' -exec rm -rf {} +
	find src/ -name '*.dep' -exec rm -rf {} +
	find src/ -name '*.a' -exec rm -rf {} +
	rm -rf $(MAINBIN)
	rmdir $(BINDIR)

-include $(MAINDEPS)

