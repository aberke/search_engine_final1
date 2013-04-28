# k-means algorithm implementation
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




