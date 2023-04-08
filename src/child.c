#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/explorefile.h"
#include <unistd.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <time.h>
#include <fcntl.h>
#include <limits.h>
#include "../include/sharedmemory.h"
#include "../include/sharedmemoryfun.h"
#include <sys/time.h>
#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)

// segments from 0 to segments -1
// lines per segment from 0 to lines per segment -1

void child(int requests,int segments,int linespersegment,int maxlinesize,sharedmemory semlock,
sem_t* requestsegments, sem_t* answersegments, sem_t* requestread, sem_t* answerread, sem_t** active, int i){

	srand(time(NULL) -getpid()); 

	/* Output file creation */
	char* name = malloc(30*sizeof(char));
	sprintf(name, "output/%d.txt", i);
	
	FILE *fp;
	fp = fopen(name, "w+");

	/* Child process main part */
	// clock_t start;
	struct timeval start, temp;
	double reqtime, anstime;
	int segrequest, linerequest;
	char *line;

	for(int reqsofar = 0; reqsofar < requests; ){
		gettimeofday(&start, NULL);

		/* Creating the request */
		if(reqsofar == 0){ 
			segrequest = (rand() % (segments));
		}else{
			if(rand()/(double)RAND_MAX <= 0.3)
				segrequest = (rand() % (segments));
		}
		linerequest = (rand()% (linespersegment));

		/* Wait for access to users of the segment */
		if(sem_wait(active[segrequest]) <0){perror("semaphore wait\n"); exit(EXIT_FAILURE);}
		semlock->activeusers[segrequest]++; //increase active users on segment
		if(semlock->activeusers[segrequest] == 1){  //fist user here, make sure the correct segment is loaded
			if(sem_wait(requestsegments) <0){perror("semaphore wait\n"); exit(EXIT_FAILURE);}
			semlock->cursegment = segrequest;
			if(sem_post(answersegments) <0){perror("semaphore post\n"); exit(EXIT_FAILURE);}
			if(sem_wait(requestread) <0){perror("semaphore wait\n"); exit(EXIT_FAILURE);}
		}
		gettimeofday(&temp, NULL);
		reqtime = (temp.tv_sec - start.tv_sec)*1000.0 + (temp.tv_usec - start.tv_usec)/1000.0;
		if(sem_post(active[segrequest]) <0){perror("semaphore post\n"); exit(EXIT_FAILURE);}

		/* inside the critical section loaded with the segment the process wants */

		line = malloc((maxlinesize+1)*sizeof(line));
		gettimeofday(&temp, NULL);
		anstime = (temp.tv_sec - start.tv_sec)*1000.0 + (temp.tv_usec - start.tv_usec)/1000.0;
		fprintf(fp, "For request <%d,%d>, time to request was: %f and time to answer was: %f\n", segrequest, linerequest, reqtime, anstime);
		strcpy(line, &semlock->segment[linerequest*(maxlinesize+1)]);
		printf("%s\n", line);
			
		// Wasting time to simulate processing
		clock_t  startdelay = clock();
		int waste = 0;
		while((clock() -startdelay)/(double)CLOCKS_PER_SEC < 0.002){waste++;};
		reqsofar++;  //Increase completed requests
		free(line);
		
		/* Exiting critical section */
		if(sem_wait(active[segrequest]) <0){perror("semaphore wait\n"); exit(EXIT_FAILURE);}
		semlock->activeusers[segrequest]--;
		if(!(reqsofar < requests)){semlock->completedchildren++;} //If completed all requests, let the parent know you finished.
		/* Last user exiting, tell parent to move on */
		if(semlock->activeusers[segrequest] == 0){ 
			if(sem_post(answerread) <0){perror("semaphore post\n"); exit(EXIT_FAILURE);}
		}
		if(sem_post(active[segrequest]) <0){perror("semaphore post\n"); exit(EXIT_FAILURE);}

	}
	
	free(name);
	fclose(fp);
}