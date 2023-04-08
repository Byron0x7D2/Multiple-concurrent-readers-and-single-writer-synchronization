#include "../include/sharedmemory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <limits.h>
#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)

/* Initialize shared memory segment */
void init(sharedmemory mem, int linespersegment, int maxline, int segments){
	mem->cursegment = -1;
	// Allocate shared memory according to the sizes given as inputs to the function
	int shmid = shmget(IPC_PRIVATE, sizeof(char)*linespersegment*(maxline+1), (S_IRUSR | S_IWUSR));
	mem->segment = (char*)shmat(shmid, NULL, 0);
	mem->completedchildren = 0;
	shmid = shmget(IPC_PRIVATE, (segments)*sizeof(int), (S_IRUSR | S_IWUSR));
	mem->activeusers = (int*)shmat(shmid, NULL, 0);
	for(int i =0; i < segments; i++){
		mem->activeusers[i] = 0;
	}
}

/* Frees the shared memory, allocated during the initialisation phase */
void del(sharedmemory mem, int linespersegment){
	shmdt((void*)mem->segment);
	shmdt((void*)mem->activeusers);
}

