DOCMAIN :=$(wildcard *.toc)
PLATFORMLOWER := $(shell echo $(PLATFORM) | tr A-Z a-z)

all: chunked single

chunked: $(DOCMAIN)
	echo "// Don't edit it manually!" > def.cfg
	echo ":chip: $(PLATFORM)" >> def.cfg
	echo ":$(PLATFORMLOWER):" >> def.cfg
	asciidoc -a toc $<
	cp -f *.html $(OBJDIR)
single:

pdf: $(DOCMAIN)
	a2x -dbook -f pdf $<
	cp -f *.pdf $(OBJDIR)

.PHONY: clean
clean:
	rm -f *.pdf *.html
