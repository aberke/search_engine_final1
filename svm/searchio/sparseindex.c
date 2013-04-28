/*
    SparseIndex
    A lazy-loading implementation of the CS158 Search Engine index.
*/

#include "sparseindex.h"
#include <fcntl.h>
#include <arpa/inet.h>
#include "searchio.h"

/* Object struct */
struct SparseIndex_s {
    PyObject_HEAD
    int fd;
    uint32_t postingsStart;
    PyObject *terms;
};

/* Type object */
static PyMappingMethods SparseIndexMappingMethods = {
    &SparseIndex_Length,
    &SparseIndex_GetItem,
    NULL
};

static PySequenceMethods SparseIndexSequenceMethods = {
    &SparseIndex_Length,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    &SparseIndex_Contains,
    NULL,
    NULL
};

PyTypeObject SparseIndexType = {
    PyObject_HEAD_INIT(NULL)
    0,                                          /*ob_size*/
    "searchio.SparseIndex",                     /*tp_name*/
    sizeof(SparseIndex),                        /*tp_basicsize*/
    0,                                          /*tp_itemsize*/
    (destructor)&SparseIndex_dealloc,           /*tp_dealloc*/
    0,                                          /*tp_print*/
    0,                                          /*tp_getattr*/
    0,                                          /*tp_setattr*/
    0,                                          /*tp_compare*/
    0,                                          /*tp_repr*/
    0,                                          /*tp_as_number*/
    &SparseIndexSequenceMethods,                /*tp_as_sequence*/
    &SparseIndexMappingMethods,                 /*tp_as_mapping*/
    0,                                          /*tp_hash */
    0,                                          /*tp_call*/
    0,                                          /*tp_str*/
    0,                                          /*tp_getattro*/
    0,                                          /*tp_setattro*/
    0,                                          /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /*tp_flags*/
    "SparseIndex objects",                      /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    &SparseIndex_GetIter,                       /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    (initproc)&SparseIndex_init,                /* tp_init */
    0,                                          /* tp_alloc */
    0,                                          /* tp_new */
};

/* Initializers */
PyObject *SparseIndex_new(int fd)
{
    /* allocate the object */
    SparseIndex *self = (SparseIndex *)(SparseIndexType.tp_alloc(&SparseIndexType, 0));
    if (self != NULL)
    {
        self->fd = fd;
        self->postingsStart = 0;
        self->terms = NULL;
        
        /* rebuild the sparse index */
        SparseIndex_reconstruct(self);
    }
    
    return (PyObject *)self;
}
int SparseIndex_init(SparseIndex *self, PyObject *args, PyObject *kwds)
{
    /* grab the filename */
    const char *filename = NULL;
    static char *kwlist[] = {"filename", NULL};
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, &filename))
        return -1;
    
    if (filename != NULL)
    {
        if (self->fd > 2)
        {
            close(self->fd);
            self->fd = -1;
        }
        
        /* open the new file */
        self->fd = open(filename, O_RDONLY);
        if (self->fd == -1)
        {
            PyErr_SetFromErrno(PyExc_IOError);
            return -1;
        }
        
        /* rebuild the sparse index */
        SparseIndex_reconstruct(self);
    }
    
    return 0;
}

/* Deallocator */
void SparseIndex_dealloc(SparseIndex *self)
{
    if (self->fd > 2)
        close(self->fd);
    
    Py_XDECREF(self->terms);
    self->ob_type->tp_free((PyObject *)self);
}

