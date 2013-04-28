/*
    searchio
    A Python module for interacting with CS158 Search Engine indices.
*/

#ifndef __SEARCHIO_H__
#define __SEARCHIO_H__

#include <Python.h>

/* Constants */
#define SEARCHIO_TOKENIZER_BUFFER_SIZE 1024 * 1024
#define SEARCHIO_WF_SCALE 100000

/* Handy macros */
#define SEARCHIO_MAX(a, b) ((a < b) ? b : a)
#define SEARCHIO_MIN(a, b) ((a > b) ? b : a)

/* Index file structures */
#pragma pack(push, 1)
typedef struct searchio_index_header {
    uint32_t numDocuments;
    uint32_t numTerms;
    uint32_t postingsStart;
} searchio_index_header_t;

typedef struct searchio_index_term {
    uint32_t postingsOffset;
    uint32_t df;
    uint32_t numDocumentsInPostings;
    uint16_t termLength;
} searchio_index_term_t;

typedef struct searchio_index_posting {
    uint32_t pageID;
    uint32_t wf;
    uint32_t numPositions;
} searchio_index_posting_t;

#pragma pack(pop)

#endif
