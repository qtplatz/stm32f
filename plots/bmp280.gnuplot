set terminal epslatex size 6in,6in color colortext standalone header \
"\\usepackage{graphicx}\n\\usepackage{amsmath}\n\\usepackage[version=3]{mhchem}\n\\usepackage{siunitx}"

set output ARG1
logfile = ARG2
date = ARG3

set xtics nomirror
set y2tics
set y2label "Pressure(Pa)"
set ylabel  "Temp.(\\si{\\celsius})"
set xlabel sprintf( "Seconds (%s)", date )

plot logfile using 0:3 with linespoints axes x1y1 title "T" \
     , logfile using 0:1 with linespoints axes x1y2 title "P"
