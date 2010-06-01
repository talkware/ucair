#include "mixture.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <boost/foreach.hpp>
#include <iostream>

using namespace std;
using namespace boost;

namespace ucair {

void estimateMixture(std::vector<boost::tuple<double, double, double> > &values, const double alpha){
	assert(alpha > 0.0 && alpha < 1.0);
	const double beta = 1.0 - alpha;
	vector<pair<double, int> > quotients;
	quotients.reserve(values.size());
	for (int i = 0; i < (int) values.size(); ++ i){
		tuple<double, double, double> &value = values[i];
		assert(get_f(value) > 0.0 && get_p(value) >= 0.0);
		quotients.push_back(make_pair(get_p(value) / get_f(value), i));
	}
	sort(quotients.begin(), quotients.end());
	double f_sum = 0.0;
	double p_sum = 0.0;
	int k = 0;
	for (; k < (int) values.size(); ++ k){
		tuple<double, double, double> &value = values[quotients[k].second];
		f_sum += get_f(value);
		p_sum += get_p(value);
		if (beta / alpha + p_sum <= f_sum * quotients[k].first){
			f_sum -= get_f(value);
			p_sum -= get_p(value);
			break;
		}
	}
	double lambda = f_sum / (1.0 + alpha / beta * p_sum);
	for (int i = 0; i < k; ++ i){
		tuple<double, double, double> &value = values[quotients[i].second];
		get_q(value) = get_f(value) / lambda - alpha / beta * get_p(value);
	}
	for (int i = k; i < (int) values.size(); ++ i){
		tuple<double, double, double> &value = values[quotients[i].second];
		get_q(value) = 0.0;
	}
}

void estimateMixtureWeights(const vector<double> &target, const vector<vector<double> > &components, vector<double> &weights, int max_tries, int max_iterations) {
	assert(! target.empty() && ! components.empty() && ! weights.empty());
	assert(target.size() == components[0].size() && components.size() == weights.size());

	double LL = 0.0, last_LL = 0.0, best_LL = 0.0;
	vector<double> best_weights(weights.size());
	vector<double> priors = weights;

	vector<vector<double> > z;
	z.resize(target.size());
	BOOST_FOREACH(vector<double> &zi, z) {
		zi.resize(components.size());
	}

	for (int try_count = 1; try_count <= max_tries; ++ try_count) {
		double sum = 0.0;
		BOOST_FOREACH(double &weight, weights) {
			weight = max_tries > 1 ? (rand() + 1.0) : 1.0;
			sum += weight;
		}
		BOOST_FOREACH(double &weight, weights) {
			weight /= sum;
		}

		BOOST_FOREACH(vector<double> &zi, z) {
			fill(zi.begin(), zi.end(), 0.0);
		}

		for (int iteration = 1; iteration <= max_iterations; ++ iteration) {
			for (int i = 0; i < (int) target.size(); ++ i) {
				sum = 0.0;
				for (int j = 0; j < (int) components.size(); ++ j) {
					z[i][j] = components[j][i] * weights[j];
					sum += z[i][j];
				}
				if (sum > 0.0) {
					for (int j = 0; j < (int) components.size(); ++ j) {
						z[i][j] /= sum;
					}
				}
			}

			sum = 0.0;
			for (int j = 0; j < (int) components.size(); ++ j) {
				weights[j] = priors[j];
				for (int i = 0; i < (int) target.size(); ++ i) {
					weights[j] += z[i][j] * target[i];
				}
				sum += weights[j];
			}
			if (sum > 0.0) {
				for (int j = 0; j < (int) components.size(); ++ j) {
					weights[j] /= sum;
				}
			}

			LL = 0.0;
			for (int i = 0; i < (int) target.size(); ++ i) {
				sum = 0.0;
				for (int j = 0; j < (int) components.size(); ++ j) {
					sum += components[j][i] * weights[j];
				}
				LL += log(sum) * target[i];
			}

			if (iteration > 1) {
				if (abs(LL - last_LL) < abs(LL + last_LL) * 1e-6) {
					break;
				}
			}
			last_LL = LL;
		}

		if (best_LL == 0.0 || LL > best_LL) {
			best_LL = LL;
			best_weights = weights;
		}
	}

	weights = best_weights;
}

} // namespace ucair
