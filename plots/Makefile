
date = "`date +'20%y/%m/%d %H:%M'`"

all: temp.pdf temp-lt.pdf
#all: putty.pdf
#all: 2018-03-19.pdf temp.pdf

2018-03-19.tex: 2018-03-19.txt bmp280.gnuplot 
	gnuplot -c bmp280.gnuplot $@ $< $(basename $@)

temp.tex: ${HOME}/temp.txt bmp280.gnuplot 
	gnuplot -c bmp280.gnuplot $@ $< $(date)

temp-lt.tex: ${HOME}/temp-lt.txt bmp280.gnuplot
	gnuplot -c bmp280.gnuplot $@ $< $(date)

${HOME}/temp-lt.txt: ${HOME}/temp.txt ${HOME}/temp1.txt
	cat ${HOME}/temp1.txt ${HOME}/temp.txt > ${HOME}/temp-lt.txt

putty.tex: putty.txt bmp280.gnuplot
	gnuplot -c bmp280.gnuplot $@ $< $(basename $@)

%.pdf: %.tex ; pdflatex $<

clean:
	rm -f *~ *.log *-inc-eps-converted-to.pdf *.tex *.aux *.eps

.PHONY: clean