/* Loading the index */
void SparseIndex_reconstruct(SparseIndex *self)
{
    /* we assume an open file */
    if (self->fd == -1)
        return;
    
    /* start by resetting our properties */
    self->postingsStart = 0;
    Py_XDECREF(self->terms);
    self->terms = PyDict_New();
    
    /* create a buffer for reading terms */
    char *termBuffer = (char *)malloc(sizeof(char) * SEARCHIO_TOKENIZER_BUFFER_SIZE);
    
    /* read the header */
    searchio_index_header_t header;
    lseek(self->fd, 0, SEEK_SET);
    read(self->fd, (void *)&header, sizeof(header));
    
    /* normalize header values */
    header.numDocuments = ntohl(header.numDocuments);
    header.numTerms = ntohl(header.numTerms);
    header.postingsStart = ntohl(header.postingsStart);
    
    /* store our postings start */
    self->postingsStart = header.postingsStart;
    
    /* loop over the terms in the index, add them to the dictionary */
    uint32_t i;
    for (i = 0; i < header.numTerms; i++)
    {
        /* read and normalize a term header */
        searchio_index_term_t term;
        read(self->fd, (void *)&term, sizeof(term));
        
        term.postingsOffset = ntohl(term.postingsOffset);
        term.df = ntohl(term.df);
        term.numDocumentsInPostings = ntohl(term.numDocumentsInPostings);
        term.termLength = ntohs(term.termLength);
        
        /* read the term, create a Python string */
        read(self->fd, (void *)termBuffer, term.termLength);
        termBuffer[term.termLength] = '\0';
        
        PyObject *termStr = PyString_FromString(termBuffer);
        
        /* store the result in the index */
        PyObject *termEntry = PyDict_New();
        PyDict_SetItemString(termEntry, "postingsOffset", PyLong_FromUnsignedLong(term.postingsOffset));
        PyDict_SetItemString(termEntry, "df", PyLong_FromUnsignedLong(term.df));
        PyDict_SetItemString(termEntry, "numDocumentsInPostings", PyLong_FromUnsignedLong(term.numDocumentsInPostings));
        
        PyDict_SetItem(self->terms, termStr, termEntry);
    }
    
    /* clean up */
    free(termBuffer);
}

/* Mapping methods */
Py_ssize_t SparseIndex_Length(PyObject *o)
{
    SparseIndex *self = (SparseIndex *)o;
    
    /* return the size of the term dictionary */
    return PyDict_Size(self->terms);
}
PyObject *SparseIndex_GetItem(PyObject *o, PyObject *key)
{
    SparseIndex *self = (SparseIndex *)o;
    
    /* check if the term is in the dictionary */
    PyObject *term = PyDict_GetItem(self->terms, key);
    if (term == NULL)
    {
        PyErr_SetObject(PyExc_KeyError, key);
        return NULL;
    }
    
    /* check if we have postings for the item */
    PyObject *postingsResult = PyDict_GetItemString(term, "postings");
    if (postingsResult != NULL)
    {
        Py_INCREF(postingsResult);
        return postingsResult;
    }
    
    /* looks like we have to lazily load the postings; start by seeking to the right place */
    size_t offset = PyInt_AsUnsignedLongMask(PyDict_GetItemString(term, "postingsOffset"));
    lseek(self->fd, self->postingsStart + offset, SEEK_SET);
    
    /* create a postings list, load the postings */
    uint32_t numDocumentsInPostings = (uint32_t)PyInt_AsUnsignedLongMask(PyDict_GetItemString(term, "numDocumentsInPostings"));
    PyObject *postings = PyList_New(numDocumentsInPostings);
    
    uint32_t i;
    for (i = 0; i < numDocumentsInPostings; i++)
    {
        /* grab the posting header */
        searchio_index_posting_t posting;
        read(self->fd, &posting, sizeof(posting));
            
        /* normalize it */
        posting.pageID = ntohl(posting.pageID);
        posting.wf = ntohl(posting.wf);
        posting.numPositions = ntohl(posting.numPositions);
            
        /* create a positions list, fill it out */
        PyObject *positions = PyList_New(posting.numPositions);
        uint32_t j;
        for (j = 0; j < posting.numPositions; j++)
        {
            uint32_t position;
            read(self->fd, &position, sizeof(position));
            
            PyList_SetItem(positions, j, PyLong_FromUnsignedLong(ntohl(position)));
        }
            
        /* add an entry to the postings list */
        PyObject *entry = PyList_New(3);
        PyList_SetItem(entry, 0, PyLong_FromUnsignedLong(posting.pageID));
        PyList_SetItem(entry, 1, PyFloat_FromDouble((double)posting.wf / (double)SEARCHIO_WF_SCALE));
        PyList_SetItem(entry, 2, positions);
        PyList_SetItem(postings, i, entry);
    }
        
    /* store the result in the index */
    PyObject *termEntry = PyList_New(2);
    PyList_SetItem(termEntry, 0, PyDict_GetItemString(term, "df"));
    PyList_SetItem(termEntry, 1, postings);
    PyDict_SetItemString(term, "postings", termEntry);
    
    Py_INCREF(termEntry);
    return termEntry;
}
int SparseIndex_Contains(PyObject *o, PyObject *value)
{
    SparseIndex *self = (SparseIndex *)o;
    
    /* check if the term is in the dictionary */
    return PyDict_Contains(self->terms, value);
}
PyObject *SparseIndex_GetIter(PyObject *o)
{
    SparseIndex *self = (SparseIndex *)o;
    return PyObject_GetIter(self->terms);
}
