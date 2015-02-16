TEX = xelatex
BIB = bibtex

.PHONY: clean

mbark.pdf: mbark.tex mbark.bib
	$(TEX) mbark
	$(BIB) mbark
	$(TEX) mbark
	$(TEX) mbark

mbark.bib: 

clean:
	rm -rf *.log *.aux *.pdf *.bbl *.blg