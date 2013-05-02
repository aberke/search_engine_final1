# Create labelSVM.dat given a value of epsilon and the 11 prediction sets
# Usage:
#   python combine_labels.py 0.1 prediction0.dat prediction1.dat... labelSVM.dat

import sys

def main(epsilon, prediction0, prediction1, prediction2, prediction3, prediction4, prediction5, prediction6, prediction7, prediction8, prediction9, prediction10, output_filename):
	labels = []
	epsilon = float(epsilon)
	
	predictions = [prediction0, prediction1, prediction2, prediction3, prediction4, prediction5, prediction6, prediction7, prediction8, prediction9, prediction10]
	
	for i in range(len(predictions)):
		f = open(predictions[i], 'r')
		
		j = 0
		for line in f:
			if j >= len(labels):
				labels.append([])
			
			score = float(line)
			if score > epsilon:
				labels[j].append(str(i))
			
			j += 1
		
		f.close()
	
	output = open(output_filename, 'w')
	for line in labels:
		output.write(' '.join(line) + '\n')
	output.close()

main(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5], sys.argv[6], sys.argv[7], sys.argv[8], sys.argv[9], sys.argv[10], sys.argv[11], sys.argv[12], sys.argv[13])
