# Create the test set for use with svm_classify
# Usage:
#   python create_test_set.py vecrep.dat test.dat

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

# Write out a new SVM classifying file with the given vector representation
def export_test_data(vecrep, output_filename):
	output = open(output_filename, 'w')
	
	for pageID in vecrep:
		feature_vector = vecrep[pageID]
		pageString = "0"
		for feature, val in sorted(feature_vector.items()):
			pageString += ' '+str(feature + 1)+':'+str(val)
		output.write(pageString + '\n')
	
	output.close()

def main(vecrep_filename, output_filename):
	vecrep = recreate_vecrep(vecrep_filename, True)
	export_test_data(vecrep, output_filename)

main(sys.argv[1], sys.argv[2])
