all: SWIM_MILL FISH PELLET

SWIM_MILL: SWIM_MILL.c
	gcc -pthread -o SWIM_MILL SWIM_MILL.c -I.

FISH: FISH.c
	gcc -pthread -o FISH FISH.c -I.

PELLET: PELLET.c
	gcc -pthread -o PELLET PELLET.c -I.
