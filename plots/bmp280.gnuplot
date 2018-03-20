set terminal epslatex size 6in,6in color colortext standalone header \
"\\usepackage{graphicx}\n\\usepackage{amsmath}\n\\usepackage[version=3]{mhchem}\n\\usepackage{siunitx}"

set output ARG1
logfile = ARG2
date = ARG3

set multiplot layout 2,1

set xtics nomirror
#set y2tics

set xlabel sprintf( "Seconds (%s)", date )
set lmargin 12

set ylabel  "Temp.(\\si{\\celsius})"
set format y "%.2f"

plot logfile using 1:4 with linespoints pt 5 ps 0.5 axes x1y1 title "T"

set ylabel "Pressure(hPa)"
set format y "%.2f"

plot logfile using 1:($2/100) with linespoints pt 6 ps 0.5 axes x1y2 title "P"
