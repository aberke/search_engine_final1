/*
    linalgebra
    A Python module for interacting with CS158 Search Engine matrices.  Based on Matt's searchio.
*/

#include "linalgebra.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <math.h>   

/* Module method declarations */
static PyObject *linalgebra_difference_normsq(PyObject *self, PyObject *args);
static PyObject *linalgebra_compute_pagerank(PyObject *self, PyObject *args);

/* Module method table */
static PyMethodDef LinalgebraMethods[] = {
    {"difference_normsq", &linalgebra_difference_normsq, METH_VARARGS, "helper for testing -- finds the norm of the difference of two vectors"},
    {"compute_pagerank", &linalgebra_compute_pagerank, METH_VARARGS, "does work for pagerank.py of turning adjacentry matrix to stochastic matrix and computes pagerank vector"},
    {NULL, NULL, 0, NULL}
};

/* Initialization function */
PyMODINIT_FUNC initlinalgebra(void)
{
    /* initialize the module */
    PyObject *m = Py_InitModule("linalgebra", LinalgebraMethods);
}
/**************************** FOR PAGERANK.PY ****************************/

/* linalgebra_stochastic_row_t constructor 
input: 1) adjancy matrix A  as dictionary mapping row to list of nnz entries {i:[j for j in docIDs if i links to j]}
       2) row i of A that this should represent
       3) N:= total number of pageIDs
       4) alpha:= constant in stochastic matrix
output: row n of stochastic matrix P */
linalgebra_stochastic_row_t* linalgebra_stochastic_row_init(PyObject *A, PyObject *i, int N, double alpha){
    // extract row n from A
    PyObject *nnz_listPy = PyDict_GetItem(A, i);
    int n = (int)PyList_Size(nnz_listPy);

    linalgebra_stochastic_row_t* row = malloc(sizeof(struct linalgebra_stochastic_row) + (n+1)*sizeof(int));
    // set nnz_entry
    row->nnz_entry = ((1-alpha)/n) + (alpha/N);
    // fill in nnz_array
    int j;
    for(j=0; j<n; j++)
        (row->nnz_array)[j] = (int)PyLong_AsLong(PyList_GetItem(nnz_listPy, j));
    // put a (-1) in the last entry to mark the end
    (row->nnz_array)[n] = -1;
    return row;
}
/******** Helper to linalgebra_compute_pagerank: computes normsq of difference of two input vectors as arrays ******/
double compute_normsq_diff(double vec1[], double vec2[], int N){
    double normsq = 0;
    int i;
    for(i=0; i<N; i++)
        normsq = normsq + (vec1[i] - vec2[i])*(vec1[i]-vec2[i]);
    return normsq;
}
/******** Helper to linalgebra_compute_pagerank: fills array with all zeros ******/
void zero_array(double array[], int N){
    int i;
    for(i=0; i<N; i++)
        array[i] = 0;
}
/******** Helper to linalgebra_compute_pagerank: computes norm of "vector" ******/
double compute_norm(double vector[], int N){
    double normsq = 0;
    int i;
    for(i=0; i<N; i++)
        normsq = normsq + vector[i]*vector[i];
    return sqrt(normsq);
}

