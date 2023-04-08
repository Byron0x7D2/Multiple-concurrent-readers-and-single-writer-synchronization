#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/explorefile.h"
#include <unistd.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <limits.h>
#include "../include/sharedmemory.h"
#include "../include/sharedmemoryfun.h"
#include "../include/child.h"
#include <time.h>
#include <sys/time.h>
#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)

int main(int argc, char *argv[]){

	int N, requests, segments, lines, maxlinesize,linespersegment;
	char *filename, **semnames;

	/* Read command line arguments */
	if(argc != 5){perror("Not enough input arguments \n"); exit(EXIT_FAILURE);}
	filename = argv[1];
	segments = atoi(argv[2]);
	N = atoi(argv[3]);
	requests = atoi(argv[4]);

	/* Explore the fille given as argument */
	explorefile(filename, &lines, &maxlinesize);

	if(lines < 1000){perror("Not enough lines\n"); exit(EXIT_FAILURE);}
	linespersegment = lines/segments;

	/* Creating a semaphore array, one semaphore per segment */
	semnames = malloc(segments*sizeof(char*));
	sem_t **active = malloc(segments*sizeof(sem_t*));
	for(int i =0; i < segments; i++){
		semnames[i] = malloc(50*sizeof(char));
		sprintf(semnames[i], "active%d", i);
		sem_unlink(semnames[i]);
		active[i] = sem_open(semnames[i], O_CREAT | O_EXCL , SEM_PERMS, 1);
		if(active[i] == SEM_FAILED){perror("semaphore \n"); exit(EXIT_FAILURE);}

	}


	/* Unlink semaphores that may have been left during failed runs. 
	There shouldn't be a need for those commands, but better safe than sorry.
	I must admit I did have to kill the programm many times during writing and testing.
	Now it should be bug free and finish every time. */
	sem_unlink("requestsegments");
	sem_unlink("answersegments");
	sem_unlink("requestread");
	sem_unlink("answerread");

	/* Allocating and initialising shared memory and shared request queue */
	sharedmemory semlock;

	int shmid = shmget(IPC_PRIVATE, sizeof(struct memory), (S_IRUSR | S_IWUSR));
	if(shmid == -1){perror("shmget \n"); exit(EXIT_FAILURE);}

	semlock = (sharedmemory)shmat(shmid, NULL, 0);
	if(semlock == (void*)-1){perror("shmat \n"); exit(EXIT_FAILURE);}

	init(semlock, linespersegment, maxlinesize, segments);


	/*Semaphore Initialisation  */
	sem_t* requestsegments = sem_open("requestsegments", O_CREAT | O_EXCL , SEM_PERMS, 0);
	if(requestsegments == SEM_FAILED){perror("semaphore \n"); exit(EXIT_FAILURE);}
	sem_t* answersegments = sem_open("answersegments", O_CREAT | O_EXCL , SEM_PERMS, 0);
	if(answersegments == SEM_FAILED){perror("semaphore \n"); exit(EXIT_FAILURE);}
	sem_t* requestread = sem_open("requestread", O_CREAT | O_EXCL , SEM_PERMS, 0);
	if(requestread == SEM_FAILED){perror("semaphore \n"); exit(EXIT_FAILURE);}
	sem_t* answerread = sem_open("answerread", O_CREAT | O_EXCL , SEM_PERMS, 0);
	if(answerread == SEM_FAILED){perror("semaphore \n"); exit(EXIT_FAILURE);}


	/* Child processes initialisation, After a child function is completed, the program exits */
	pid_t *pid;
	pid = malloc(N*sizeof(pid_t));

	for(int i = 0; i <N; i++){
		pid[i] = fork();
		if(pid[i]<0){perror("fork \n"); exit(EXIT_FAILURE);}
		if(pid[i]==0){
			child(requests, segments, linespersegment, maxlinesize, semlock, 
		 requestsegments, answersegments, requestread, answerread,active, i);
		   printf("exit {%d}\n", i);
		   	sem_close(requestsegments);
			sem_close(answersegments);
			sem_close(requestread);
			sem_close(answerread);
			for(int i =0; i < segments; i++){
				sem_close(active[i]);
				free(semnames[i]);
			}
			free(active);
			free(pid);
			free(semnames);
		    exit(0);
		}
	}

	/* Output file creation */
	FILE *fpout;
	fpout = fopen("output/parent.txt", "w+");
	// clock_t start;
	struct timeval start, temp;

	/* Parent Portion in writers-readers problem */
	while(semlock->completedchildren < N){ 
		/* Accept request */
		if(sem_post(requestsegments) <0){perror("semaphore post\n"); exit(EXIT_FAILURE);}
		/* Wait for request */
		if(sem_wait(answersegments) <0){perror("semaphore wait\n"); exit(EXIT_FAILURE);}
		/* Load request */
		loadrequestedsegment(semlock->cursegment, semlock, filename, linespersegment, maxlinesize);
		gettimeofday(&start, NULL);
		/* Signal children that the segment is loaded */
		if(sem_post(requestread) <0){perror("semaphore post\n"); exit(EXIT_FAILURE);}
		/* Wait for children finish reading segment */
		if(sem_wait(answerread) <0){perror("semaphore wait\n"); exit(EXIT_FAILURE);}
		gettimeofday(&temp, NULL);
		fprintf(fpout, "For segment %d, time in memory is %f\n",semlock->cursegment, (temp.tv_sec - start.tv_sec)*1000.0 + (temp.tv_usec - start.tv_usec)/1000.0);
	}


	/* All children have completed, wait for all of them to exit */
	printf("end\n");
	int status;
	for(int i = 0; i <N; i++) wait(&status);

	/* Free memory, Close semaphores */
	fclose(fpout);
	free(pid);
	del(semlock, linespersegment);
	shmdt((void*)semlock);

	sem_close(requestsegments);
	sem_unlink("requestsegments");
	sem_close(answersegments);
	sem_unlink("answersegments");
	sem_close(requestread);
	sem_unlink("requestread");
	sem_close(answerread);
	sem_unlink("answerread");
	
	for(int i =0; i < segments; i++){
		sem_close(active[i]);
		sem_unlink(semnames[i]);
		free(semnames[i]);
	}
	free(semnames);
	free(active);

	return 0;
}
