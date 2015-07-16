/*
 ============================================================================
 Name        : main.c
 Author      : S. Ameli
 Date        : 01-MAR-2014
 Description : This program reads two matrices A and B from files: "matrixA.txt" and "matrixB.txt"
               and calculates their product using different proceses and threads and a shared memory.
               In each of the matrix files each number should be seperated by a space and each row should be
               seperated by a new line. Example files are provided in the source folder.
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

#define MAX_LEN 10 // this is maximum lenth of row and columns. This must be increased for big matrices.

typedef struct{
	int matrix[MAX_LEN][MAX_LEN];
	int numberOfRows;
	int numberOfColumns;
} MatrixType;

// this is a type that holds 3 matrices to be used and shared in the memory
typedef struct{
	MatrixType matrixA, matrixB, mResult;
} DataTableType;

DataTableType* getDataTableFromSharedMem();


/*
 * this is the main function. the program starts from here.
 * this method creates 3 proceses:
 * 1- a process to: first read data from files: "matrixA.txt" and "matrixB.txt", and second create a shared
 * 		memory and store the matrices there.
 * 2- load the data from shared memory and calculate product of matrices using threads
 * 3- printing the result
 */
int main(void) {

	pid_t pid;

	pid = fork(); // this process is for loading and sharing memory

	if (pid == 0) { /* child process */
		printf ("child process 1: reading data and sharing memory. id %d\n", (int) getpid ());
		loadDataAndCreateSharedMem();
		return 0;
	}else if (pid > 0) { /* parent process */
		wait(NULL);
		//printf ("returning to parent process: id %d\n", (int) getpid ());

		pid_t pid2 = fork(); // this process is for reading shared memory and calculation multiplication using threads

		if(pid2 == 0){ /* child process */
			printf ("child process 2: read shared mem and calculate multiplication. id %d\n", (int) getpid ());

			DataTableType *data = getDataTableFromSharedMem();
			calcMatrixThreaded(data);

		}else if (pid2 > 0){ /* parent process */
			wait(NULL);
			//printf ("returning to parent process: id %d\n", (int) getpid ());

			pid_t pid3 = fork(); // this process is for reading shared memory and printing the result

			if(pid3 == 0){ /* child process */
				printf ("child process 3: read shared mem and print. id %d\n", (int) getpid ());

				DataTableType *data = getDataTableFromSharedMem();
				printMatrix(data);

			}else if (pid3 > 0){ /* parent process */
				wait(NULL);
				//printf ("returning to parent process: id %d\n", (int) getpid ());
			}

		}
	}

	return 0;
}

/*
 * this method:
 * 1- loads data from files
 * 2- prints the loaded matrices
 * 3- calls a method to create shared memory
 */
void loadDataAndCreateSharedMem(){

	DataTableType *dataTable = malloc(sizeof(DataTableType));

	//loadData(dataTable); // this is just for testing reading data from this method

	readDataFromFile("matrixA.txt", &dataTable->matrixA); // we pass a reference of matrix A to be modified written
	readDataFromFile("matrixB.txt", &dataTable->matrixB); // same as above
	printMatrixAB(dataTable);
	createSharedMem(dataTable);
}

/*
 * this method accepts a file name of a matrix and a matrix type to write data to.
 * the file should contain one matrix. each number should be seperated by a space and each row should be
 * seperated by a new line. for example a matrix of row and columns of 2x2 is:
 * 3 4
 * 5 8
 */
void readDataFromFile(char* fileName, MatrixType* matrix){

	int LINESZ = MAX_LEN*MAX_LEN;
	char buff[LINESZ];
	FILE *fin = fopen (fileName, "r");

	if (fin == NULL) {
		printf ("File not found.\n");
		return;
	}

	int list[MAX_LEN*MAX_LEN];

	if (fin != NULL) {

		int countAll = 0, countRows = 0, countColumns = 0;

		while (fgets (buff, LINESZ, fin)) {

			countRows++;
			//printf("%s \n", buff);

			char * pch;
			//printf ("Splitting string \"%s\" into tokens:\n",buff);
			pch = strtok (buff," ");

			countColumns = 0;
			while (pch != NULL){
				countColumns++;
				//printf ("%s\n",pch);
				int cellValue = atoi(pch);
				list[countAll] = cellValue;
				countAll++;

				pch = strtok (NULL, " ");
			}

		}

		//printf ("rows: %d, columns: %d\n", countRows, countColumns);

		matrix->numberOfColumns = countColumns;
		matrix->numberOfRows = countRows;


		//matrix->matrix = malloc(countRows*sizeof(int*));
		int i,j;
		int n=0;
		for(i=0; i< countRows; i++){
			//matrix->matrix[i] = malloc(countColumns*sizeof(int));

			for(j=0; j< countColumns; j++){
				//printf ("%d ", list[n]);
				matrix->matrix[i][j] = list[n];
				//printf ("matrix[%d][%d]: %d ",i, j, matrix->matrix[i][j]);
				n++;
			}
			//printf ("\n");
		}
		//printf ("countAll: %d\n", countAll);

		fclose (fin);
	}

}

/*
 * this method is just for testing loading data
 */
