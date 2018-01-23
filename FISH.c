#include "header.h"

///////////////////////
// Method prototypes //
///////////////////////
int findPellet();
void moveLeft();
void moveRight();
void stay();

// Pointers to start of shared mem and the fish's location respectively
char *shm, *fish;

// Keeps track of which column the fish is in
int col;

int main(){

	// Used for setting up and identifying shared memory.
	int shmid;
	key_t shm_key = 1234;

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Based on tutorial: https://users.cs.cf.ac.uk/Dave.Marshall/C/node27.html

	// Locating shared memory segment
	if((shmid = shmget(shm_key, RIVER_SIZE, 0666)) < 0){
		perror("shmget");
		exit(1);
	}

	// Attaching segment
	if((shm = shmat(shmid, NULL, 0)) == (char *) -1){
		perror("shmat");
		exit(1);
	}
	///////////////////////////////////////////////////////////////////////////////////////////////

    // Opening the necessary semaphores created by SWIM_MILL.
	fish_sem = sem_open("/fish_sem", 0);
	pellet_sem = sem_open("/pellet_sem", 0);

	// Setting start location for fish
	fish = shm + RIVER_HEIGHT * (RIVER_WIDTH - 1) + RIVER_WIDTH / 2;
	*fish = 'F';
	col = RIVER_WIDTH / 2;

	// Finds pellet and moves fish towards it. Runs until SWIM_MILL kills FISH
	while(1){
		// Get direction of closest reachable pellet
		int dir = findPellet();

		// Entering FISH's critical section
		sem_wait(fish_sem);
		
		// Moves fish in direction of pellet
		if(dir < 0) {
			moveRight();
		} else if(dir > 0) {
			moveLeft();
		} else {
			stay();
		}
		
		// Raising pellet_sem so PELLET can run
		sem_post(pellet_sem); 
	}

	return(0);
}

// Moves the fish right. If it can't move further right, does nothing.
void moveRight(){
	if (col != RIVER_WIDTH - 1){
		*fish = '~';
		fish++;
		*fish = 'F';
		col++;
	}
}

// Moves the fish left. If it can't move further left, does nothing.
void moveLeft(){
	if (col != 0){
		*fish = '~';
		fish--;
		*fish = 'F';
		col--;
	}
}

// Fish doesn't move but rewrites its character. Helps prevent fish character from being overwritten
// due to synchronization issues.
void stay(){
	*fish = 'F';
}

// Scans through the array to find the nearest reachable pellet. Scans in a 'v' shape starting
// above the fish and spreading outward and upwards.
int findPellet(){

	// How wide of a section to search. Search is centered above the fish so don't want to go
	// too wide or the fish won't be able to reach pellets found.
	int width = 1;

	// Looping through rows starting with row just above fish and moving up
	for(int i = 0; i < (RIVER_HEIGHT - 1); i++){

		// Pointers start in same column as fish
		char *left = fish - RIVER_WIDTH * i;
		char *right = fish - RIVER_WIDTH * i;

		// Checking if closest pellet is in same column as fish; if so, don't move
		if(*left == 'o'){
			return(0);
		}

		// Pointers move outward, *left to the left and *right to the right, checking each location
		for(int j = 0; j < width; j++){

			// Checking it doesn't go past the left side
			if((left - shm) % RIVER_WIDTH != 0){
				left--;
				if(*left == 'o'){
					return(1);
				}
			}

			// Checking it doesn't go past the right side
			if((right - shm) % RIVER_WIDTH != 10){
				right++;
				if(*right == 'o'){
					return(-1);
				}
			}

		}
		width++;
		// Making sure width doesn't get larger than the grid itself
		if(width > RIVER_WIDTH/2)
			width = RIVER_WIDTH/2;
	}
	// If no pellet found, fish moves toward center
	return(col - 5);
}
