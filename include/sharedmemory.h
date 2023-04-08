#pragma once

struct memory{
	int *activeusers;   // users reading the shared memory segment
	int cursegment;    //segment currently loaded - requested
	char *segment;    //loaded text
	int completedchildren;   //child processes that have completed
};

typedef struct memory* sharedmemory;

