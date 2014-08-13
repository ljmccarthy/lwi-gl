SRCDIR = src/demo
INCDIRS = include
OBJDIR = obj
LIBDIRS = bin
LIBS = GL GLU pthread
DEFINES = _BSD_SOURCE LWI_CAIRO

PKGS = cairo
PKG_CFLAGS = $(foreach pkg,$(PKGS),$(shell pkg-config --cflags $(pkg)))
PKG_LIBS = $(foreach pkg,$(PKGS),$(shell pkg-config --libs $(pkg)))

CFLAGS = -std=c99 -Wall -W $(PKG_CFLAGS)

ifeq ($(strip $(DEBUG)),)
  CFLAGS += -O2 -fomit-frame-pointer -fvisibility=hidden
  DEFINES += NDEBUG
else
  CFLAGS += -ggdb
endif

CFLAGS += $(DEFINES:%=-D%) $(INCDIRS:%=-I%)
LINKAGE = $(LIBDIRS:%=-L%) $(LIBS:%=-l%) $(shell cat bin/liblwi.flags) bin/liblwi.a
CEE_SRC = $(wildcard $(SRCDIR)/*.c)
CEE_DEP = $(CEE_SRC:%=$(OBJDIR)/%.d)
CEE_EXE = $(CEE_SRC:$(SRCDIR)/%.c=bin/%)

.PHONY : all clean

all : $(CEE_EXE)
	@exit 0

bin/% : $(OBJDIR)/$(SRCDIR)/%.c.o
	@echo " EXE      $@"
	@gcc -pipe -o $@ $< $(CFLAGS) $(PKG_LIBS) $(LINKAGE)
ifeq ($(strip $(DEBUG)),)
	@strip -s -R .comment $@
endif

clean :
	@rm -f $(CEE_EXE) $(CEE_DEP)

$(OBJDIR)/%.c.o : %.c
	@echo " GCC      $<"
	@gcc -c -pipe $(CFLAGS) -o $@ $<

$(OBJDIR)/%.c.d : %.c
	@mkdir -p $(dir $@)
	@gcc -MM -MP -MT $(<:%=$(OBJDIR)/%.o) $(CFLAGS) $< > $@ \
	  || (rm -f $@; exit 1)

ifneq ($(MAKECMDGOALS),clean)
-include $(CEE_DEP)
endif
