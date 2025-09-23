
Workflow
--------
- Export ESP32 measurement data with telnet's export" command to file system (file: /history.dat)
- Download this measurement history data file into PC (use FTP)
- Run "data2gp" data conversion application from command line like

      data2gp > history.txt

- Run gnuplot from command line with script (script uses "history.txt" data file)

      gnuplot  plot_script.gp


"data2gp" Command Line Options
------------------------------
 -f [filename]   input data file name (default filename is "history.dat")
 -s [seconds]    "dataset_t" and output's' rows wall clock time step in "seconds"
                 (default is 10, "seconds" can be also float eg. 0.1)
 -o [hours]      start gnuplot data rows after "hours" of valid data in "filename"
 -h [hours]      end   gnuplot data rows after "hours" of valid data in "filename"
