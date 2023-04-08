#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/sharedmemory.h"

/* Takes a filename and returns the line count as well as the size of the largest line */
void explorefile(char* filename, int *lines, int *maxlinesize){
	FILE *fp;
	int l = 0, tmax = 0, max = 0;
	char ch;
	fp = fopen(filename, "r");
	if(fp == NULL){perror("Could not read file"); exit(EXIT_FAILURE);}
	for (ch = getc(fp); ch != EOF; ch= getc(fp)){
		tmax++;
		if (ch == '\n'){
			l++;
			if(tmax > max){
				max = tmax;
			}
			tmax = 0;
		}
	}
	fclose(fp);
	*lines = l;
	*maxlinesize = max;
}

/* Loads in shared memory the part of text that was specified */
void loadrequestedsegment(int req,sharedmemory semlock,char* filename,int linespersegment,int maxlinesize){
	FILE *fp;
	fp = fopen(filename, "r");
	char* temp = malloc((maxlinesize+1)*sizeof(char));
	int start = req*linespersegment; 
	int end = start + linespersegment - 1;
	int i = 0, j=0;
	while(fgets(temp, maxlinesize, fp)){
		if(i >= start && i <=end){  //If line within specified limits, copy it to the shared memory
			strcpy(&semlock->segment[j*(maxlinesize+1)], temp);
			j++;
		}else if(i > end) break;
		i++;
	}
	free(temp);
	fclose(fp);
}


