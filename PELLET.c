#include "header.h"

///////////////////////
// Method prototypes //
///////////////////////
static void *pellet_func(void *ignored);

// Used for setting up and identifying shared memory
int shmid;
key_t shm_key = 1234;

// Points to start of shared memory
char *shm;

// For logging pellet information
FILE *fp2;
int eaten_counter;

// Attempt A: use one semaphore for when threads can run and keep track of when
// they've all finished to know when to lower the semaphore and go back to SWIM_MILL
// (this attempt was the closest to working properly)
sem_t thread_sem;
int thread_total = 0;
int thread_counter = 0;

// The Pellet process creates threads for individual pellets.
int main(){

	// Array for storing threads running pellets
	pthread_t pellet_threads[TIME];

	// Prepping file to write pellet info into
	fp2 = fopen("Pellet Log.txt", "w");
	fprintf(fp2, "Pellet tracker: \n");
	fclose(fp2);

	// Getting seed for random numbers
	srand(time(NULL));

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

  // Opening the necessary semaphores created by SWIM_MILL
	pellet_sem = sem_open("/pellet_sem", 0);
	mill_sem = sem_open("/mill_sem", 0);

	// With attempt A: Creating semaphore for controlling when threads run
	sem_init(&thread_sem, 0, 1);

    // Generating pellets
    for(int i = 0; i < TIME; i++){

			// Entering critical section for main PELLET (where threads are created)
			sem_wait(pellet_sem);

			// Randomly generates a pellet 50% of the time
			if(rand() % 2 == 0){

    		// Starting a pellet thread
				int code = pthread_create(&pellet_threads[i], NULL, pellet_func, NULL);

				// With attempt A: keeping track of how many total threads there are
				thread_total++;

				// Returns error and ends program if thread creation failed.
				if (code) {
					fprintf(stderr, "pthread_create failed with code %d\n", code);
					return 0;
				}
    	}
			//printf("pellet main\n");

			thread_counter = 0;
			sem_post(&thread_sem); // Starting pellet threads
			//printf("running threads\n");
			while(thread_counter < thread_total); // Checking if all threads are finished
			//printf("stopping threads\n");
			sem_wait(&thread_sem); // Stopping pellet threads

			sem_post(mill_sem); // Starting SWIM_MILL again
	}
	return(0);
}

// Pellet functionality. These pellets are  placed at the top of a random column in the river then
// flow down. At the second to last row they check if the fish is below them, and if it is they
// are eaten. Otherwise they are missed and their thread ends. Either occurance is logged.
// Pellets move differently based on their location in the grid, so they have multiple
// critical sections.
static void *pellet_func(void *arg){

	// Attempt C: Stores the number of this thread in the threads array. Need to pass
	// (void*) i as argument when thread is created
	//int* a = (int*) arg;

	// Storing thread ID for logging
	int tid = pthread_self();

	// Randomly chooses column
	int col = rand() % RIVER_WIDTH;

	// Putting pellet in its column at top of grid
	sem_wait(&thread_sem);
	char *pellet = shm + col;
	*pellet = 'o';
	//printf("pellet %d\n", tid);
	thread_counter++;
	sem_post(&thread_sem);
	sleep(1);

	// Moves down the grid till the second to last row
	for(int i = 0; i < RIVER_HEIGHT - 2; i++){
		sem_wait(&thread_sem);
		*pellet = '~';
		pellet += RIVER_WIDTH;
		*pellet = 'o';

		//printf("pellet %d\n", tid);
		thread_counter++;
		sem_post(&thread_sem);
		sleep(1);
	}

	sem_wait(&thread_sem);
	*pellet = '~';
	pellet += RIVER_WIDTH;

	// Checks if fish is below it in last row
	if(*pellet == 'F'){
		// If fish is below, pellet is eaten
		eaten_counter++;

		// Logging eaten pellet
		fp2 = fopen("Pellet Log.txt", "a");
		fprintf(fp2, "\tPellet eaten (column: %d, tid: %d)\n", col, tid);
		fprintf(fp2, "\t\tTotal pellets eaten: %d\n", eaten_counter);
		fclose(fp2);
		printf("Pellet eaten (column: %d)\n", col);
		thread_total--;
		sem_post(&thread_sem);
		return NULL;
	}
	else{
		// Pellet not eaten
		*pellet = 'o';
	}
	//printf("pellet %d\n", tid);
	thread_counter++;
	sem_post(&thread_sem);
	sleep(1);

	// Removes pellet when it reaches the end
	sem_wait(&thread_sem);
	*pellet = '~';
	//printf("pellet %d\n", tid);
	thread_counter++;
	sem_post(&thread_sem);

	// Logging missed pellet
	fp2 = fopen("Pellet Log.txt", "a");
	fprintf(fp2, "\tPellet missed (column: %d, tid: %d)\n", col, tid);
	fclose(fp2);
	printf("Pellet missed (column: %d)\n", col);
	thread_total--;
	return NULL;
}
