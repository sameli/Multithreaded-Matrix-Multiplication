# Multithreaded-Matrix-Multiplication
A program to multiply matrices in different threads and using shared memory. When the program runs, 
3 different processes will be forked.
First one is for reading and loading files from disk. Second one is for multiplying matrices using multiple threads.
And the third process is used for printing the result. A shared memory is used for communication between processes.

This program has been written to utilize and demonstrate shared memory across multiple processes and threads.

Note: This program has been tested with GCC 4.8.2 and Ubuntu 14.04
```
Compile: gcc main.c -o main -lrt -lpthread
Run: ./main
```
