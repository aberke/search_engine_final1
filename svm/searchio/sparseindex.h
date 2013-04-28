/*
    SparseIndex
    A lazy-loading implementation of the CS158 Search Engine index.
*/

#ifndef __SPARSEINDEX_H__
#define __SPARSEINDEX_H__

#include <Python.h>

/* Object struct */
typedef struct SparseIndex_s SparseIndex;

/* Type object */
extern PyTypeObject SparseIndexType;

/* Initializers and Deallocator */
PyObject *SparseIndex_new(int fd);
int SparseIndex_init(SparseIndex *self, PyObject *args, PyObject *kwds);
void SparseIndex_dealloc(SparseIndex *self);

/* Loading the index */
void SparseIndex_reconstruct(SparseIndex *self);

/* Mapping/sequence methods */
Py_ssize_t SparseIndex_Length(PyObject *o);
PyObject *SparseIndex_GetItem(PyObject *o, PyObject *key);
int SparseIndex_Contains(PyObject *o, PyObject *value);
PyObject *SparseIndex_GetIter(PyObject *o);

#endif
