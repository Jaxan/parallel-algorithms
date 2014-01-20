
.PHONY: report fast images

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

fast:
	mkdir -p build
	cp references.bib build/
	pdflatex -output-directory=build report.tex
	cp build/report.pdf ./

images:
	mkdir -p build
	pdflatex -output-directory=build images.tex
	pdflatex -output-directory=build images.tex
	scp build/images.pdf moerman@stitch.science.ru.nl:~/wvlt_images.pdf
	ssh moerman@stitch.science.ru.nl 'pdf2svg wvlt_images.pdf wvlt_images.svg'
	scp moerman@stitch.science.ru.nl:~/wvlt_images.svg ./images.svg

