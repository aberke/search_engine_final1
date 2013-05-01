#vecrep.py file
# taken from classification project (augmented) -- does work of turning documents into vectors in features space
import searchio  # import our own optimized I/O module
from math import sqrt


# input: filename (fname) of features file
# output: dictionary {feature: featureIndex for feature in features}, ie a dictionary mapping feature to its index
def create_features_dict(fname):
    f = open(fname, 'r')
    features_dict = {}
    w = f.readline()
    line = 0 # feature_index = line feature was read off of

    while w != '': # read to EOF
        if w[len(w)-1] == '\n':
            w = w[:len(w)-1] # strip off '\n'
        features_dict[w] = line      

        w = f.readline()
        line += 1
    f.close()
    return features_dict

def replace(textString, keepStar):
    t = ''
    for ch in textString:
        if ch.isalnum() or ((ch=='*') and keepStar):
            t += ch
        else:
            t += ' '
    return t


#input: filename (fname) of the file collection
#output: tuple: (dictionary, int):
#        dictionary with key: pageID, value: textString
#        maximum pageID found
def parse(fname):
    f = open(fname)
    pageID = ""
    dictionary = {} # dictionary initially empty
    maxID = 0

    # loop through entire document by line
    currLine = f.readline()
    while currLine:

        while (not "<id>" in currLine and currLine): currLine = f.readline()
        # currLine now contains the pageID, so parse that as needed
        currLine = currLine.replace("<id>", "")
        currLine = currLine.replace("</id>\n", "")
        if not currLine:
            break
        pageID = int(currLine)

        while (not "<title>" in currLine and currLine): currLine = f.readline()
        # currLine now contains the title, so parse that as needed
        titleString = currLine[7:len(currLine)-9]+'\n'
        textString = titleString 

        while (not "<text>" in currLine and currLine): currLine = f.readline()
        # now we've reached the <text> section, so iterate through lines until reaching the end at </text>
        currLine = currLine.replace("<text>", "")
        while (not "</text>" in currLine and currLine):
            textString = textString + currLine
            currLine = f.readline()

        #now we know we've reached the last line of </text>
        currLine = currLine.replace("</text>", "")
        textString = textString + currLine

        dictionary[pageID] = textString
        if pageID > maxID:
            maxID = pageID
        currLine = f.readline()

    return (dictionary, maxID)


# computes norm of feature vector -- helper to normalize and to k-means algorithm
def compute_norm(feature_vector):
	sum_d = 0
	for f_i in feature_vector:
		occ_i = feature_vector[f_i]
		sum_d += occ_i**2
	# divide each entry of unnormalized vector by the norm
	return sqrt(sum_d)

# helper to main -- normalizes feature vector
def normalize(feature_vector):
	norm = compute_norm(feature_vector)
	for f_i in feature_vector:
		feature_vector[f_i] = float(feature_vector[f_i])/norm
	return feature_vector

# main function:
# input: <pagesCollection filename>, <features filename>
# output: (X, F)
#			X: dictionary of document vectors X:= {pageID: feature_vector} where feature_vector := {f_i:float value for f_i in features} is normalized (euclidean norm)
#			F: len(features_dict)
def main(pagesCollection_filename, features_filename):
	# obtain features in a dictionary {feature: f_i for feature in features_filename} to allow both quick checking and mapping feature to its index
	features_dict = create_features_dict(features_filename)
	# initialize empty index, X, with structure {docID: {f_i:occ_i for feature in features}}
	X = {}

	# obtain dictionary mapping pageID's to list of title and text words, ie collection = {pageID: textString}
	(collection, maxID) = parse(pagesCollection_filename)

	# iterate over keys (pageID's) to fill the index
	for i in range(maxID+1):
		if not i in collection:
			continue

		pageID = i
		textString = collection[i]
		feature_vector = {}
		
		# tokenize textString
		token_list = searchio.tokenize(set(), textString, False) # tokenize wants to take stopwards set as first argument, but don't care about stopwords here
		
		# map feature to feature_occurance in index
		for t in range(len(token_list)):
			token = token_list[t]

			if token in features_dict:
				f_i = features_dict[token] # token is a feature, so get feature index of that feature
				if not f_i in feature_vector:
					feature_vector[f_i] = 0
				feature_vector[f_i] += 1

		# normalize feature-vector and insert into index X
		X[pageID] = normalize(feature_vector)
	return (X, len(features_dict))
				