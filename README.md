# OPERATING SYSTEMS ASSIGNMENT 1
# Vyron-Georgios Anemogiannis 

This is the first assignment of the operating systems cource, writen in C. 


## How To Run
In order to compile, link and run, you can use the included Makefike with the following commands.

- Compile all necessary files `make`
- Run the programm with the specified parameters (in the corresponding makefile section) `make run`
- Clean all the object files and the executable `make clean`
- Clean all the object files, the executable and the output files `make cleanall`
- To verify that all the memory has been freed with valgrind `make valgrind`

### General:
So the main portion of the program consists of 2 simple loops. The parent simply asks for requests and when requests are given, he loads the request in the shared memory (for the requests, it is first get the semaphore, first served so it satisfies the FIFO requirement) and lets the chidren know he is finished loading. After he waits for all the readers to exit the segment. This goes on untill all children finish. The child for all the requests it must do, it first creates a request then it waits for access to the active user counter of the requested segment. Once it enters, if it is the first one there it waits for the parent to accept the request and then load the memory. Once it gets it, it goes in the shared memory segment. All the other requests for the segment, just need to wait for the active user semaphore and if they are not the first ones, they get to the shared memory immediately. Once all the requests for the segment are done, the last one informs the parent to move on with the reuqests.
Times are mesured with the `gettimeofday()` syscall since it is the best way to understand real time passed in such small scale.
The program recieves as command line arguments the name of the text file, how many segments are requested, how many children to create and how many requests each child should make. Everything is included in the makefile.



## Important file explenation

### sharedmemory.h
For this assignment, we use 1 memory segment.
We have four fields. 
- `activeusers[Segments]` denotes the child processes currently reading each segment, this is an integer array. Each child entering a segment, increases the corresponding active user counter.
- `cursegment` is the segment currently inside the memory, it also acts like a request field. If there is none `cursegment = -1`. 
- `segment` is an array where the text will go. Since we can't possibly know the size before hand, and we don't want to make assumptions about the memory size, we will allocate memory during the initialisation phase. 
- `completedchildren` denotes how many child processes have finished with all their requests.


### sharedmemoryfun.c
In this file there are functions for the shared memory struct.
- `init`: Now that we know how many lines we need to save as well as the maximum size of a line and the segments we need, we can allocate memory accordingly. We cannot use malloc of course so we use the same function we used for all shared memory. In addition we initialize the active users and the completed children to 0.
-  `del`: Simply frees the shared memory we allocated in `init`.

### explorefile.c
2 simple functions that have to do with file management.
- `explorefile`: Takes a file and returns the number of the lines inside, as well as the amount of characters of the biggest line of the file.
- `loadrequestedsegment`: Takes a file and loads the lines of the requested segment in shared memory. We start counting the segments and the lines from 0 up to MAX-1.

### parent.c
Main program file. There are many sections in this file containing only the main function.
- Imputs: We start by getting the inputs from the command line arguments. Those are the name of the file, the number of segments, the number of **child** processes and the amount of requests each process will have. After we use the `explorefile` function to get the number of lines as well as the biggest line. If the file doesn't have at least 1000 lines, we end the program. In addition, each segment will have amount of lines **div** segment count lines.
- Shared memory: We allocate shared memory space for the struct of `sharedmemory.h` and we initialise it with his init function. We pass as arguments all the requested information so we can allocate more memory accordingly.
- Semaphore initialisation. We initialise all the semaphores we are going to use later. Those are:
	- `requestsegments`: Tells child processes that the parent is ready to receive their requests in the shared queue.
	- `answersegments`: Signals parent that requests have been given and to start loading them.
	- `requestread`: Tells child processes that a segment is loaded in the shared memory.
	- `answeread`: Child processes tell father that they are done reading shared memory.
	- `active[Segments]`: Signal for child processes, not to change the `activeusers[Segments]` counter, while another one is using it. Each segment has its own flag and counter.
- Children creation: the parent initialises N child processes. Each child calls the `child` function and once it is done, it exits using `exit`.
- Readers - Writers Problem. Parent Side:
The parent repeats, untill there are no more not completed children. 
	1. the parent signals the children that he can accept new requests for segments. 
	2. he waits for a response that requests have been given.
	3. he loads the requested segment in memory and signals the child that the segment is loaded
	4. he waits for a response, that there are no more readers in the segment. (Also updating output file with time of segment in memory)
	5. starting again untill all children have completed.
- Cleaning: Once he is done with the requests, the parent waits for all children to exit. Afterwards, the parent frees the memory he malloced, calls the necessary functions to free the shared memory and closes and unlinks all the semaphores.

### child.c
This file contains the child program function. Once it is finished, control goes back to main, where the child process exits.
- Srand initialisation. Every child initialises the rand function using the current time minus the process id. This ensures each child will get diffrent random values.
- Output file creation. We create the file where the printfs will go. We name each file with the number of the process starting from 0 up to N-1. Files are saved in the output folder.
- Readers - Writers Problem. Child Side:
The child repeats untill it completes the required amount of requests.
	1. Every time we start the function we start the clock so we can know the answer and request times. In addition we choose a segment and line. This is done with the `rand` function. For the segment, we also use `rand` to give a possibility of only 0.3 of the segment changing, otherwise we use the privius one.
	2. The child waits for access for the user counter of the segment it wants.
	3. Once request is given, it increases the user counter. If it is the first user, it waits for the parent to accept segment requests. He only accepts one at a time with the built in FIFO ability of the semaphores.
	If it is not the first one, it simple increases the users and goes on the shared memory segment.
	4. In the requested segment the child reads the line, it updates the output file with the times needed and it waits 2 ms to simulate processing. Since the request was completed, the `reqsofar` variable is increased.
	5. The child waits to be allowed to change the `activeusers[Segment]` shared variable. Then it decreases the users since it exites the shared segment. If it was the last user, it also signals the parent to go ahead and change the segment. In addition if it was the last request of the child, it increases the `completedchildren` variable so the parent knows when to finish. After that the child allows other children to change the user count.
	6. This goes on untill the required amount of requests is completed.
	7. We free the memory, close the file and send control back to main, where the child process immediately exits.

