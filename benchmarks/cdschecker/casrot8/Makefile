include ../benchmarks.mk

TESTNAME = casrot8

all: $(TESTNAME)

$(TESTNAME): $(TESTNAME).c
	$(CC) -o $@ $< $(CFLAGS) -DN=8 -std=c11 $(LDFLAGS)

clean:
	rm -f $(TESTNAME) *.o
