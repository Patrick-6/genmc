include ../benchmarks.mk

TESTNAME = casw4

all: $(TESTNAME)

$(TESTNAME): $(TESTNAME).c
	$(CC) -o $@ $< $(CFLAGS) -DN=4 -std=c11 $(LDFLAGS)

clean:
	rm -f $(TESTNAME) *.o
