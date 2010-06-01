#ifndef __agglomerative_clustering_h__
#define __agglomerative_clustering_h__

#include <map>
#include <vector>
#include <boost/tuple/tuple.hpp>

namespace ucair {

class SimMatrix{
public:
	double get(int i, int j) const;
	void set(int i, int j, double sim);
	std::map<std::pair<int, int>, double> data;
};

class Cluster{
public:
	std::vector<int> points;
};

/// Implements a basic agglomerative clustering method using avg link.
class AgglomerativeClusteringMethod {
public:
	/*! Constructor.
	 *
	 *  \param initClusters initial clusters (one point per cluster)
	 *  \param simMatrix similarity matrix between the points
	 *  \param stop_sim_threshold stop when avg similarity drops below this threshold
	 *  \return
	 */
	AgglomerativeClusteringMethod(const std::vector<Cluster> &initClusters, SimMatrix &simMatrix, double stop_sim_threshold);

	/*! Runs the algorithm.
	 *
	 *  \return result clusters
	 */
	std::vector<Cluster> run();

private:
	void mergeCluster(int cluster_id_1, int cluster_id_2, double sim);

	SimMatrix &sim_matrix;
	const std::vector<Cluster> &init_clusters;
	int next_cluster_id;

	std::vector< boost::tuple<int, int, double> > heap;
	size_t heap_size;
	double stop_sim_threshold;

	class MergeStep{
	public:
		bool active;
		int cluster_id_1;
		int cluster_id_2;
		double sim;
		double weight;
	};

	std::map<int, MergeStep> merge_steps;
};


} // namespace ucair

#endif
