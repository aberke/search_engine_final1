/*
    Porter stemming algorithm.
    Implemented in C by Dr. Martin Porter.
*/

/* Given p (the string), i (the start of the string), and j
    (the last character in the string before \0), stem(...) returns
    the index of the new last character in the string. */
int stem(char *p, int i, int j);
