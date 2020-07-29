#!/usr/bin/awk -f


BEGIN {
	if (ARGC != 2) {
		print "Usage: modify_source.awk <input.litmus>" > "/dev/stderr";
		exit_invoked = 1;
		exit 1;
	}

	input_file = ARGV[1];
	output_file = ARGV[3];
	line_count = 0;
	in_comment = 0;

	header = "#include <stdlib.h>\n#include <lkmm.h>\n#include <pthread.h>\n#include <assert.h>\n"
	program = "";
	num_threads = 0;

	print "--- Modifying file:", ARGV[1] > "/dev/stderr";
}

{
	++line_count;

	## Do not collect inits
	if (line_count == 1 || match($0, "{}") != 0)
		next;

	## Do not collect "exists" and "always"
	if (match($0, "exists|always|locations"))
		next;

	## Remove derefence from ONCEs and collect variables
	r = "([READ|WRITE]_ONCE)\\(\\*(\\w+)(.*;)"
	if (match($0, r, a)) {
		++global_variables[a[2]];
		sub(r, a[1] "(" a[2] a[3]);
	}

	## Remove comments
	if (match($0, "\\(\\*") != 0) {
		in_comment = 1;
		next;
	}
	if (match($0, "\\*\\)") != 0) {
		in_comment = 0;
		next;
	}
	if (in_comment == 1)
		next;

	## Collect variables from acquires, releases, etc
	r = "(smp_store_release|smp_load_acquire)\\((\\w+)(.*;)"
	if (match($0, r, a)) {
		++global_variables[a[2]];
		sub(r, a[1] "(\\&" a[2] a[3]);
	}

	## Collect variables from spinlocks
	r = "(spin_lock|spin_unlock)\\((\\w+)(.*;)"
	if (match($0, r, a)) {
		++spinlocks[a[2]];
		sub(r, a[1] "(\\&" a[2] a[3]);
	}

	## Change the way threads are printed
	r = "(P[0-9]+)\\(.*";
	if (match($0, r, a ) != 0) {
		++num_threads;
		sub(r, "void *" a[1] "(void *unused)");
	}

	## Collect line
	program = program "\n" $0;
}

END {
	if (exit_invoked)
		exit 1;

	## Print header
	printf header "\n";

	## atomic variables and spinlocks
	for (v in global_variables)
		print "atomic_t " v ";"
	for (s in spinlocks)
		print "spinlock_t " s ";"

	## print threads
	print program "\n";

	## print main
	printf("int main()\n{\n\tpthread_t ");
	for (i = 0; i < num_threads - 1; i++)
		printf("t%d, ", i);
	print "t" (num_threads - 1) ";\n"

	for (i = 0; i < num_threads; i++)
		print "\tpthread_create(&t" i ", NULL, P" i ", NULL);";

	print "\n\treturn 0;\n}"
}
