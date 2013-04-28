/*
    searchio
    A Python module for interacting with CS158 Search Engine indices.
*/

#include "searchio.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include "stemmer.h"
#include "sparseindex.h"

/* Global variables */
static char *searchio_tokenizerBuffer = NULL;

/* Module method declarations */
static PyObject *searchio_tokenize(PyObject *self, PyObject *args);
static PyObject *searchio_createIndex(PyObject *self, PyObject *args);
static PyObject *searchio_loadIndex(PyObject *self, PyObject *args);
static PyObject *searchio_loadSparseIndex(PyObject *self, PyObject *args);

/* Module method table */
static PyMethodDef SearchioMethods[] = {
    {"tokenize", &searchio_tokenize, METH_VARARGS, "Obtain a viable list of tokens from a string."},
    {"createIndex", &searchio_createIndex, METH_VARARGS, "Create an on-disk representation of the provided index."},
    {"loadIndex", &searchio_loadIndex, METH_VARARGS, "Load an index from disk."},
    {"loadSparseIndex", &searchio_loadSparseIndex, METH_VARARGS, "Load only the terms of an index from disk, and return an object that reads postings lists on demand."},
    {NULL, NULL, 0, NULL}
};

/* Initialization function */
PyMODINIT_FUNC initsearchio(void)
{
    /* initialize the tokenizer buffer */
    searchio_tokenizerBuffer = (char *)malloc(sizeof(char) * SEARCHIO_TOKENIZER_BUFFER_SIZE);
    
    /* initialize the SparseIndex type */
    SparseIndexType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&SparseIndexType) < 0)
        return;
    
    /* initialize the module */
    PyObject *m = Py_InitModule("searchio", SearchioMethods);
    
    /* register the SparseIndex type */
    Py_INCREF(&SparseIndexType);
    PyModule_AddObject(m, "SparseIndex", (PyObject *)&SparseIndexType);
}

