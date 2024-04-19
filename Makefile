all: cdraw
cdraw:
	gcc -O3 -march=native -o cdraw main.c -lcsfml-graphics -lcsfml-window -lcsfml-system -lm
.PHONY:
	clean all
clean:
	rm cdraw