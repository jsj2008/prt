if EXAMPLES
  EXAMPLE_OPT = examples
endif

SUBDIRS=prt tests $(EXAMPLE_OPT)

format-source:
	tools/clang-format-all

.PHONY : format-source
ACLOCAL_AMFLAGS=-I m4
