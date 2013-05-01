/*
    linalgebra
    A Python module for interacting with CS158 Search Engine matrices.  Based on Matt's searchio.
*/

#ifndef __LINALGEBRA_H__
#define __LINALGEBRA_H__

#include <Python.h>

/* Handy macros */
#define LINALGEBRA_MAX(a, b) ((a < b) ? b : a)
#define LINALGEBRA_MIN(a, b) ((a > b) ? b : a)


typedef struct linalgebra_stochastic_row {
    double nnz_entry;
    int nnz_array[];
} linalgebra_stochastic_row_t;


#endif