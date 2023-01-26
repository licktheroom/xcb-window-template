CC = clang
CFLAGS = -O2 -march=native -pipe -fomit-frame-pointer -Wall -Wextra -Wshadow \
		-Wdouble-promotion -fno-common -std=c11
CLIBS = -lxcb

files = main.o

all: ${files}
	mkdir -p build/
	${CC} ${CFLAGS} ${CLIBS} ${files} -o build/xcb-window
	rm -f *.o

main.o:
	${CC} ${CFLAGS} -c -o main.o src/main.c