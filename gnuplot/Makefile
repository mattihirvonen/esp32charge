
CC=c:\pgm\CodeBlocks\MinGW\bin\x86_64-w64-mingw32-gcc.exe

APP=data2gp.exe

all: $(APP)

# CodeBlocks:
Debug:        $(APP)
Release:      $(APP)
cleanDebug:   clean
cleanTelease: clean

$(APP):    data2gp.c  Makefile
	$(CC)  data2gp.c  -o $(APP)

clean:
	del /S $(APP)
