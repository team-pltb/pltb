UNAME := $(shell uname)
TARGET=pltb.out
CC=gcc
MCC=MPICH_CC=$(CC) OMPI_CC=$(CC) mpicc
ifeq ($(UNAME), Darwin)
LFLAGS=-lm
else
LFLAGS=-lm -lrt
endif
CFLAGS=-c -O3 -std=gnu99 -Wall -Wextra -Wredundant-decls -Wswitch-default \
-Wimport -Wno-int-to-pointer-cast -Wbad-function-cast \
-Wmissing-declarations -Wmissing-prototypes -Wnested-externs \
-Wstrict-prototypes -Wformat-nonliteral -Wundef

.PHONY: default all clean

default: avx

clang: CC := clang
clang: CFLAGS += -Weverything -pedantic
clang: LFLAGS += -l pll-avx
clang: $(TARGET)

avx: LFLAGS += -l pll-avx
avx: $(TARGET)

avx-pthreads: LFLAGS += -l pll-avx-pthreads
avx-pthreads: $(TARGET)

sse3: LFLAGS += -l pll-sse3
sse3: $(TARGET)

sse3-pthreads: LFLAGS += -l pll-sse3-pthreads
sse3-pthreads: $(TARGET)

debug: CFLAGS += -DDEBUG -g -O0
debug: CFLAGS := $(filter-out -O3,$(CFLAGS))
debug: LFLAGS += -l pll-avx
debug: $(TARGET)

debug-sse3: CFLAGS += -DDEBUG -g -O0
debug-sse3: CFLAGS := $(filter-out -O3,$(CFLAGS))
debug-sse3: LFLAGS += -l pll-sse3
debug-sse3: $(TARGET)

OBJECTS = $(patsubst src/%.c,src/%.o,$(wildcard src/*.c))
HEADERS = $(wildcard src/*.h)

%.o: %.c $(HEADERS)
	$(MCC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(MCC) $(OBJECTS) $(LFLAGS) -o $@

clean:
	-rm -f src/*.o
	-rm -f $(TARGET)
