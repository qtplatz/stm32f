set terminal epslatex size 12in,9in color colortext standalone header \
"\\usepackage{graphicx}\n\\usepackage{amsmath}\n\\usepackage[version=3]{mhchem}\n\\usepackage{siunitx}"

set output ARG1
logfile = ARG2
date = ARG3

set multiplot layout 2,1


set xtics nomirror rotate
#set y2tics
stats logfile using 1 nooutput

set xdata time 
#set format x "%H:%M"

set xlabel sprintf( "Hours (%s)", date )
#set xlabel sprintf( "Hours (%s)", strftime("%T",time(0)) )
set lmargin 12

set ylabel  "Temp.(\\si{\\celsius})"
set format y "%.2f"

#plot logfile using (($1-STATS_max)+time(0)):4 with linespoints pointinterval 600 pt 5 ps 0.5 axes x1y1 title "T"

plot logfile using (($1-STATS_max)+time(0)+3600*9):4 with linespoints pointinterval 600 pt 5 ps 0.5 axes x1y1 title "T"

set ylabel "Pressure(hPa)"
set format y "%.2f"

plot logfile using (($1-STATS_max)+time(0)+3600*9):($2/100) with linespoints pointinterval 600 pt 6 ps 0.5 axes x1y2 title "P"
