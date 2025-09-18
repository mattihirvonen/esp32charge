
Workflow
--------
- Export ESP32 measurement data with telnet's export" command to file system (file: /history.dat)
- Download this measurement history data file into PC (use FTP)
- Run "data2gp" data conversion application from command line like

      data2gp > history.txt

- Run gnuplot from command line with script (script uses "history.txt" data file)

      gnuplot  plot_script.gp

