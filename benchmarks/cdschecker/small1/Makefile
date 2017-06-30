include ../benchmarks.mk

TESTNAME = small1

all: $(TESTNAME)

$(TESTNAME): $(TESTNAME).c
	$(CC) -o $@ $< $(CFLAGS) -std=c11 $(LDFLAGS)

clean:
	rm -f $(TESTNAME) *.o
