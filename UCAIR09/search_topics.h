#ifndef __search_topics_h__
#define __search_topics_h__

#include <map>
#include <string>
#include <list>
#include "component.h"
#include "main.h"
#include "properties.h"
#include "sqlitepp.h"

namespace ucair {

/// A search topic is a set of related searches representing a user search interest.
class UserSearchTopic {
public:
	UserSearchTopic(int topic_id_ = 0) : topic_id(topic_id_), trivial(false), total_click_count(0) {}

	/// Returns all possible ways to sort different topics.
	static std::list<std::string> getAllSortingCriteria();
	/// Generates a score which can be used to sort the topics.
	double getSortingScore(const std::string &criteria);

	int topic_id; ///< topic id
	bool trivial; ///< whether the topic is trivial, or informative about user search interests
	std::map<int, double> model; ///< topic language model
	util::Properties properties; ///< additional properties

	/// map from search id to the search's weight in the topic
	std::map<std::string, double> searches;
	/// map from session id to number of searches having this session id in the topic
	std::map<std::string, int> sessions;
	/// map from query text to number of searches having this query text in the topic
	std::map<std::string, int> queries;
	/// map from urls to number of clicks having this url in the topic
	std::map<std::string, int> clicks;
	/// total number of clicks in the topic
	int total_click_count;
};

/// Manages user search topics.
class UserSearchTopicManager : public Component {
public:
	bool initialize();
	bool initializeUser(User &user);

	/*! \brief Returns the existing search topics of a user.
	 *
	 *  These are the topics from last time they were computed, so they may be outdated.
	 */
	std::map<int, UserSearchTopic>* getAllSearchTopics(const std::string &user_id);

	/// Run the clustering algorithm to update the search topics of a user.
	void updateSearchTopics(const std::string &user_id);

private:

	/// Runs agglomerative clustering and computes the topic models.
	void agglomerativeCluster(User &user, std::map<int, UserSearchTopic> &topics);
	/// Computes the session, query, click statistics.
	void computeProperties(User &user, UserSearchTopic &topic) const;
	/// Whether a topic is considered trivial.
	void testTrivial(User &user, UserSearchTopic &topic) const;

	/// Creates table structure for storing topics in database.
	static void sqlCreateTables(sqlite::Connection &conn);
	/// Loads topics from database.
	void loadSearchTopics(User &user, sqlite::Connection &conn);
	/// Saves topics to database.
	void saveSearchTopics(User &user, sqlite::Connection &conn);

	std::map<std::string, std::map<int, UserSearchTopic> > all_user_search_topics;

	double agglomerative_clustering_stop_sim; ///< stop joining two clusters when similarity drops below this threshold
	bool join_sessions; ///< whether to join searches in the same session before running agglomerative clustering
	bool join_same_queries; ///< whether to join searches having same query text before running agglomerative clustering
	int nontrivial_topic_session_count; ///< minimum number of sessions for a topic to be considered nontrivial
};

DECLARE_GET_COMPONENT(UserSearchTopicManager);

} // namespace ucair

#endif
