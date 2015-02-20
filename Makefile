CC = clang
SRC = apem.c
NAME = apem
CFLAGS = -Wall -Wpedantic -Wextra -Wno-shift-op-parentheses -std=c99 

all:	
	${CC} -o ${NAME} ${CFLAGS} ${SRC} 

test: all
	./${NAME} 

clean:
	rm -f ${NAME}
