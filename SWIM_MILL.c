#include "header.h"

///////////////////////
// Method prototypes //
///////////////////////
void printMem(char *shm);
void interruptAndExit();
void killAndExit();

// Used for setting up and identifying shared memory.
int shmid;
key_t shm_key = 1234;

// Used for accessing shared memory
char *shm, *river;

// Store process IDs for the three processes
pid_t fish, pellet, pid;

// For logging process information
FILE *fp;

// Used when deallocating shared mem, not totally sure what it is
// from: https://www.ibm.com/support/knowledgecenter/en/SSB23S_1.1.0.14/gtpc2/cpp_shmctl.html
struct shmid_ds buf;

// Coordinator process that creates the shared memory, fish , and pellet processes. Prints the grid
// every second for duration given in header.h and kills/closes everything when done.
int main(){

	pid = getpid();

	// Opens process log and writes initial information
	fp = fopen("Process Log.txt", "w");
	fprintf(fp, "-----START-----\n");
	fprintf(fp, "Creating processes:\n");
	fprintf(fp, "\tSwim mill process started (pid: %d)\n", pid);

	// Setting up method to run on interrupt (ctrl + c)
	signal(SIGINT, interruptAndExit);

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Based on tutorial: https://users.cs.cf.ac.uk/Dave.Marshall/C/node27.html

	// Creating shared memory
	if((shmid = shmget(shm_key, RIVER_SIZE, IPC_CREAT | 0666)) < 0){
		perror("shmget");
		exit(1);
	}

	// Attaching shared memory
	if((shm = shmat(shmid, NULL, 0)) == (char *) -1){
		perror("shmat");
		exit(1);
	}
	///////////////////////////////////////////////////////////////////////////////////////////////
	fprintf(fp, "\tShared memory created (shmid: %d)\n", shmid);

	// Creating named semaphores for controlling processes. Multiple semaphores
	// are used as each one controls one process. When one process finishes it
	// raises the semaphore of the next process so that process can run. This
	// part actually works.
	mill_sem = sem_open("/mill_sem", O_CREAT, S_IRWXU | S_IRWXO, 1);
	fish_sem = sem_open("/fish_sem", O_CREAT, S_IRWXU | S_IRWXO, 0);
	pellet_sem = sem_open("/pellet_sem", O_CREAT, S_IRWXU | S_IRWXO, 0);

	// Filling shared memory with '~' (to represent water)
	river = shm;
	for(int i = 0; i < RIVER_HEIGHT; i++){
		for(int j = 0; j < RIVER_WIDTH; j++){
			*river = '~';
			river++;
		}
	}
	river = shm;

	// Creating fish process
	fish = fork();
	if(fish == 0){
		const char *arg;
		execl("./FISH", arg, (char *) NULL);
	}
	fprintf(fp, "\tFish process started (pid: %d)\n", fish);

	// Creating pellet process
	pellet = fork();
	if(pellet == 0){
		const char *arg;
		execl("./PELLET", arg, (char *) NULL);
	}
	fprintf(fp, "\tPellet process started (pid: %d)\n", pellet);

	// Running the program for the desired duration, prints the grid every second with updated
	// locations of fish and pellets.
	for(int t = TIME; t > 0; t--){
		// Entering SWIM_MILL critical section
		sem_wait(mill_sem);
		printf("\nTime remaining: %d\n", t);
		printMem(shm);
		//printf("swim mill\n");
		sem_post(fish_sem); // Opens fish_sem so FISH can run
		sleep(1);
	}

    // Closing everything
    printf("Simulation time over.\n");
    killAndExit(0);
    return(0);
}

// Loops through and prints the contents of the shared memory
void printMem(char *shm){
	for(int i = 0; i < RIVER_HEIGHT; i++){
		for(int j = 0; j < RIVER_WIDTH; j++){
			char c = *shm;
			shm++;
			putchar(c);
			putchar(' ');
		}
		putchar('\n');
	}
	putchar('\n');
}

// Handles closing everything on interrupt
void interruptAndExit(){
	fprintf(stderr, "\nInterrupt received.\n");
	killAndExit();
}

// Handles closing everything at the end the of program OR on interrupt
// Also writes information to log as processes are closed
void killAndExit(){
	fprintf(fp, "\nKilling children and exiting swim mill:\n");

	// Killing child processes
	kill(fish, SIGTERM);
	fprintf(fp, "\tFish process killed (pid: %d)\n", fish);
	kill(pellet, SIGTERM);
	fprintf(fp, "\tPellet process killed (pid: %d)\n", pellet);

	// Detaching shared mem and removing it
	fprintf(fp, "\tDeallocating shared memory (shmid: %d)\n", shmid);
	shmdt(shm);
	shmctl(shmid, IPC_RMID, &buf);

	// Removing semaphores
	fprintf(fp, "\tRemoving semaphore.\n");
	sem_close(mill_sem);
	sem_unlink("/mill_sem");
	sem_close(fish_sem);
	sem_unlink("/fish_sem");
	sem_close(pellet_sem);
	sem_unlink("/pellet_sem");

	// Exiting
	fprintf(fp, "\tExiting swim mill (pid: %d)\n", pid);
	fprintf(fp, "-----END-----");
	fclose(fp);
	exit(0);
}
