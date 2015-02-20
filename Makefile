CC = clang
SRC = em.c
NAME = em
CFLAGS = -Wall -Wpedantic -Wextra -std=c99 -Wno-shift-op-parentheses

all:	
	${CC} -o ${NAME} ${CFLAGS} ${SRC} 

test: all
	./${NAME} 

clean:
	rm -f ${NAME}
