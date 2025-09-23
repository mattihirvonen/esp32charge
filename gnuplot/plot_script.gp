set title   "Simple XY Plot - Hupiakku"
set xlabel  "Time [h]"
set ylabel  "Current [A] and Charge [Ah]"
set y2label "Voltage [V]"

set grid
set ytics  5 nomirror
set y2tics 0.2

set yrange  [-20:20]
set y2range [11.6:14.8]

# https://docs.w3cub.com/gnuplot/linetypes_colors_styles.html
set style line 3 lt rgb "grey" lw 1

#plot "history.txt" using 1:2 with linespoints title "XY Data"

plot  "history.txt" using 1:3 with lines      axes x1y1 title "Current [A]", \
      "history.txt" using 1:4 with lines      axes x1y2 title "Voltage [V]", \
      "history.txt" using 1:2 with lines ls 3 axes x1y1 title "Charge [Ah]"

#pause -1 "Press any key to exit..."
pause mouse
