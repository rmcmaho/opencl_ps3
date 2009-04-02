all: article

article:
	pdflatex opencl_ps3
	bibtex opencl_ps3
	pdflatex opencl_ps3
	pdflatex opencl_ps3

clean:
	-rm -f *.aux *.log *.nav *.out *.snm *.toc *.vrb *.ps *.dvi *.bbl *.blg
	-rm -f *~
