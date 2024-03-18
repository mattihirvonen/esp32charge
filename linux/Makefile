all: logger


logger: logger.c  logger.h  Makefile
	gcc logger.c -o logger

debug:
	gcc -O0 -ggdb -fsanitize=address -fno-omit-frame-pointer logger.c -o logger

clean:
	rm logger