void loadData(DataTableType *dataTable){

	int first[3][3] = {3,2, 3,3};
	int second[3][3] = {4,4, 4,5, 4,6};

	int i,j;
	for(i=0; i<3; i++){
		for(j=0; j<3; j++){
			dataTable->matrixA.matrix[i][j] = first[i][j];
			dataTable->matrixB.matrix[i][j] = second[i][j];
		}
	}

	dataTable->matrixA.numberOfColumns = 2;
	dataTable->matrixA.numberOfRows = 2;
	dataTable->matrixB.numberOfColumns = 2;
	dataTable->matrixB.numberOfRows = 3;

}

/*
 * this method creates a shared memory and inserts the given data
 */
void createSharedMem(DataTableType *dataTable){

	const char *name = "calcMatrixSharedMemName";

	const int SIZE = sizeof(DataTableType);

	int shm_fd;
	DataTableType *ptr = NULL;

	/* create the shared memory segment */
	shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);

	/* configure the size of the shared memory segment */
	ftruncate(shm_fd,SIZE);

	/* now map the shared memory segment in the address space of the process */
	ptr = mmap(0,SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (ptr == MAP_FAILED) {
		printf("Map failed\n");
		return -1;
	}

	// writting dataTable to shared memory:
	*ptr = *dataTable;
}

/*
 * this method connects to the shared memory and returns a pointer to that data
 */
DataTableType* getDataTableFromSharedMem(){

	const char *name = "calcMatrixSharedMemName";
	const int SIZE = sizeof(DataTableType);

	int shm_fd;
	DataTableType *ptr = NULL;
	int i;

	/* open the shared memory segment */
	shm_fd = shm_open(name, O_RDWR, 0666);
	if (shm_fd == -1) {
		printf("shared memory failed\n");
		exit(-1);
	}

	/* now map the shared memory segment in the address space of the process */
	ptr = (DataTableType*) mmap(0,SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (ptr == MAP_FAILED) {
		printf("Map failed\n");
		exit(-1);
	}

	/* remove the shared memory segment */
	/*
	if (shm_unlink(name) == -1) {
		printf("Error removing %s\n",name);
		exit(-1);
	}
	 */

	return ptr;

}

// this is a struct used for each thread for calculating product of matrices
typedef struct {
	int i; /* row */
	int j; /* column */
	DataTableType *dataTable;
} ThreadData;

void *oneThread(void *param);

/*
 * this method accepts a DataTableType pointer and creates threads to calculate product of matrices
 */
void calcMatrixThreaded(DataTableType *dataTable){

	if ( dataTable->matrixA.numberOfRows != dataTable->matrixB.numberOfColumns ){
		printf("Matrices with entered orders can't be multiplied with each other.\n");
		exit(0);
	}else{



		int i,j, count =0;

		for (i = 0 ; i < dataTable->matrixA.numberOfRows ; i++ ){
			for (j = 0 ; j < dataTable->matrixB.numberOfColumns ; j++ ){

				ThreadData *data = malloc(sizeof(ThreadData*));
				data->i = i;
				data->j = j;
				data->dataTable = dataTable;

				pthread_t tid;       //Thread ID
				pthread_attr_t attr; //Set of thread attributes
				//Get the default attributes
				pthread_attr_init(&attr);
				//Create the thread
				pthread_create(&tid,&attr,oneThread,data);
				//Make sure the parent waits for all thread to complete
				pthread_join(tid, NULL);
				count++;
			}
		}

		printf("Total number of threads used for calculating multiplication: %d\n", count);

	}

}

/*
 * this methods is a function that is called by one thread. it reads a row and columns and calculates their value
 * to be used in the result matrix (which will be the product of the matrices)
 */
void *oneThread(void *param) {
	ThreadData *data = param; // the structure that holds our data

	int n, sum = 0; //the counter and sum

	for (n = 0 ; n < data->dataTable->matrixB.numberOfColumns ; n++ ){
		sum = sum + data->dataTable->matrixA.matrix[data->i][n] * data->dataTable->matrixB.matrix[n][data->j];
	}

	data->dataTable->mResult.matrix[data->i][data->j] = sum;

	//Exit the thread
	pthread_exit(0);
}

/*
 * this method prints the resulting matrix
 */
void printMatrix(DataTableType *dataTable){

	printf("Product of matrices A and B:\n");
	int i,j;

	for (i = 0 ; i < dataTable->matrixA.numberOfRows ; i++ ){
		for (j = 0 ; j < dataTable->matrixB.numberOfColumns ; j++ )
			printf("%d\t", dataTable->mResult.matrix[i][j]);

		printf("\n");
	}
}

/*
 * this method prints the given matrices
 */
void printMatrixAB(DataTableType *dataTable){

	printf("Matrices A and B:\n");
	int i,j;

	printf("Matrix A:\n");
	for (i = 0 ; i < dataTable->matrixA.numberOfRows ; i++ ){
		for (j = 0 ; j < dataTable->matrixA.numberOfColumns ; j++ )
			printf("%d\t", dataTable->matrixA.matrix[i][j]);

		printf("\n");
	}

	printf("\nMatrix B:\n");
	for (i = 0 ; i < dataTable->matrixB.numberOfRows ; i++ ){
		for (j = 0 ; j < dataTable->matrixB.numberOfColumns ; j++ )
			printf("%d\t", dataTable->matrixB.matrix[i][j]);

		printf("\n");
	}
}














