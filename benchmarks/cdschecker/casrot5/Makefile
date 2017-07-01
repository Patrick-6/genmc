include ../benchmarks.mk

TESTNAME = casrot5

all: $(TESTNAME)

$(TESTNAME): $(TESTNAME).c
	$(CC) -o $@ $< $(CFLAGS) -DN=5 -std=c11 $(LDFLAGS)

clean:
	rm -f $(TESTNAME) *.o
