include ../benchmarks.mk

TESTNAME = coa2

all: $(TESTNAME)

$(TESTNAME): $(TESTNAME).c
	$(CC) -o $@ $< $(CFLAGS) -DN=2 -std=c11 $(LDFLAGS)

clean:
	rm -f $(TESTNAME) *.o
