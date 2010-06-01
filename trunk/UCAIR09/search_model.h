#ifndef __search_model_h__
#define __search_model_h__

#include <ctime>
#include <list>
#include <map>
#include <string>
#include <boost/smart_ptr.hpp>
#include "component.h"
#include "main.h"
#include "properties.h"
#include "user.h"

namespace ucair {

/// A language model for a search.
class SearchModel {
public:
	SearchModel(); // do not use
	SearchModel(const std::string &model_name, const std::string &model_description, bool adaptive = false);

	std::map<int, double> probs; ///< term probs in the language model
	util::Properties properties; ///< additional attributes

	/// Whether the search model is user-adaptive.
	bool isAdaptive() const { return adaptive; }

	/// Returns name of the search model.
	std::string getModelName() const { return model_name; }
	/// Returns description of the search model.
	std::string getModelDescription() const { return model_description; }
	/// Returns creation time of the search model.
	time_t getTimestamp() const { return timestamp; }

private:
	std::string model_name;
	std::string model_description;
	time_t timestamp;
	bool adaptive;
};

/// A search model generator.
class SearchModelGen {
public:
	SearchModelGen(const std::string &model_name, const std::string &model_description);
	virtual ~SearchModelGen() {}

	/// Generates a model for a user search.
	virtual SearchModel getModel(const UserSearchRecord &search_record, const Search &search) const = 0;
	/// Whether a given model for a user search is out-dated (when new information about the user search becomes available).
	virtual bool isOutdated(const UserSearchRecord &search_record, const SearchModel &search_model) const { return false; }

	/// Returns name of generated search models.
	std::string generateModelName() const { return model_name; }
	/// Returns description of generated search models.
	std::string generateModelDescription() const { return model_description; }

private:
	std::string model_name;
	std::string model_description;
};

/// Generates search models using MLE from only query text.
class QueryMLEModelGen: public SearchModelGen {
public:
	QueryMLEModelGen(const std::string &model_name, const std::string &model_description) : SearchModelGen(model_name, model_description) {}

	SearchModel getModel(const UserSearchRecord &search_record, const Search &search) const;
};

/// Generates search models by assigning different weights to query terms, clicked result terms, unclicked result terms.
class WeightedClickModel: public SearchModelGen {
public:
	WeightedClickModel(const std::string &model_name,
			const std::string &model_description,
			double query_term_weight,
			double clicked_result_term_weight,
			double unclicked_result_term_weight);

	bool isOutdated(const UserSearchRecord &search_record, const SearchModel &search_model) const;

protected:
	bool isAdaptive(const UserSearchRecord &search_record) const;
	void countTermsWeighted(const UserSearchRecord &search_record, const Search &search, std::map<int, double> &term_counts) const;

	double query_term_weight; ///< weight on query terms
	double clicked_result_term_weight; ///< weight on terms in clicked results
	double unclicked_result_term_weight;  ///< weight on terms in unclicked results
};

/// Generates search models using Rocchio feedback.
class RocchioModelGen: public WeightedClickModel {
public:
	RocchioModelGen(const std::string &model_name,
			const std::string &model_description,
			double query_term_weight,
			double clicked_result_term_weight,
			double unclicked_result_term_weight);

	SearchModel getModel(const UserSearchRecord &search_record, const Search &search) const;
};

/// Generates search models using mixture model based feedback.
class MixtureModelGen: public WeightedClickModel {
public:
	MixtureModelGen(const std::string &model_name,
			const std::string &model_description,
			double query_term_weight,
			double clicked_result_term_weight,
			double unclicked_result_term_weight,
			double bg_coeff);

	SearchModel getModel(const UserSearchRecord &search_record, const Search &search) const;

private:
	double bg_coeff;
};

/// Manages different search models.
class SearchModelManager : public Component {
public:
	/// Registers a search model generator.
	void addModelGen(const boost::shared_ptr<SearchModelGen> &model);
	/// Finds search model generator by name.
	SearchModelGen* getModelGen(const std::string &model_name) const;
	/// Returns all search model names.
	std::list<std::string> getAllModelGens() const;
	/// Generates a search model by specifying user, search and model name.
	const SearchModel& getModel(const UserSearchRecord &search_record, const Search &search, const std::string &model_name);

	bool initialize();

private:
	/// map from search model name to search model generators
	std::map<std::string, boost::shared_ptr<SearchModelGen> > model_gens;
	/// map from pair of (search id, search model name) to cached search models
	std::map<std::pair<std::string, std::string>, SearchModel> cached_models;
};

DECLARE_GET_COMPONENT(SearchModelManager)

} // namespace ucair

#endif
