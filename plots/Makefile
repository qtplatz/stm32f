
all: putty.pdf
#all: 2018-03-19.pdf temp.pdf

2018-03-19.tex: 2018-03-19.txt bmp280.gnuplot 
	gnuplot -c bmp280.gnuplot $@ $< $(basename $@)

temp.tex: ${HOME}/temp.txt bmp280.gnuplot 
	gnuplot -c bmp280.gnuplot $@ $< $(basename $@)

putty.tex: putty.txt bmp280.gnuplot
	gnuplot -c bmp280.gnuplot $@ $< $(basename $@)

%.pdf: %.tex ; pdflatex $<

clean:
	rm -f *~ *.log *-inc-eps-converted-to.pdf *.tex *.aux *.eps

.PHONY: clean
