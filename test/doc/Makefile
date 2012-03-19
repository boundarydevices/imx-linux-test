DOCMAIN :=$(wildcard *.toc)

all: pre-build chunked single

pre-build:
	./gen_conf.sh $(PLATFORM)

chunked: $(DOCMAIN)
	asciidoc -a toc $<
ifneq ($(OBJDIR),)
	cp -f *.html $(OBJDIR)
endif

single:

pdf: $(DOCMAIN)
	a2x -dbook -f pdf $<
ifneq ($(OBJDIR),)
	cp -f *.pdf $(OBJDIR)
endif

.PHONY: clean
clean:
	rm -f *.pdf *.html
