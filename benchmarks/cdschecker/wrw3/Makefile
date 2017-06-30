include ../benchmarks.mk

TESTNAME = wrw3

all: $(TESTNAME)

$(TESTNAME): $(TESTNAME).c
	$(CC) -o $@ $< $(CFLAGS) -std=c11 -DN=3 $(LDFLAGS)

clean:
	rm -f $(TESTNAME) *.o
