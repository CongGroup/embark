TEX = xelatex
BIB = bibtex

.PHONY: clean

mb.pdf: mb.tex mb.bib
	$(TEX) mb
	$(BIB) -min-crossrefs=10000 mb
	$(TEX) mb
	$(TEX) mb

mb.bib: 

clean:
	rm -rf *.log *.aux *.pdf *.bbl *.blg *.out *.synctex.gz