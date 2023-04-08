CC = gcc 

INCLUDE = ./include
SRC = ./src
LIBS:= -lpthread
OUT = ./output

CFLAGS = -pthread -Wall -g3 $(DEFINES) -I$(INCLUDE)  

SRCS    := $(wildcard $(SRC)/*.c)
OBJS    := $(patsubst $(SRC)/%.c,$(SRC)/%.o,$(SRCS)) 

EXEC = assignment1

ARGS = star.txt 100 10 1000

$(EXEC): $(OBJS) 
	$(CC) $(OBJS) $(LIBS) -o $(EXEC) 

run: $(EXEC)
	./$(EXEC) $(ARGS) > out.txt

cleanall:
	@rm -f $(OBJS) $(EXEC) $(wildcard $(OUT)/*.txt) valgrind-out.txt out.txt

clean:
	@rm -f $(OBJS) $(EXEC) 
	
valgrind: $(EXEC)
	valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         --log-file=valgrind-out.txt \
         ./$(EXEC) $(ARGS)