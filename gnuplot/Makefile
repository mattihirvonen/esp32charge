
CC=c:\pgm\CodeBlocks\MinGW\bin\x86_64-w64-mingw32-gcc.exe

APP=data2gp.exe

OPT= -Wall -Wextra

all: $(APP)

# CodeBlocks:
Debug:        $(APP)
Release:      $(APP)
cleanDebug:   clean
cleanTelease: clean

$(APP):  Makefile  data2gp.c
	$(CC)  $(OPT)  data2gp.c  -o $(APP)

clean:
	del /S $(APP)
