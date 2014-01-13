
.PHONY: report

# We don want to pollute the root dir, so we use a build dir
# http://tex.stackexchange.com/questions/12686/how-do-i-run-bibtex-after-using-the-output-directory-flag-with-pdflatex-when-f
report:
	mkdir -p build
	cp references.bib build/
	pdflatex -output-directory=build report.tex
	cd build; bibtex report
	pdflatex -output-directory=build report.tex
	pdflatex -output-directory=build report.tex
	cp build/report.pdf ./
