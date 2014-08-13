TARGETS = \
  src/xlib/Makefile \
  src/demo/Makefile

.PHONY : all clean

MAKE_CMD = $(MAKE) RELDIR=$$(dirname $(1)) --no-print-directory -f$(1) $(2)

all :
	@mkdir -p bin
	@+$(foreach t,$(TARGETS),$(call MAKE_CMD,$(t),);)

clean :
	@+$(foreach t,$(TARGETS),$(call MAKE_CMD,$(t),clean);)
	@rm -rf bin obj
