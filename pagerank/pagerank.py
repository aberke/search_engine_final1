# pagerank main file
import sys
import linalgebra
# global variables
alpha = 0.1
iterations = 128



# computes pagerank of collection and writes pagerank out to file where the ith line is the ith component of the pagerank vector
# input: 1) filename of collection of documents
#        2) filename of document to write to
def main(collection_filename, output_filename):
    # 1) create dictionary mapping title_map: {title: DocID}, dictionary mapping link_map: {docID: set(link for link in document)}, sorted list of docIDs
    (title_map, link_map, id_list) = parse(collection_filename)
    print('parsed')
    # 2) create adjancy matrix A  as dictionary mapping row to list of nnz entries {i:[j for j in docIDs if i links to j]}
    A = create_adjacency_matrix(title_map, link_map)
    print('create_adjacency_matrix')
    pagerank = linalgebra.compute_pagerank(A, alpha, id_list, iterations)
    print('done with pagerank')
    # print pagerank to file
    print_output(output_filename, pagerank, id_list)
    return

# version that uses python rather than C module
def main2(collection_filename, output_filename):
    # 1) create dictionary mapping title_map: {title: DocID}, dictionary mapping link_map: {docID: set(link for link in document)}, sorted list of docIDs
    (title_map, link_map, id_list) = parse(collection_filename)
    print('parsed')
    # 2) create adjancy matrix A  as dictionary mapping row to list of nnz entries {i:[j for j in docIDs if i links to j]}
    A = create_adjacency_matrix(title_map, link_map)
    print('create_adjacency_matrix')

    # 3) create stochastic matrix P from A, as defined in section 12.2.1 of textbook with damping factor alpha=0.1
    P = create_stochastic_matrix(A, alpha, id_list) # alpha defined as global variable above
    print('create_stochastic_matrix')
    # create initial vector x with its only 1 in the first entry
    x = {j:0 for j in id_list}
    x[id_list[0]]=1
    print('now to compute pagerank')
    # now compute pagerank after so many vector-matrix-multiply iterations of x*P
    pagerank = compute_pagerank(x,P, id_list, 128)
    # print pagerank to file
    print_output(output_filename, pagerank, id_list)
    return #(A,P,pagerank)

# input:  1) filename of output file to write to
#         2) pagerank to represent on file
#         3) sorted list of docIDs (sorted in the order they were found in collection)
# prints to file such that the ith line of the file is the ith component of the pagerank -- should be in same order documents were found in collection
def print_output(output_filename, pagerank, id_list):
    f = open(output_filename, 'w')
    for i in id_list:
        f.write(str(pagerank[i])+'\n')
    f.close()

# input:  1) vector x:={pageID: value for pageID in collection}
#         2) P -- stochastic matrix as dictionary of (non-sparse) row dictionaries {i:{j: P_i_j where i,j pageIDs}}
#         3) list of pageIDs
#         4) number of iterations to compute x*P
# output: pagerank after <iterations> many x=x*P computations
def compute_pagerank(x,P,id_list,iterations):
    count = 0
    pagerank = x
    while count < iterations:
        new_pagerank = pagerank_P_multiply(pagerank, P, id_list)
        print('iteration: '+str(count)+', squared norm of difference: '+str(difference_normsq(pagerank, new_pagerank)))
        pagerank = new_pagerank
        count += 1
    return pagerank

# helper for testing -- finds the norm of the difference of two vectors
# useful because I want to see that this norm decreases as iterations of pagerank vector-matrix-multiply continue -- ie I want to see that result of pagerank is converging
def difference_normsq(vec1, vec2):
    return linalgebra.difference_normsq(vec1, vec2)

# returns vector-matrix-multiplication result x*P
def pagerank_P_multiply(vector, P, keys):
    result = {j:0 for j in keys}
    zero_entry = float(alpha)/len(keys)
    for i in keys:
        nnz_entry = P[i]['nnz_entry']
        nnz_array = P[i]['nnz_array']
        v_i = vector[i]
        for j in keys:
            if j in nnz_array:
                result[j] += v_i*nnz_entry
            else:
                result[j] += v_i*zero_entry
    print('result:')
    for j in keys:
        print('result['+str(j)+']='+str(result[j]))
    return result

