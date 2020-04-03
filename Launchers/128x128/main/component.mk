#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

COMPILEDATE:=\"$(shell date "+%Y%m%d")\"
GITREV:=\"$(shell git rev-parse HEAD | cut -b 1-10)\"

CFLAGS += -DCOMPILEDATE="$(COMPILEDATE)" -DGITREV="$(GITREV)"