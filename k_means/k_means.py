# k-means algorithm implementation
import sys
from vecrep import main as vecrep, compute_norm, normalize

k = 11 # algorithm to be implemented with 11 clusters
e = 0.025 # don't know what to expect for now
threshhold = 1e-3 # suppression threshhold -- these values might as well be zeros and we like sparcity
# I'm zero-indexing my clusters



"""
Input : Number of clusters k, a set of data points X, and e>=0
Output: Mi clusters and their means.

	initialize u_1,....u_k;
	d_i <- ||u_i|| for 1<=i<=k
	M_i = {} for 1<=i<=k
	while d_i>=e for some i:
		for x in X:
			j = argmin||u_j - x||
			M_j.add(x)
		for i in range(k):
			u_i' = mean of points in M_i
		for i in range(k):
			d_i = ||u_i' - u_i||
		for i in range(k):
			u_i = u_i'
	return M_i's and u_i's
"""
# input: 1) <collection_filename> filename of collection of pages to represent as vectors
#		 2) <input_filename> -- training file to read from where each line has a pageID to assign to a cluster
# 		 3) <clusterKM_filename> -- file to write to 
#		 4) <features_filename> -- filename of features to read from -- pages will be represented as vectors in this feature space
# output: writes to <clusterKM_filename> in format: ith line of file: <ith pageID of training file> <id of cluster ith pageID assigned to>
def main(collection_filename, input_filename, clusterKM_filename, features_filename):
	# obtain pages as vectors and F:= len(features_dict), ie gives range to iterate over for feature-keys
	X, F = vecrep(collection_filename, features_filename)
	print('created X')
	# create initial cluster means u_i for 0<=i<k
	u_dict = initialize_means1(X)
	print('created u_dict')
	# compute initial max_delta as argmax ||u_i||
	max_delta = 1
	# initialize empty M clusters of form M_dict:= {i:set(pageID for x in cluster i) for i in range(k)} as empty dictionary at first
	M_dict = {}
	# run algorithm until max_delta < target e (ie, algorithm stabilized enough)
	i = 0 #&&&&&&& take out
	while max_delta >= e:
		print('iteration: '+str(i))
		# let last M_dict get garbage collected
		M_dict = initialize_clusters()
		# at each iteration, recomputes max_delta, M_dict, u_dict
		(max_delta, M_dict, u_dict) = recluster(u_dict, M_dict, X)
		print('iteration: '+str(i)+', max_delta: '+str(max_delta))
		i += 1
	# compute inverse of M, ie, dictionary mapping {pageID: cluster_id}
	M_inverse = compute_M_inverse(M_dict)
	# print results to file in same order of pageIDs in input_filename
	print_clusters(M_inverse, input_filename, clusterKM_filename)
	print('done')
	return

# iterative part of the k-means algorithm that recomputes max_delta, M_dict, u_dict
# input: 1) u_dict:= {i: u_i} where u_i's cluster means as feature vectors:= {f_i: float value for f_i in features}
#		 2) empty M clusters to be filled in with form M_dict:= {i:set(pageID for x in cluster i) for i in range(k)}
#		 3) X:= pages as feature vectors dictionary {pageID: feature-vector}
# output: tuple (max_delta, M_dict, u_dict) 
def recluster(u_dict, M_dict, X):
	# for each x in X, find j = argmin||u_j - x|| and add x to M_j
	for pageID in X:
		if pageID % 100 == 0:
			print('on pageID: '+str(pageID))
		j = best_cluster(u_dict, X[pageID]) 
		M_dict[j].add(pageID)
	# compute new u_i's as mean of points in M_i and along the way compute max delta
	max_delta = 0
	for i in range(k):
		u_i_new = compute_mean(X, M_dict[i])
		delta = compute_norm(compute_diff(u_dict[i], u_i_new))
		if delta > max_delta:
			max_delta = delta
		u_dict[i] = u_i_new

	return (max_delta, M_dict, u_dict)

# method: pick u_0 randomly.  Pick u_i+1 such that number of features that exclusively appear in u_i XOR u_i+1 is maximized
# input: dictionary of document vectors X:= {pageID: feature_vector} where feature_vector := {f_i:float value for f_i in features} is normalized (euclidean norm)
# output: dictionary of initial means u_dict:= {i: u_i} where u_i is a feature-vector from X
def initialize_means1(X):
	u_dict = {}
	# start with set of all pageIDs
	pageID_set = set(pageID for pageID in X)
	# first mean is random element from set
	mean = X[pageID_set.pop()]
	u_dict[0] = mean
	i = 1

	while i < k:
		# find x in X with max number of features that differ between x and mean
		next_candidate = -1 # pageID of candidate next mean
		max_f_missing = -1
		for pageID in pageID_set:
			x = X[pageID]
			f_missing = 0
			for f_i in mean:
				if not f_i in x:
					f_missing += 1
			if  f_missing > max_f_missing:
				max_f_missing = f_missing
				next_candidate = pageID
		mean = X[next_candidate]
		pageID_set.remove(next_candidate)
		u_dict[i] = mean
		i += 1

	return u_dict

