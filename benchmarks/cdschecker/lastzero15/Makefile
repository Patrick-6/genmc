include ../benchmarks.mk

TESTNAME = lastzero15

all: $(TESTNAME)

$(TESTNAME): $(TESTNAME).c
	$(CC) -o $@ $< $(CFLAGS) -std=c11 -DN=15 $(LDFLAGS)

clean:
	rm -f $(TESTNAME) *.o
