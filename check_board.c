///////////////////////////////////////////////////////////////////////////////
// Main File:        check_board
// This File:        check_board.c
// Semester:         CS 354 Lecture 001      SPRING 2024
// Instructor:       deppeler
// 
// Author:           Lojain Adly
// Email:            ladly@wisc.edu
// CS Login:         lojain
///////////////////////////////////////////////////////////////////////////////
// Copyright 2021-24 Deb Deppeler
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *DELIM = ",";  // commas ',' are a common delimiter character for data strings

#define MAX_SIZE 9

/* TODO: implement this function
 * Returns 1 if and only if the board is in a valid Sudoku board state.
 * Otherwise returns 0.
 * 
 * A valid row or column contains only blanks or the digits 1-size, 
 * with no duplicate digits, where size is the value 1 to 9.
 * 
 * Note: p2A requires only that each row and each column are valid.
 * 
 * board: heap allocated 2D array of integers 
 * size:  number of rows and columns in the board
 */
int valid_board(int **board, int size) {

	if(size < 1 || size > MAX_SIZE){
		return 0;
	}
	//arrays to keep track of values in rows and columns 
	int **row_seen = (int **)malloc(size * sizeof(int *));
	int **col_seen = (int **)malloc(size * sizeof(int *));
	if(row_seen == NULL || col_seen == NULL){
		printf("Something went wrong");
		return 0;
	}
	for(int i =0; i < size; i++){
		*(row_seen+i) = (int *)malloc(size * sizeof(int));
		*(col_seen+i) = (int *)malloc(size * sizeof(int));
		if(*(row_seen+i) == NULL|| *(col_seen+i) == NULL){
			printf("Something went wrong");
			return 0;
		}
		memset(*(row_seen+i), 0, size * sizeof(int));
		memset(*(col_seen+i), 0, size * sizeof(int));

	}
	//check for duplicates
	for(int i =0; i < size; i++){
		for(int j=0; j < size; j++){
			int val = *(*(board+i)+j);
			//check for valid values
			if(val < 0 || val > size){
				for(int i =0; i < size; i++){
					free(*(row_seen+i));
					free(*(col_seen+i)); 
				}
				free(row_seen);
				free(col_seen);
				return 0;
			}
			if(val != 0){
				//check for duplicates in the same row
				if(*(*(row_seen+i)+val-1) == 1){
					for(int i =0; i < size; i++){
						free(*(row_seen+i));
						free(*(col_seen+i)); 
					}
					free(row_seen);
					free(col_seen);
					return 0;
				}
				*(*(row_seen+i)+val-1) = 1;
				//check for duplicates in the same column
				if(*(*(col_seen+j)+val-1) == 1){
					for(int i =0; i < size; i++){
						free(*(row_seen+i));
						free(*(col_seen+i)); 
					}
					free(row_seen);
					free(col_seen);
					return 0;
				}
				*(*(col_seen+j)+val-1) = 1;
			}
		}
	}  
	//free dynamically allocated memeory 
	for(int i =0; i < size; i++){
		free(*(row_seen+i));
		free(*(col_seen+i)); 
	}
	free(row_seen);
	free(col_seen);


	return 1; //board is valid  
}    

/* COMPLETED (DO NOT EDIT):       
 * Read the first line of file to get the size of that board.
 * 
 * PRE-CONDITION #1: file exists
 * PRE-CONDITION #2: first line of file contains valid non-zero integer value
 *
 * fptr: file pointer for the board's input file
 * size: a pointer to an int to store the size
 *
 * POST-CONDITION: the integer whos address is passed in as size (int *) 
 * will now have the size (number of rows and cols) of the board being checked.
 */
void get_board_size(FILE *fptr, int *size) {      
	char *line = NULL;
	size_t len = 0;

	// 'man getline' to learn about <stdio.h> getline
	if ( getline(&line, &len, fptr) == -1 ) {
		printf("Error reading the input file.\n");
		free(line);
		exit(1);
	}

	char *size_chars = NULL;
	size_chars = strtok(line, DELIM);
	*size = atoi(size_chars);

	// free memory allocated for line 
	free(line);
	line = NULL;
}


/* TODO: COMPLETE THE MAIN FUNCTION
 * This program prints "valid" (without quotes) if the input file contains
 * a valid state of a Sudoku puzzle board wrt to rows and columns only.
 * It prints "invalid" (without quotes) if the input file is not valid.
 *
 * Usage: A single CLA that is the name of a file that contains board data.
 *
 * argc: the number of command line args (CLAs)
 * argv: the CLA strings, includes the program name
 *
 * Returns 0 if able to correctly output valid or invalid.
 * Exit with a non-zero result if unable to open and read the file given.
 */
int main( int argc, char **argv ) {              

	// TODO: Check if number of command-line arguments is correct.
	if(argc != 2){
		printf("Usage: %s <input_filename>\n", *(argv+0));
		return 1;
	}
	// Open the file 
	FILE *fp = fopen(*(argv + 1), "r");
	if (fp == NULL) {
		printf("Can't open file for reading.\n");
		exit(1);
	}

	// will store the board's size, number of rows and columns
	int size;

	// TODO: Call get_board_size to read first line of file as the board size.
	get_board_size(fp, &size);
	// TODO: Dynamically allocate a 2D array for given board size.
	// You must dyamically create a 1D array of pointers to other 1D arrays of ints
	int **board = (int **)malloc(size * sizeof(int *));
	if(board == NULL){
		printf("Something went wrong");
		return 0;
	}

	for (int i = 0; i < size; i++){
		*(board+i) = (int *)malloc(size * sizeof(int *));
		if(*(board+i) == NULL){
			printf("Something is wrong");
			return 0;
		}
	}
	// Read the remaining lines.
	// Tokenize each line and store the values in your 2D array.
	char *line = NULL;
	size_t len = 0;
	char *token = NULL;
	for (int i = 0; i < size; i++) {

		// read the line
		if (getline(&line, &len, fp) == -1) {
			printf("Error while reading line %i of the file.\n", i+2);
			exit(1);
		}

		token = strtok(line, DELIM);
		for (int j = 0; j < size; j++) {
			// TODO: Complete the line of code below
			// to initialize elements of your 2D array.
			*(*(board+i)+j) = atoi(token); 
			token = strtok(NULL, DELIM);
		}
	}

	// TODO: Call valid_board and print the appropriate
	//       output depending on the function's return value.
	if(valid_board(board, size)){
		printf("valid\n");
	} else {
		printf("invalid\n");
	}
	// TODO: Free dynamically allocated memory.
	for (int i = 0; i < size; i++) {
		free(*(board+i));
	}
	free(board);
	if(line){
		free(line);
	}
	//Close the file.
	if (fclose(fp) != 0) {
		printf("Error while closing the file.\n");
		exit(1);
	} 

	return 0;       
}       

