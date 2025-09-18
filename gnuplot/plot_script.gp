set title   "Simple XY Plot - Hupiakku"
set xlabel  "Time [h]"
set ylabel  "Current [A] and Charge [Ah]"
set y2label "Voltage [V]"

set grid
set ytics  5 nomirror
set y2tics 0.2

set yrange [-15:15]
set y2range [11.8:13.0]

#plot "data.txt" using 1:2 with linespoints title "XY Data"

plot "history.txt" using 1:3 with lines axes x1y1 title "Current [A]", \
     "history.txt" using 1:4 with lines axes x1y2 title "Voltage [V]", \
     "history.txt" using 1:2 with lines axes x1y1 title "Charge [Ah]"

#pause -1 "Press any key to exit..."
pause mouse 
