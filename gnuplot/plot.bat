@echo off

set  DATA2GP=bin\Debug\data2gp.exe
set  GNUPLOT=c:\pgm\gnuplot\bin\gnuplot.exe

echo open 192.168.2.1   >ftpscript.txt
echo root              >>ftpscript.txt
echo rootpassword      >>ftpscript.txt
echo cd  /             >>ftpscript.txt
echo lcd .             >>ftpscript.txt
echo get history.dat   >>ftpscript.txt
echo bye               >>ftpscript.txt

ftp  -s:ftpscript.txt
rem del     ftpscript.txt

%DATA2GP%  >history.txt
%GNUPLOT%   plot_script.gp

