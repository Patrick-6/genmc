SUBDIRS = src

.PHONY: test ftest
test:
	$(MAKE) -C src
	cd $(top_builddir)/tests && ./driver.sh

ftest:
	$(MAKE) -C src
	cd $(top_builddir)/tests && ./driver.sh --fast
