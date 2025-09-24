@echo off

set  DATA2GP=data2gp.exe
set  GNUPLOT=c:\pgm\gnuplot\bin\gnuplot.exe

rem  echo open 192.168.2.1   >ftpscript.txt
rem  echo root              >>ftpscript.txt
rem  echo rootpassword      >>ftpscript.txt
rem  echo cd  /             >>ftpscript.txt
rem  echo lcd .             >>ftpscript.txt
rem  echo get history.dat   >>ftpscript.txt
rem  echo bye               >>ftpscript.txt

ftp  -s:ftpscript.txt

rem start 20:15
rem %DATA2GP%  -f history.dat.alku >history.txt

%DATA2GP%  -f history.dat      >history.txt
%GNUPLOT%  -p plot_script.gp    history.txt
