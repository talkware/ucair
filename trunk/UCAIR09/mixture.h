#ifndef __mixture_h__
#define __mixture_h__

#include <vector>
#include <boost/tuple/tuple.hpp>

namespace ucair {

inline double& get_f(boost::tuple<double, double, double> &t) { return t.get<0>(); }
inline double& get_p(boost::tuple<double, double, double> &t) { return t.get<1>(); }
inline double& get_q(boost::tuple<double, double, double> &t) { return t.get<2>(); }

/*! \brief Computes the maximum likelihood estimate of an unknown component in a two-component mixture language model.
 *
 *  We want to maximize \sum f_i log(\alpha * p_i + (1 - \alpha * q_i)
 *  f, p, alpha is known, while q is unknown.
 *  See the paper by Yi Zhang, Wei Xu: Fast Exact Maximum Likelihood Estimation for Mixture of Language Models
 *  \param[in,out] values a vector of (f_i, p_i, q_i). q_i will be filled after the call.
 *  \param[in] alpha weight on p
 */
void estimateMixture(std::vector<boost::tuple<double, double, double> > &values, const double alpha);

/*! \brief Computes the maximum likelihood estimate of the weights in a mixture model when the components are fixed using the EM algorithm.
 *
 * \param target[in] the target mixture model
 * \param components[in] the mixture components
 * \param weights[out] the mixing weights to estimate
 * \param max_tries[in] max number of tries (each try with different random start)
 * \param max_iterations[in] max number of EM iterations in each try
 */
void estimateMixtureWeights(const std::vector<double> &target, const std::vector<std::vector<double> > &components, std::vector<double> &weights, int max_tries = 1, int max_iterations = 100);

} // namespace ucair

#endif
