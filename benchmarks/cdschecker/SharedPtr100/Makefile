include ../benchmarks.mk

TESTNAME = SharedPtr100

all: $(TESTNAME)

$(TESTNAME): $(TESTNAME).c
	$(CC) -o $@ $< $(CFLAGS) -std=c11 -DN=100 $(LDFLAGS)

clean:
	rm -f $(TESTNAME) *.o