/* Method implementations */
static PyObject *searchio_tokenize(PyObject *self, PyObject *args)
{
    /* get the set of stopwords, and the string to tokenize */
    PyObject *stopwords = NULL;
    const char *textString = NULL;
    int keepStar = 0;
    
    if (!PyArg_ParseTuple(args, "Osi", &stopwords, &textString, &keepStar))
        return NULL;
    
    /* loop over the characters in textString to normalize it */
    size_t len = SEARCHIO_MIN(strlen(textString), SEARCHIO_TOKENIZER_BUFFER_SIZE - 1);
    size_t i;
    size_t wordsFound = 1;
    for (i = 0; i < len; i++)
    {
        char c = tolower(textString[i]);
        if (isalnum(c) || (keepStar && c == '*'))
            searchio_tokenizerBuffer[i] = c;
        else
        {
            searchio_tokenizerBuffer[i] = '\0';
            wordsFound++;
        }
    }
    
    /* NUL-terminate our buffer */
    searchio_tokenizerBuffer[i] = '\0';
    
    /* create a result list */
    PyObject *result = PyList_New(0);
    
    /* loop over each string, check if it's a stopword, and if not, stem it */
    char *str = searchio_tokenizerBuffer;
    for (i = 0; i < wordsFound; i++)
    {
        size_t l = strlen(str);
        if (l == 0)
        {
            str += 1;
            continue;
        }
        
        /* convert to a Python string */
        PyObject *pystr = PyString_FromString(str);
        
        /* is it a stop word? */
        if (!PySet_Contains(stopwords, pystr))
        {
            if (!keepStar || strchr(str, '*') == NULL)
            {
                /* stem it */
                int newEnd = stem(str, 0, (int)(l - 1));
                str[newEnd + 1] = '\0';
            }
            
            /* add it to the list */
            PyList_Append(result, PyString_FromString(str));
        }
        
        Py_DECREF(pystr);
        str += (l + 1);
    }
    
    return result;
}
static PyObject *searchio_createIndex(PyObject *self, PyObject *args)
{
    /* grab the filename, number of documents, and our index */
    const char *filename = NULL;
    uint32_t numDocuments = 0;
    PyObject *index = NULL;
    
    if (!PyArg_ParseTuple(args, "sIO", &filename, &numDocuments, &index))
        return NULL;
    
    /* open the index file */
    int fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC);
    if (fd == -1)
        return PyErr_SetFromErrno(PyExc_IOError);
    
    fchmod(fd, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    size_t totalWritten = 0;
    
    /* get the number of terms in the index */
    Py_ssize_t numTermsPy = PyDict_Size(index);
    uint32_t numTerms = 0;
    if (numTermsPy > 0 && numTermsPy < UINT32_MAX)
        numTerms = (uint32_t)numTermsPy;
    else
    {
        PyErr_SetString(PyExc_MemoryError, "the number of terms is greater than UINT32_MAX (or less than 0)");
        return NULL;
    }
    
    /* write a document header */
    searchio_index_header_t header = {htonl(numDocuments), htonl(numTerms), 0};
    totalWritten += write(fd, (void *)&header, sizeof(header));
    
    /* loop once over index to write term entries */
    PyObject *indexKeys = PyDict_Keys(index);
    Py_ssize_t indexKeyLen = PyList_Size(indexKeys);
    
    uint32_t postingsOffset = 0;
    uint32_t keyIdx;
    for (keyIdx = 0; keyIdx < indexKeyLen; keyIdx++)
    {
        PyObject *key = PyList_GetItem(indexKeys, keyIdx);
        PyObject *value = PyDict_GetItem(index, key);
        
        /* create a term entry */
        searchio_index_term_t term;
        term.postingsOffset = htonl(postingsOffset);
        
        /* pull out the term, its df, and a reference to the postings list */
        const char *termStr = PyString_AS_STRING(key);
        PyObject *pydf = PyList_GetItem(value, 0);
        PyObject *postings = PyList_GetItem(value, 1);
        Py_ssize_t postingsLen = PyList_Size(postings);
        
        /* fill in the header */
        size_t termLen = strlen(termStr);
        term.df = htonl(PyInt_AsUnsignedLongMask(pydf));
        term.numDocumentsInPostings = htonl((uint32_t)postingsLen);
        term.termLength = htons((uint16_t)termLen);
        
        /* write out the header, and its term */
        totalWritten += write(fd, (void *)&term, sizeof(term));
        totalWritten += write(fd, termStr, termLen);
        
        /* determine the on-disk size of the posting list */
        uint32_t totalSize = 0;
        Py_ssize_t i;
        for (i = 0; i < postingsLen; i++)
        {
            PyObject *entryList = PyList_GetItem(postings, i);
            PyObject *positions = PyList_GetItem(entryList, 2);
            Py_ssize_t positionLen = PyList_Size(positions);
            
            /* on-disk size is size of a posting struct, plus sizeof(uint32_t) * positions */
            totalSize += (uint32_t)sizeof(searchio_index_posting_t) + (uint32_t)(positionLen * sizeof(uint32_t));
        }
        
        /* increment the postings offset */
        postingsOffset += totalSize;
    }
    
    /* update the postings offset */
    header.postingsStart = htonl(totalWritten);
    
    /* loop again to write postings lists */
    for (keyIdx = 0; keyIdx < indexKeyLen; keyIdx++)
    {
        PyObject *key = PyList_GetItem(indexKeys, keyIdx);
        PyObject *value = PyDict_GetItem(index, key);
        
        /* get the postings list and its length */
        PyObject *postings = PyList_GetItem(value, 1);
        Py_ssize_t postingsLen = PyList_Size(postings);
        
        /* loop over each entry in the postings list and write it out */
        Py_ssize_t i;
        for (i = 0; i < postingsLen; i++)
        {
            /* pull out some values */
            PyObject *entryList = PyList_GetItem(postings, i);
            PyObject *pypageID = PyList_GetItem(entryList, 0);
            PyObject *pywf = PyList_GetItem(entryList, 1);
            PyObject *positions = PyList_GetItem(entryList, 2);
            Py_ssize_t positionsLen = PyList_Size(positions);
            
            /* create a header */
            searchio_index_posting_t posting;
            posting.pageID = htonl((uint32_t)PyInt_AsUnsignedLongMask(pypageID));
            posting.wf = htonl((uint32_t)(PyFloat_AsDouble(pywf) * SEARCHIO_WF_SCALE));
            posting.numPositions = htonl((uint32_t)positionsLen);
            
            /* write the header */
            write(fd, (void *)&posting, sizeof(posting));
            
            /* write each of the positions */
            Py_ssize_t j;
            for (j = 0; j < positionsLen; j++)
            {
                PyObject *item = PyList_GetItem(positions, j);
                uint32_t p = htonl((uint32_t)PyInt_AsUnsignedLongMask(item));
                write(fd, (void *)&p, sizeof(p));
            }
        }
    }
    
    /* rewrite the header with correct offsets */
    lseek(fd, 0, SEEK_SET);
    write(fd, (void *)&header, sizeof(header));
    
    /* close the index */
    close(fd);
    
    /* no meaningful return value here */
    Py_RETURN_NONE;
}
static PyObject *searchio_loadIndex(PyObject *self, PyObject *args)
{
    /* grab the filename */
    const char *filename = NULL;
    
    if (!PyArg_ParseTuple(args, "s", &filename))
        return NULL;
    
    /* open the file, read the header */
    int fd = open(filename, O_RDONLY);
    if (fd == -1)
        return PyErr_SetFromErrno(PyExc_IOError);
    
    struct stat indexStat;
    fstat(fd, &indexStat);
    
    searchio_index_header_t header;
    read(fd, (void *)&header, sizeof(header));
    
    /* normalize header values */
    header.numDocuments = ntohl(header.numDocuments);
    header.numTerms = ntohl(header.numTerms);
    header.postingsStart = ntohl(header.postingsStart);
    
    /* allocate a postings buffer and load the postings into memory */
    size_t postingsBufSize = ((size_t)indexStat.st_size - header.postingsStart);
    void *postingsBuf = malloc(postingsBufSize);
    lseek(fd, header.postingsStart, SEEK_SET);
    read(fd, postingsBuf, postingsBufSize);
    
    lseek(fd, sizeof(header), SEEK_SET);
    
    /* create a result dictionary */
    PyObject *result = PyDict_New();
    
    /* loop over the terms in the index, add them to the dictionary */
    uint32_t i;
    for (i = 0; i < header.numTerms; i++)
    {
        /* read and normalize a term header */
        searchio_index_term_t term;
        read(fd, (void *)&term, sizeof(term));
        
        term.postingsOffset = ntohl(term.postingsOffset);
        term.df = ntohl(term.df);
        term.numDocumentsInPostings = ntohl(term.numDocumentsInPostings);
        term.termLength = ntohs(term.termLength);
        
        /* read the term, create a Python string */
        read(fd, (void *)searchio_tokenizerBuffer, term.termLength);
        searchio_tokenizerBuffer[term.termLength] = '\0';
        
        PyObject *termStr = PyString_FromString(searchio_tokenizerBuffer);
        
        /* create a postings list, load the postings */
        PyObject *postings = PyList_New(term.numDocumentsInPostings);
        uint32_t j;
        size_t offset = term.postingsOffset;
        for (j = 0; j < term.numDocumentsInPostings; j++)
        {
            /* grab the posting header */
            searchio_index_posting_t posting;
            memcpy(&posting, postingsBuf + offset, sizeof(posting));
            
            /* normalize it */
            posting.pageID = ntohl(posting.pageID);
            posting.wf = ntohl(posting.wf);
            posting.numPositions = ntohl(posting.numPositions);
            
            /* create a positions list, fill it out */
            PyObject *positions = PyList_New(posting.numPositions);
            uint32_t k;
            for (k = 0; k < posting.numPositions; k++)
            {
                uint32_t *position = ((uint32_t *)(postingsBuf + offset + sizeof(posting)) + k);
                PyList_SetItem(positions, k, PyLong_FromUnsignedLong(ntohl(*position)));
            }
            
            /* add an entry to the postings list */
            PyObject *entry = PyList_New(3);
            PyList_SetItem(entry, 0, PyLong_FromUnsignedLong(posting.pageID));
            PyList_SetItem(entry, 1, PyFloat_FromDouble((double)posting.wf / (double)SEARCHIO_WF_SCALE));
            PyList_SetItem(entry, 2, positions);
            PyList_SetItem(postings, j, entry);
            
            /* move to the next posting */
            offset += (sizeof(posting) + (sizeof(uint32_t) * posting.numPositions));
        }
        
        /* store the result in the index */
        PyObject *termEntry = PyList_New(2);
        PyList_SetItem(termEntry, 0, PyLong_FromUnsignedLong(term.df));
        PyList_SetItem(termEntry, 1, postings);
        PyDict_SetItem(result, termStr, termEntry);
    }
    
    /* close the index and clean up */
    close(fd);
    free(postingsBuf);
    
    return PyTuple_Pack(2, result, PyLong_FromUnsignedLong(header.numDocuments));
}
static PyObject *searchio_loadSparseIndex(PyObject *self, PyObject *args)
{
    /* grab the filename */
    const char *filename = NULL;
    
    if (!PyArg_ParseTuple(args, "s", &filename))
        return NULL;
    
    /* open the file, read the header */
    int fd = open(filename, O_RDONLY);
    if (fd == -1)
        return PyErr_SetFromErrno(PyExc_IOError);
    
    searchio_index_header_t header;
    read(fd, (void *)&header, sizeof(header));
    
    /* normalize header values */
    header.numDocuments = ntohl(header.numDocuments);
    header.numTerms = ntohl(header.numTerms);
    header.postingsStart = ntohl(header.postingsStart);
    
    /* construct and return a sparse index */
    PyObject *index = SparseIndex_new(fd);
    
    return PyTuple_Pack(2, index, PyLong_FromUnsignedLong(header.numDocuments));
}