# builds SPARSE stochastic matrix P from adjacency matrix as defined in section 12.2.1 of textbook
# input:  1) matrix A as dictionary mapping row to list of nnz entries {i:[j for j in docIDs if i links to j]}
#         2) damping factor alpha
#         3) sorted list of docIDs id_set
# output: P -- stochastic matrix as dictionary of sparse row dictionaries {i: {'nnz_entry':((1-alpha)/N + alpha/N), 'nnz_array':A[i]}} 
def create_stochastic_matrix(A, alpha, id_list):
    # establish variables used throughout procedure
    N = len(id_list) 
    beta = 1 - alpha
    epsilon = alpha/N
    P = {} # dictionary of rows
    # Augment A into P as follows:  
            # 1. If a row of A has no 1's, then replace each element by 1/N. For all other rows proceed as follows.
            # 2. Divide each 1 in A by the number of 1's in its row. Thus, if there is a row with three 1's, then each of them is replaced by 1/3.
            # 3. Multiply resulting matrix by 1-alpha
            # 4. Add alpha/N to every entry of the resulting matrix, to obtain P. 
    for i in id_list: 
        P[i] = {'nnz_entry':((beta/len(A[i])) + epsilon), 'nnz_array':A[i]}  
    return P 


# builds adjacency matrix A
# input:  1) title_map: {title: DocID}
#         2) link_map: {docID: set(link for link in document)}
# output: matrix A as dictionary mapping row to list of nnz entries {i:[j for j in docIDs if i links to j]}
def create_adjacency_matrix(title_map, link_map):
    A = {}
    for i in link_map:
        row_i = []
        doc_links = link_map[i]
        for link in doc_links:
            if link in title_map: # then this is an internal link -- lets put this (i,j) pair in our matrix
                j = title_map[link]
                row_i.append(j)
        A[i] = sorted(row_i)
    return A

# helper to parse
# input: 1) currline -- line to extract links from
#        2) link_set -- set of links to add found links to
# output: updated link_set
def extractLinks(currline, link_set):
    if ("[[" in currline and "]]" in currline):
        i = 0
        while i < (len(currline)-1):
            if currline[i] == '[' and currline[i+1] == '[':
                link_string = ''
                i += 2
                while i < (len(currline)-1):
                    if currline[i] == ']' and currline[i+1] == ']':
                        i += 2
                        link_string = (link_string.split('#')[0]).split('|')[0]
                        link_set.add(link_string)
                        break
                    else:
                        link_string += currline[i]
                        i += 1
            i += 1
    return link_set

#input: filename (fname) of the file collection
#output: tuple: (title_map, link_map, id_set):
#        title_map is dictionary {'title':docID}
#        link_map is dictionary {docID: set('link' for link in document)}
#        sorted list of the found docIDs
def parse(fname):
    f = open(fname)
    title_map = {} # initalize empty title_map
    link_map = {}  # initialize empty link_map
    id_list = []

    # loop through entire document by line
    currline = f.readline()
    while currline:

        link_set = set() # initalize new set of links that pageID maps to

        while (not "<id>" in currline and currline): currline = f.readline()
        # currLine now contains the pageID, so parse that as needed
        currline = currline.replace("<id>", "")
        currline = currline.replace("</id>\n", "")
        if not currline:
            break
        docID = int(currline)

        while (not "<title>" in currline and currline): currline = f.readline()
        # currLine now contains the title, so parse that as needed
        title = currline[7:len(currline)-9]

        while currline: # now extract links out of the document, line by line, inserting each into the link_set
            extractLinks(currline, link_set)
            if "</page>" in currline:  # then we're done extracting the links for this doc
                break
            currline = f.readline()
        # store our newly collected information
        if title in title_map:
            print("ERROR: REPEATED TITLE FOUND WHEN PARSING DOCUMENTS")
        title_map[title] = docID 
        link_map[docID] = link_set
        id_list.append(docID)
        
        currline = f.readline() # now we're done with this page, so lets keep parsing next page

    # finished parsing all pages -- now just close document return maps and sorted list of docIDs
    f.close()
    return (title_map, link_map, id_list)

main(sys.argv[1], sys.argv[2])