# method: pick u_0 randomly.  Pick u_i+1 that doesn't contain feature with max-value from u_i
# input: dictionary of document vectors X:= {pageID: feature_vector} where feature_vector := {f_i:float value for f_i in features} is normalized (euclidean norm)
# output: dictionary of initial means u_dict:= {i: u_i} where u_i is a feature-vector from X
def initialize_means2(X):
	u_dict = {}
	# start with set of all pageIDs
	pageID_set = set(pageID for pageID in X)
	# first mean is random element from set
	mean = X[pageID_set.pop()]
	u_dict[0] = mean
	i = 1

	while i<k:
		# find max-value of last mean:
		max_f = -1
		max_value = -1
		for (f_i, v) in mean.items():
			if v > max_value:
				max_value = v
				max_f = f_i
		# now take first vector that doesn't have f_i as a feature
		for pageID in pageID_set:
			x = X[pageID]
			if not max_f in x:
				u_dict[i] = x
				pageID_set.remove(pageID)
				mean = x
				break
		i += 1
	return u_dict


# input: 1) M_inverse:= {pageID: cluster_id}
#		 2) <input_filename> -- training file to read from where each line has a pageID to assign to a cluster
#		 3) <clusterKM_filename> -- file to write to 
# output: writes to <clusterKM_filename> in format: ith line of file: <ith pageID of training file> <id of cluster ith pageID assigned to>
def print_clusters(M_inverse, input_filename, clusterKM_filename):
	f_clusterKM = open(clusterKM_filename, 'w')
	f_input = open(input_filename, 'r')
	line = f_input.readline()
	while line:
		pageID = int(line.split()[0])
		if pageID in M_inverse:
			f_clusterKM.write(str(pageID)+' '+str(M_inverse[pageID])+'\n')
		else:
			f_clusterKM.write(str(pageID)+'\n')
		line = f_input.readline()
	f_input.close()
	f_clusterKM.close()
	return

# input:  M_dict:= {i:set(pageID for x in cluster i) for i in range(k)} as empty dictionary at first
# output: M_inverse:= {pageID: cluster_id}
def compute_M_inverse(M_dict):
	M_inverse = {}
	for (i, pageID_set) in M_dict.items():
		for pageID in pageID_set:
			M_inverse[pageID] = i
	return M_inverse

# input: u_dict of form {i:u_i for i in range(k)}
# output: max_d := argmax ||u_i|| for u_i in u_dict
def initialize_max_delta(u_dict):
	return compute_max_delta(u_dict, {})

# input: 2 u_dict's of form {i:u_i for i in range(k)} where u_dict2 is optionally missing some (or all) i's
# output: max_d := argmax ||u_i2-u_i1|| for u_i's in u_dict's
def compute_max_delta(u_dict1, u_dict2):
	max_delta = 0
	for i in range(k):
		diff_i = u_dict1[i]
		if i in u_dict2:
			diff_i = compute_diff(u_dict1[i], u_dict2[i])
		norm_i = compute_norm(diff_i)
		if norm_i > max_delta:
			max_delta = norm_i
	return max_delta		

# initialize empty M clusters of form M_dict:= {i:set(pageID for x in cluster i) for i in range(k)}
def initialize_clusters():
	return {i:set() for i in range(k)}

# input: 2 vectors
# output diff:= vec2-vec1
def compute_diff(vec1, vec2):
	diff = {}
	for f_i in {k for k in vec1.keys()}.union({k for k in vec2.keys()}):
		if f_i in vec1:
			v1 = vec1[f_i]
		else:
			v1 = 0
		if f_i in vec2:
			v2 = vec2[f_i]
		else:
			v2 = 0
		diff[f_i] = v2-v1
	return diff

# input:  1) vec1
#		  2) vec2
# output: ||u_j - x||**2 <-- minimizing sq of norm effectively minimizes norm
def compute_normsq_diff(vec1, vec2):
	normsq = 0
	for f_i in set(k for k in vec1.keys()).union(k for k in vec2.keys()):
		v1 = 0
		v2 = 0
		if f_i in vec1:
			v1 = vec1[f_i]
		if f_i in vec2:
			v2 = vec2[f_i]
		normsq += (v2-v1)**2
	return normsq

# input:  1) u_dict of cluster means u_i's
#		  2) x:= vector to find best cluster for
# output: j = argmin||u_j - x||**2 <-- minimizing sq of norm effectively minimizes norm
def best_cluster(u_dict, x):
	best_cluster = -1
	best_diff_normsq = -1
	for i in u_dict:
		diff_normsq = compute_normsq_diff(u_dict[i], x)
		if (diff_normsq < best_diff_normsq) or (best_diff_normsq < 0):
			best_diff_normsq = diff_normsq
			best_cluster = i
	return best_cluster

# input:  1) X -- dictionary of page-vectors of form {pageID: feature-vector for pageID in collection}
#		  2) pageID_set:= set of keys in X of form set(pageID,...)
# output: mean vector (1/|pageID_set|)sum(X[pageID] for pageID in pageID_set)
def compute_mean(X, pageID_set):
	denominator = len(pageID_set)
	mean = {}
	for pageID in pageID_set:
		vector = X[pageID]
		for f_i in vector:
			if not f_i in mean:
				mean[f_i] = 0
			mean[f_i] += vector[f_i]
	# return vector only with values above threshhold to maintain sparcity
	result = {}
	for f_i,v in mean.items():
		value = float(v)/denominator
		if value > threshhold:
			result[f_i] = value
	# must normalize it again:
	result = normalize(result)
	return result



main(sys.argv[1], sys.argv[2],sys.argv[3], sys.argv[4])