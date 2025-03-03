///////////////////////////////////////////////////////////////////////////////
// Main File:        my_magic_square
// This File:        my_magic_square.c
// Other Files:      magic_square1.txt, magic_square3.txt, 
//					 magic_square5.txt, magic_square11.txt,
//					 magic_square13.txt, magic_square17.txt,
//					 magic_square19.txt
// Semester:         CS 354 Lecture 001      SPRING 2024
// Instructor:       deppeler
// 
// Author:           Lojain Adly
// Email:            ladly@wisc.edu
// CS Login:         lojain
///////////////////////////////////////////////////////////////////////////////
// Copyright 2020 Jim Skrentny
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Structure that represents a magic square
typedef struct {
	int size;           // dimension of the square
	int **magic_square; // pointer to heap allocated magic square
} MagicSquare;

/* TODO:
 * Promps the user for the magic square's size, read it,
 * check if it's an odd number >= 3 (if not, display the required
 * error message and exit)
 *
 * return the valid number
 */
int getSize() {

	int size;

	//prompt user
	printf("Enter magic square's size (odd integer >=3)");
	scanf("%d", &size);
	// check if the size is odd and greater than or equal to 3   
	if (size % 2 == 0)
	{
		printf("Magic square size must be odd.\n");
		exit(1);
	}

	if(size <3)
	{
		printf("Magic square size must be >= 3.\n");
		exit(1);
	}
	return size;
} 

/* TODO:
 * Makes a magic square of size n,
 * and stores it in a MagicSquare (on the heap)
 *
 * It may use the Siamese magic square algorithm 
 * or alternate from assignment 
 * or another valid algorithm that produces a magic square.
 *
 * n - the number of rows and columns
 *
 * returns a pointer to the completed MagicSquare struct.
 */
MagicSquare *generateMagicSquare(int n) {

	//alocate memory for square structure
	MagicSquare *mySquare = (MagicSquare *)malloc(sizeof(MagicSquare));
	if(mySquare == NULL)
	{
		perror("Memory allocation failed");
		exit(1);
	}

	//set the size in the square structure 
	mySquare->size = n;

	//allocate memory for the square array 
	mySquare->magic_square = (int **)malloc(n * sizeof(int));
	if(mySquare == NULL)
	{
		perror("Memory allocation failed");
		exit(1);
	}

	for (int i = 0; i < n; i++)
	{
		*(mySquare->magic_square + i) = (int *)malloc(n * sizeof(int));
		if(*(mySquare->magic_square + i) == NULL)
		{
			perror("Memory allocation failed");
			exit(1);
		}	
	}
	// initialize magic square
	int row = 0;
	int col = n / 2;
	for (int num = 1; num <= n * n; num++) 
	{
		*(*(mySquare->magic_square + row) + col) = num;

		// calculate next position
		int next_row = (row - 1 + n) % n;
		int next_col = (col + 1) % n;

		// move down if filled
		if (*(*(mySquare->magic_square + next_row) + next_col) != 0) 
		{
			row = (row + 1) % n;
		} else
		{
			row = next_row;
			col = next_col;
		}
	}

	return mySquare;

} 

/* TODO:  
 * Opens a new file (or overwrites the existing file)
 * and writes the magic square values to the file
 * in the specified format.
 *
 * magic_square - the magic square to write to a file
 * filename - the name of the output file
 */
void fileOutputMagicSquare(MagicSquare *magic_square, char *filename) {
	FILE *file = fopen(filename, "w");
	if(file == NULL)
	{
		perror("Error opening file for writing");
		exit(1);
	}

	//write size to file
	fprintf(file, "%d\n", magic_square->size);

	//write value to file
	for(int i = 0; i < magic_square->size; i++)
	{
		for (int j = 0; j < magic_square->size; j++)
		{
			fprintf(file, "%d", *(*(magic_square->magic_square + i) + j));
			if (j < magic_square->size - 1)
			{
				fprintf(file, ",");
			}
		}
		fprintf(file, "\n");
	}

	fclose(file);
}


/* TODO:
 * Generates a magic square of the user specified size and
 * outputs the square to the output filename.
 * 
 * Add description of required CLAs here
 */
int main(int argc, char **argv) {
	// TODO: Check input arguments to get output filename
	if(argc != 2)
	{
		printf("Usage: %s <output_filename>\n", *(argv + 0));
		exit(1);
	}

	//getting output filename from the command line args
	char *output_filename = *(argv + 1);

	// TODO: Get magic square's size from user
	int size = getSize();
	// TODO: Generate the magic square by correctly interpreting 
	//       the algorithm(s) in the write-up or by writing or your own.  
	//       You must confirm that your program produces a 
	//       Magic Sqare as described in the linked Wikipedia page.
	MagicSquare *mySquare = generateMagicSquare(size);
	// TODO: Output the magic square
	fileOutputMagicSquare(mySquare, output_filename);

	// Free allocated memory
	for (int i = 0; i < mySquare->size; i++) {
		free(*(mySquare->magic_square + i));
	}
	free(mySquare->magic_square);
	free(mySquare);

	return 0;
} 

// Spring 2024


