
all: 2018-03-18.pdf

2018-03-18.tex: bmp280.gnuplot 2018-03-18.txt
	gnuplot -c $< $@ $(basename $@).txt $(basename $@)

%.pdf: %.tex ; pdflatex $<

clean:
	rm -f *~ *.log *-inc-eps-converted-to.pdf *.tex *.aux *.eps

.PHONY: clean