/****************************
            adjancy matrix A  as dictionary mapping row to list of nnz entries {i:[j for j in docIDs if i links to j]}
    def compute_pagerank(A, alpha, id_list, iterations):
        compute stochastic matrix as:
        
        int N
        zero_entry = alpha/N
        stochastic_row_t rows[N]

        pagerank = [1,0,0,...0]
        for i in range(iterations):
            pagerank = pagerank*P

        return pagerank
*************************************/
static PyObject *linalgebra_compute_pagerank(PyObject *self, PyObject *args){
    /************ unpack arguments ***********/
    PyObject *A = NULL;
    double alpha = 0;
    PyObject *id_listPy = NULL;
    int iterations = 0;
    if (!PyArg_ParseTuple(args, "OdOi", &A, &alpha, &id_listPy, &iterations))
        return NULL;
    /************ unpack arguments ***********/
    
    /******************************* initialize:  
        int N = total pageIDs
        double zero_entry:= alpha/N  <-- value of entries that don't appear in sparse representation
        int pageIDs[N]:= array of pageIDs
        rows[N]:= array of rows taken from A as array of pointers to linalgebra_stochastic_row_t's
    ***********/
    Py_ssize_t NPy = PyList_Size(id_listPy);
    int N = (int)NPy;
    double zero_entry = alpha/N;
    int pageIDs[N];
    linalgebra_stochastic_row_t* rows[N];
    int i,j,k;
    for(i=0; i<N; i++){
        PyObject *row_indexPy = PyList_GetItem(id_listPy, i);
        pageIDs[i] = (int)PyLong_AsLong(row_indexPy);
        rows[i] = linalgebra_stochastic_row_init(A, row_indexPy, N, alpha);
    }
    /************************* initialized: have stochastic matrix as array of rows ***********/
    printf("N:%i, alpha:%f, zero_entry:%f\n",N,alpha,zero_entry);

    /************************ Initilize pagerank[N]:= [1,0,0,....,0] ******************/
    double pagerank[N];
    double temp[N];
    zero_array(pagerank,N); //zeros out the array of doubles
    pagerank[0] = 1;
    /************************ Initilized pagerank[N]:= [1,0,0,....,0] ******************/
    printf("pagerank initialized\n");
    /************** Compute pagerank*P <iterations> times! *******************/
    for(k=0; k<iterations; k++){
        zero_array(temp,N);
        for(i=0; i<N; i++){
            // extract all necessary values for this particular row before looking at column by column
            double vec_i = pagerank[i];
            linalgebra_stochastic_row_t* row_i = rows[i];
            double nnz_entry = row_i->nnz_entry;

            // setup starting variables
            int nnz_index = 0;
            int nnz = (row_i->nnz_array)[0];
            double value;

            for(j=0; j<N; j++){
                // get value we're multiplying vec_i by
                if((nnz > j)||(nnz < 0)) // I put a (-1) at the end of each row's nnz_array to mark the end
                    value = zero_entry;
                else{
                    value = nnz_entry;
                    nnz_index ++;
                    nnz = (row_i->nnz_array)[nnz_index];
                }
                temp[j] = temp[j] + (vec_i*value);
            }
        }
        // obtained result!
        double norm = compute_norm(temp, N);
        printf("iteration %i: ***********************\nnormsq(pagerank, temp)= %f\nnorm: %f\n", k, compute_normsq_diff(pagerank, temp, N),norm);
        // reset pagerank as (normalized) result:
        double sum = 0;
        for(j=0; j<N; j++){
            pagerank[j] = temp[j]/norm; // <-- scaling each entry up by norm -- bad idea?  Definitely not quite honest
            sum = sum + pagerank[j];
        }
        printf("new pagerank entries sum to %f\n",sum);
    }
    // free all the rows
    for(i=0; i<N; i++){
        int row_index = pageIDs[i];
        free(rows[row_index]);
    }

    /* create a result dictionary and fill it in */
    PyObject *result = PyDict_New();
    for(i=0; i<N; i++){
        PyObject *id = PyFloat_FromDouble(pageIDs[i]);
        PyDict_SetItem(result, id, PyFloat_FromDouble(pagerank[i]));
    }
    return result;
}

/****************************
    def difference_normsq(vec1, vec2):
        norm = 0
        for k in vec1: #neither of these are sparse vectors
            norm += (vec1[k]-vec2[k])**2
        return norm
*************************************/
static PyObject *linalgebra_difference_normsq(PyObject *self, PyObject *args){
    PyObject *vec1 = NULL;
    PyObject *vec2 = NULL;
    if (!PyArg_ParseTuple(args, "OO", &vec1, &vec2))
        return NULL;
    
    double normsq = 0;
    PyObject *keys = PyDict_Keys(vec1);
    Py_ssize_t keyLen = PyList_Size(keys);
    long keyIndex;
    for(keyIndex = 0; keyIndex < keyLen; keyIndex++){
        PyObject *key = PyList_GetItem(keys, keyIndex);
        PyObject *v1py = PyDict_GetItem(vec1, key);
        PyObject *v2py = PyDict_GetItem(vec2, key);

        double v1 = PyFloat_AS_DOUBLE(v1py);
        double v2 = PyFloat_AS_DOUBLE(v2py);
        double diff = v1-v2;
        normsq = normsq + (diff*diff);
    }
    return Py_BuildValue("f", normsq);
}