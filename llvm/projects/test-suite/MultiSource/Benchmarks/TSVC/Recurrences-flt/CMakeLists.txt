set(PROG Recurrences-flt)
list(APPEND LDFLAGS -lm)
list(APPEND CFLAGS -std=gnu99)
set(RUN_OPTIONS 20000 5)
llvm_multisource()
