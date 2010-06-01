#include "agglomerative_clustering.h"
#include <algorithm>
#include <list>

using namespace std;
using namespace boost;

namespace ucair {

double SimMatrix::get(int i, int j) const{
	if (i > j){
		swap(i, j);
	}
	map<pair<int, int>, double>::const_iterator itr = data.find(make_pair(i,j));
	if (itr != data.end()){
		return itr->second;
	}
	return 0.0;
}

void SimMatrix::set(int i, int j, double sim){
	if (i > j){
		swap(i, j);
	}
	data[make_pair(i,j)] = sim;
}

static bool compareTuple(const tuple<int, int, double> &a, const tuple<int, int, double> &b){
	return a.get<2>() < b.get<2>();
}

AgglomerativeClusteringMethod::AgglomerativeClusteringMethod(const vector<Cluster> &init_clusters_, SimMatrix &sim_matrix_, double stop_sim_threshold_) :
	sim_matrix(sim_matrix_),
	init_clusters(init_clusters_),
	next_cluster_id(1000000),
	stop_sim_threshold(stop_sim_threshold_) {
}

vector<Cluster> AgglomerativeClusteringMethod::run() {
	merge_steps.clear();
	for (int cluster_id = 0; cluster_id < (int) init_clusters.size(); ++ cluster_id){
		MergeStep step;
		step.cluster_id_1 = step.cluster_id_2 = -1;
		step.sim = 0.0;
		step.weight = init_clusters[cluster_id].points.size();
		step.active = true;
		merge_steps[cluster_id] = step;
	}

	heap.clear();
	heap.reserve(sim_matrix.data.size() + init_clusters.size() * init_clusters.size());
	for (map<pair<int, int>, double>::const_iterator itr = sim_matrix.data.begin(); itr != sim_matrix.data.end(); ++ itr){
		heap.push_back(make_tuple(itr->first.first, itr->first.second, itr->second));
	}
	make_heap(heap.begin(), heap.end(), compareTuple);
	heap_size = heap.size();

	while (heap_size > 0){
		pop_heap(heap.begin(), heap.begin() + heap_size, compareTuple);
		-- heap_size;
		int cluster_id_1 = heap[heap_size].get<0>();
		int cluster_id_2 = heap[heap_size].get<1>();
		double sim = heap[heap_size].get<2>();
		if (sim < stop_sim_threshold){
			break;
		}
		mergeCluster(cluster_id_1, cluster_id_2, sim);
	}

	vector<Cluster> clusters;
	for (map<int, MergeStep>::const_iterator itr = merge_steps.begin(); itr != merge_steps.end(); ++ itr){
		if (itr->second.active){
			clusters.push_back(Cluster());
			Cluster &cluster = clusters.back();
			list<int> queue;
			queue.push_back(itr->first);
			while (! queue.empty()){
				int cluster_id = queue.front();
				const MergeStep &step = merge_steps[cluster_id];
				if (step.cluster_id_1 != -1){
					queue.push_back(step.cluster_id_1);
					queue.push_back(step.cluster_id_2);
				}
				else{
					copy(init_clusters[cluster_id].points.begin(), init_clusters[cluster_id].points.end(), back_inserter(cluster.points));
				}
				queue.pop_front();
			}
		}
	}
	return clusters;
}

void AgglomerativeClusteringMethod::mergeCluster(int cluster_id_1, int cluster_id_2, double sim){
	MergeStep &step1 = merge_steps[cluster_id_1];
	MergeStep &step2 = merge_steps[cluster_id_2];
	if (! step1.active || ! step2.active){
		return;
	}
	step1.active = false;
	step2.active = false;
	double weight1 = step1.weight;
	double weight2 = step2.weight;
	int new_cluster_id = next_cluster_id ++;

	for (map<int, MergeStep>::const_iterator itr = merge_steps.begin(); itr != merge_steps.end(); ++ itr){
		if (! itr->second.active){
			continue;
		}
		int cluster_id = itr->first;
		double sim = (sim_matrix.get(cluster_id_1, cluster_id) * weight1 + sim_matrix.get(cluster_id_2, cluster_id) * weight2) / (weight1 + weight2);
		if (sim >= 0.01){
			sim_matrix.set(new_cluster_id, cluster_id, sim);
			heap[heap_size - 1] = make_tuple(new_cluster_id, cluster_id, sim);
			push_heap(heap.begin(), heap.begin() + heap_size, compareTuple);
		}
	}

	MergeStep step;
	step.cluster_id_1 = cluster_id_1;
	step.cluster_id_2 = cluster_id_2;
	step.sim = sim;
	step.weight = weight1 + weight2;
	step.active = true;
	merge_steps[new_cluster_id] = step;
}

} // namespace ucair
