DOCMAIN :=$(wildcard *.toc)

all: pre-build single text chunked

pre-build:
	./gen_conf.sh $(PLATFORM)

single: $(DOCMAIN)
	asciidoc -a toc $<
ifneq ($(OBJDIR),)
	cp -f *.html $(OBJDIR)
endif

chunked:

pdf: $(DOCMAIN)
	a2x -dbook -f pdf $<
ifneq ($(OBJDIR),)
	cp -f *.pdf $(OBJDIR)
endif

text: $(DOCMAIN)
	a2x -dbook -f text $<
ifneq ($(OBJDIR),)
	cp -f *.text $(OBJDIR)
endif

.PHONY: clean
clean:
	rm -f *.pdf *.html
