#vecrep.py file
# file 1 for classification project
import sys
from vecrep_util import parse, tokenize, create_stopwords_set, create_features_dict

import searchio  # import our own optimized I/O module

			
# helper to main: after the index is created, must print it to the output_filename file
# input: filename of vecRep output file (output_filename)
#		 index to represent on disk (index)
#		 number of feature labels (F) -- since dictionary doesn't necessarily iterate through indecies in monotonically increasing order like we want
# index is built in form {pageID: (sum_d, {f_i: occ_i for f_i in pageID-vector})}
# write out index in format (referenced in handout section 1.1.1:  pageID sum_d f_i:occ_i ........
# 		--> print one line for each pageID
def printVecrep(output_filename, index, F):
	# open up output file for writing
	f = open(output_filename, 'w')
	for pageID in range(len(index)): 
		(sum_d, feature_vector) = index[pageID]
		pageString = str(pageID)+' '+str(sum_d)
		for f_i in range(F):
			if f_i in feature_vector: # must check since each feature_vector is sparse!
				occ_i = feature_vector[f_i]
				pageString += ' '+str(f_i)+':'+str(occ_i)
		f.write(pageString+'\n')
	f.close()
	return

# input: <stopWords filename>, <pagesCollection filename>, <features filename>, <vecRep output filename to be built>
# output: file vecRep with an entry line for each document in pagesCollection, where each line in form pageID sum_d f_i:occ_i ........
def main(stopwords_filename, pagesCollection_filename, features_filename, output_filename):


	# obtain the stopwords in a set for quick checking
	stopWords_set = create_stopwords_set(stopwords_filename)
	# obtain features in a dictionary {feature: f_i for feature in features_filename} to allow both quick checking and mapping feature to its index
	features_dict = create_features_dict(features_filename)
	# initialize empty index with structure {docID: (sum_d, {f_i:occ_i for feature in features})}
	index = {}

	# obtain dictionary mapping pageID's to list of title and text words, ie collection = {pageID: textString}
	(collection, maxID) = parse(pagesCollection_filename)

	# iterate over keys (pageID's) to fill the index
	for i in range(maxID+1):
		if not i in collection:
			print(str(i)+' not in collection!!')
			#continue
			return ####

		pageID = i
		textString = collection[i]
		feature_vector = {}
		
		# tokenize titleString
		token_list = searchio.tokenize(stopWords_set, textString, False)
		
		# map feature to feature_occurance in index
		for t in range(len(token_list)):
			token = token_list[t]

			if token in features_dict:
				f_i = features_dict[token] # token is a feature, so get feature index of that feature
				if not f_i in feature_vector:
					feature_vector[f_i] = 0
				feature_vector[f_i] += 1

		# now have map from f_i to occ_i, but must sum squares of occ_i's to get sum_d
		sum_d = 0
		for f_i in feature_vector:
			occ_i = feature_vector[f_i]
			sum_d += f_i**2

		# put entry for pageID in index
		index[pageID] = (sum_d, feature_vector)

	# now the index is built in form {docID: (sum_d, {f_i:occ_i for feature in features})} -- must print to file in form 'pageID sum_d f_i:occ_i ........'
	printVecrep(output_filename, index, len(features_dict))
	return index
				
main(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4])