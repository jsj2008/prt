if EXAMPLES
  EXAMPLE_OPT = examples
endif

SUBDIRS=src tests $(EXAMPLE_OPT)

format-source:
	tools/clang-format-all

.PHONY : format-source
ACLOCAL_AMFLAGS=-I m4
