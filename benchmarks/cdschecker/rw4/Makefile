include ../benchmarks.mk

TESTNAME = rw4

all: $(TESTNAME)

$(TESTNAME): $(TESTNAME).c
	$(CC) -o $@ $< $(CFLAGS) -std=c11 -DN=4 $(LDFLAGS)

clean:
	rm -f $(TESTNAME) *.o
