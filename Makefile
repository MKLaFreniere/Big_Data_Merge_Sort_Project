CFLAGS= -std=c++11
CC=g++

all: MergeSort Read

# To make an executable
MergeSort: MergeSort.cpp
	$(CC) $(CFLAGS) -o MergeSort MergeSort.cpp

Read: Read.cpp
	$(CC) $(CFLAGS) -o Read Read.cpp

# clean out the dross
clean:
	-rm MergeSort.cpp 

