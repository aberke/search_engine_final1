# Create a training set for use with SVM
# Usage (e.g., for category 4):
#   python create_training_set.py vecrep.dat training.dat 4 svmtraining4.dat

import sys, math

# Given a vector representation of our data, recreate a native Python version
def recreate_vecrep(vecrep_filename, normalized_bool):
	# open file for reading and instantiate vecrep dictionary
	f = open(vecrep_filename, 'r')
	vecrep = {}

	lineString = f.readline()
	while lineString:
		lineList =  lineString.split()
		pageID = int(lineList[0])
		sum_d = int(lineList[1])
		feature_vector = {}
		# fill in feature-vector
		for i in range(2, len(lineList)):
			(f_i, occ_i) = lineList[i].split(':')
			if normalized_bool:
				feature_vector[int(f_i)] = float(occ_i)/math.sqrt(sum_d)
			else:
				feature_vector[int(f_i)] = int(occ_i)
		vecrep[pageID] = feature_vector

		lineString = f.readline()
	f.close()
	return vecrep

# Scan the training file, and write out a new SVM training file according to the given category
def export_training_data(vecrep, training_filename, category, output_filename):
	output = open(output_filename, 'w')
	training = open(training_filename, 'r')
	
	for line in training:
		(pageID, c) = line.split()
		pageID = int(pageID)
		c = int(c)
		
		if pageID in vecrep:
			feature_vector = vecrep[pageID]
			designator = "+1" if category == c else "-1"
			pageString = designator
			for feature, val in sorted(feature_vector.items()):
				pageString += ' '+str(feature + 1)+':'+str(val)
			output.write(pageString + '\n')
	
	training.close()
	output.close()

def main(vecrep_filename, training_filename, category, output_filename):
	vecrep = recreate_vecrep(vecrep_filename, True)
	export_training_data(vecrep, training_filename, int(category), output_filename)

main(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4])
