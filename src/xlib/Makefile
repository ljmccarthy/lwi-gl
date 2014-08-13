TARGET = bin/liblwi.a
SRCDIRS = src/xlib src
INCDIRS = include src/misc
OBJDIR = obj
LIBS = pthread rt X11 Xrender Xxf86vm Xfixes GL GLU
DEFINES = _BSD_SOURCE _GNU_SOURCE LWI_BUILD_CAIRO

PKGS = cairo libpng
PKG_CFLAGS = $(foreach pkg,$(PKGS),$(shell pkg-config --cflags $(pkg)))
PKG_LIBS = $(foreach pkg,$(PKGS),$(shell pkg-config --libs $(pkg)))

CFLAGS = -std=c99 -Wall -W $(PKG_CFLAGS)
LINKFLAGS = -fvisibility=hidden -fvisibility-inlines-hidden

ifeq ($(strip $(DEBUG)),)
  CFLAGS += -Os -fomit-frame-pointer -fvisibility=hidden
  DEFINES += NDEBUG
else
  CFLAGS += -ggdb
endif

CFLAGS += $(DEFINES:%=-D%) $(INCDIRS:%=-I%)
LINKAGE = $(LIBDIRS:%=-L%) $(LIBS:%=-l%)
CEE_SRC = $(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.c))
CEE_DEP = $(CEE_SRC:%=$(OBJDIR)/%.d)
CEE_OBJ = $(CEE_SRC:%=$(OBJDIR)/%.o)

.PHONY : all clean

all : $(TARGET) bin/liblwi.flags
	@exit 0

$(TARGET) : $(CEE_OBJ)
	@echo " LIB      $@"
	@rm -f $@
	@ar cru $@ $(CEE_OBJ)
	@ranlib $@
clean :
	@rm -f $(TARGET) $(CEE_OBJ) $(CEE_DEP)

bin/liblwi.flags : $(TARGET)
	@echo -n "$(strip $(LINKAGE) $(PKG_LIBS))" > $@

$(OBJDIR)/%.c.o : %.c
	@echo " GCC      $<"
	@gcc -c -pipe -fPIC $(CFLAGS) -o $@ $<

$(OBJDIR)/%.c.d : %.c
	@mkdir -p $(dir $@)
	@gcc -MM -MP -MT "$(OBJDIR)/$<.o" $(CFLAGS) $< > $@ \
	  || (rm -f $@; exit 1)

ifneq ($(MAKECMDGOALS),clean)
-include $(CEE_DEP)
endif
