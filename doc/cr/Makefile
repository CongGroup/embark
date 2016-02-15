TEX = xelatex
BIB = bibtex

.PHONY: clean diff

mb.pdf: mb.tex 
	$(TEX) mb
	$(BIB) -min-crossrefs=10000 mb
	$(TEX) mb
	$(TEX) mb

mb.bib: 


diff: diff.tex diff.bbl
	$(TEX) diff
    
diff.tex:
	latexdiff --flatten ../nsdi16/mb.tex mb.tex > diff.tex
	
diff.bbl: mb.pdf
	latexdiff ../nsdi16/mb.bbl mb.bbl > diff.bbl

clean:
	rm -rf *.log *.aux *.pdf *.bbl *.blg *.out *.synctex.gz
