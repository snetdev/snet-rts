/*
 * ejpgl.c
 *
 *  Created on: Feb 13, 2013
 *      Author: vicentsanzmarco
 */

#include "ejpgl.h"

// Function for set and get in the array for signed char
void set_array_char( int row, int matrix_size, int col, signed char value, signed char *array )
{
	array[ ( row * matrix_size ) + col] = value;
}

signed char get_array_char( int row, int matrix_size, int col, signed char *array )
{
	return (array + row * matrix_size) [col] ;
}

void set_array_short( int row, int matrix_size, int col, signed short value, signed short *array )
{
	array[ ( row * matrix_size ) + col] = value;
}

signed short get_array_short( int row, int matrix_size, int col, signed short *array )
{
	return (array + row * matrix_size) [col] ;
}

