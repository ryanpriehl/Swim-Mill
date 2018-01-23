#ifndef header_h
#define header_h
#endif

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>

#define RIVER_WIDTH 11
#define RIVER_HEIGHT 11
#define TIME 30

//static const int TIME = 30;
static const int RIVER_SIZE = 121;

sem_t *mill_sem;
sem_t *fish_sem;
sem_t *pellet_sem;

