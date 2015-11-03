# Introduction #

This tutorial shows how to add support for (explicit) relevance feedback to UCAIR. Relevance feedback allows user to explicitly specify whether a document is relevant or not, and uses this information to help rerank the unjudged documents.

Here are some screenshots of the finished relevance feedback functionality. You can notice that we allow users to give positive/negative feedbacks and they can be used to generate a feedback language model to rerank the search results accordingly.

![http://ucair.googlecode.com/svn/wiki/rel_fb_demo_1.png](http://ucair.googlecode.com/svn/wiki/rel_fb_demo_1.png)

![http://ucair.googlecode.com/svn/wiki/rel_fb_demo_2.png](http://ucair.googlecode.com/svn/wiki/rel_fb_demo_2.png)

# Step 1: UI for relevance judgment #

In this step we'll change the HTML to display clickable links that can be used to provide relevance judgment. There is a "Y" link to indicate positive relevance, a "N" link to indicate negative relevance, and an initially hidden "C" link to cancel previous made relevance judgment:

[result\_list\_view.htm](http://code.google.com/p/ucair/source/diff?spec=svn22&r=22&format=side&path=/trunk/templates/result_list_view.htm)
```
<span class="search_result_rating" style="display:none">${rating}</span>
<span class="search_result_rating_y">Y</span>
<span class="search_result_rating_n">N</span>
<span class="search_result_rating_c">C</span>
<span class="search_result_rating_url" style="display:none">/rate?sid=${search_id}&amp;pos=${result_pos}&amp;view=${view_id}&amp;rating=</span>
```

I use a hidden `span.search_result_rating` element to hold the current rating, and a hidden `span.search_result_rating_url` element to hold the url for rating submission.

Note the use of template variables such as ${search\_id} and ${result\_pos}. These are replaced by actual values when the HTML pages are rendered. I have written [a custom C++ based template engine](http://code.google.com/p/ucair/wiki/CPPTemplateEngine) that provides somewhat comparable functionality as JSP/PHP.

Following is associated CSS:

[main.css](http://code.google.com/p/ucair/source/diff?spec=svn22&r=22&format=side&path=/trunk/static_files/main.css)
```
.search_result_rating_y, .search_result_rating_n, .search_result_rating_c{
        color: #7777CC;
        margin-left: 5px;
        margin-right: 5px;
        padding-left: 2px;
        padding-right: 2px;
        text-decoration: underline;
        cursor: pointer;
}

.search_result_rating_y.rated {
        color: #007700;
        background-color: #F0F0F0;
        font-weight: bold;
        text-decoration: none;
        cursor: auto;
}

.search_result_rating_n.rated {
        color: #770000;
        background-color: #F0F0F0;
        font-weight: bold;
        text-decoration: none;
        cursor: auto;
}
```

We change the colors when they are assigned a `rated` class.

Finally there is the Javascript (I use [JQuery](http://jquery.com/)):

[main.js](http://code.google.com/p/ucair/source/diff?spec=svn22&r=22&format=side&path=/trunk/static_files/main.js)
```
function prepareRatings() {
        $(".search_result_rating").each(function() {
                var ratingElem = $(this);
                showRating(ratingElem, false);
                var y = ratingElem.nextAll(".search_result_rating_y");
                var n = ratingElem.nextAll(".search_result_rating_n");
                var c = ratingElem.nextAll(".search_result_rating_c");
                y.click(function() {
                        ratingElem.text("Y");
                        showRating(ratingElem, true);
                });
                n.click(function() {
                        ratingElem.text("N");
                        showRating(ratingElem, true);
                });
                c.click(function() {
                        ratingElem.text("");
                        showRating(ratingElem, true);
                });
        });
}

function showRating(ratingElem, sendRating) {
        var y = ratingElem.nextAll(".search_result_rating_y");
        var n = ratingElem.nextAll(".search_result_rating_n");
        var c = ratingElem.nextAll(".search_result_rating_c");
        var val = ratingElem.text();
        if (val == "Y") {
                y.addClass("rated").show();
                n.removeClass("rated").hide();
                c.show();
        }
        else if (val == "N") {
                y.removeClass("rated").hide();
                n.addClass("rated").show();
                c.show();
        }
        else {
                y.removeClass("rated").show();
                n.removeClass("rated").show();
                c.hide();
        }
        if (sendRating) {
                var url = ratingElem.nextAll(".search_result_rating_url").text();
                $.get(url + encodeURI(val));
        }
}
```

When "Y" is clicked, the following happens:
  1. rating inside the ".search\_result\_rating" element is set to "Y"
  1. "Y" is highlighted by setting the `rated' class
  1. "N" becomes hidden
  1. "C" becomes visible
  1. We send an AJAX request to the server (`$.get`) using the url `/rate?sid=${search_id}&amp;pos=${result_pos}&amp;view=${view_id}&amp;rating=Y`

Similar things happen when "N" is clicked.

`prepareRatings` is also called during page load: [basic\_search.htm](http://code.google.com/p/ucair/source/diff?spec=svn22&r=22&format=side&path=/trunk/templates/basic_search.htm)

# Step 2: Record the relevance judgment at server side #

Whenever we receive an AJAX request indicating a document is relevant or not, we need to store this relevance judgment for later use.

First, we register a handler for the AJAX request:
[basic\_search\_ui.cpp](http://code.google.com/p/ucair/source/diff?spec=svn22&r=22&format=side&path=/trunk/UCAIR09/basic_search_ui.cpp)
```
getUCAIRServer().registerHandler(RequestHandler::CGI_OTHER, "/rate", bind(&BasicSearchUI::rateResult, this, _1, _2));
```

The handler type is `CGI_OTHER`. We use `CGI_HTML` for CGI handlers that dynamically generate HTML pages, and `CGI_OTHER` for other CGI handlers that do not generate HTML pages. Both types of handlers parse params from URL and POST data.

The handler will invoke `BasicSearchUI::rateResult` whenever the request URL starts with `/rate`. The `Request` object is passed as the 1st arg, and the `Reply` object is passed as the 2nd arg.

Now let's look at the handler itself:

[basic\_search\_ui.cpp](http://code.google.com/p/ucair/source/diff?spec=svn22&r=22&format=side&path=/trunk/UCAIR09/basic_search_ui.cpp)
```
void BasicSearchUI::rateResult(Request &request, Reply &reply){
        User *user = getUserManager().getUser(request, true);
        assert(user);

        int result_pos = 0;
        try{
                string s = request.getFormData("pos");
                if (! s.empty()){
                        result_pos = lexical_cast<int>(s);
                }
        }
        catch (bad_lexical_cast &){
                getUCAIRServer().err(reply_status::bad_request);
        }
        string rating = request.getFormData("rating");

        string search_id = request.getFormData("sid");
        string view_id = request.getFormData("view");
        const SearchResult *result = getSearchProxy().getResult(search_id, result_pos);
        if (result){
                // Adds an event of user explicitly rating the result.
                shared_ptr<RateResultEvent> event(new RateResultEvent);
                event->search_id = search_id;
                event->result_pos = result_pos;
                event->rating = rating;
                user->addEvent(event);

                UserSearchRecord *search_record = user->getSearchRecord(search_id);
                if (search_record){
                        search_record->setResultRating(result_pos, rating);
                        // Mark all previous search results as viewed.
                        const string property_name = "ranking_" + view_id;
                        if (search_record->properties.has(property_name)){
                                vector<int> &ranking = search_record->properties.get<vector<int> >(property_name);
                                BOOST_FOREACH(const int &pos, ranking){
                                        search_record->addViewedResult(pos);
                                        if (pos == result_pos){
                                                break;
                                        }
                                }
                        }
                }
        }


        // Empty reply
        reply.setHeader("Content-Type", "text/plain");
}
```

First we get the `User` object from request. After the user logs in, each request carries a cookie on user id, so we can easily look up which user the request is coming from. The `User` object stores everything about a user, from their preferences to past search history.

Then we get the params `pos`, `rating`, `sid`, `view` from the request. They are parsed from the URL `/rate?sid=${search_id}&amp;pos=${result_pos}&amp;view=${view_id}&amp;rating=Y` and can be retrieved using `Request.getFormData()`

`search_id` uniquely identifies a search instance, and `result_pos` is the original rank of the search result, so from these two we can get the `SearchResult` object, facilitated by the global `SearchProxy` object, which stores all search results indexed by search id and result pos. The `SearchResult` object contains information about a particular result document, including title, summary snippet, url, etc.

The `UserSearchRecord` object stores all user activities on a particular search. Most importantly, it has all the user events, including this `RateResultEvent`, which is a subclass of `UserEvent`. These events can be persisted to database and reconstructed (discussed later).

We call `UserSearchRecord::setResultRating()` to store the relevance judgment. The judgments are part of `UserSearchRecord`:

[user\_search\_record.h](http://code.google.com/p/ucair/source/diff?spec=svn22&r=22&format=side&path=/trunk/UCAIR09/user_search_record.h)
```
public:
        /*! Marks a result as explicitly rated.
         *  If rating is empty, remove the rating.
         */
        void setResultRating(int pos, const std::string &rating);
        /*! Returns the rating of the result at a certain position.
         *  Returns an empty string if result is not rated.
         */
        std::string getResultRating(int pos) const;

        /// Whether a rating string represents positive feedback.
        static bool isRatingPositive(const std::string &rating);
private:
        std::map<int, std::string> rated_results;
```

[user\_search\_record.cpp](http://code.google.com/p/ucair/source/diff?spec=svn22&r=22&format=side&path=/trunk/UCAIR09/user_search_record.cpp)
```
void UserSearchRecord::setResultRating(int pos, const string &rating) {
        if (rating.empty()) {
                rated_results.erase(pos);
        }
        else {
                rated_results[pos] = rating;
        }
}

string UserSearchRecord::getResultRating(int pos) const {
        map<int, string>::const_iterator itr = rated_results.find(pos);
        if (itr != rated_results.end()) {
                return itr->second;
        }
        return "";
}

bool UserSearchRecord::isRatingPositive(const string &rating) {
        if (rating == "Y" || rating == "y") {
                return true;
        }
        return false;
}
```

Finally, we mark all search results before the judged search result as being "viewed". This is done by calling `UserSearchRecord::addViewedResult()`

# Step 3: Persist relevance judgments #

We need to persist the user's relevance judgments to database, so that they can be recovered when we need to do any offline analysis, or can be used to built a long-term user model.

We have a user\_events table that stores all the user event attributes, and we only need a way to serialize custom attributes into the `value` field:
```
CREATE TABLE user_events(
search_id TEXT NOT NULL,
timestamp INTEGER NOT NULL,
type TEXT NOT NULL,
value TEXT NOT NULL)
```

Following is the definition of the `RateResultEvent` class:

[user\_event.h](http://code.google.com/p/ucair/source/diff?spec=svn22&r=22&format=side&path=/trunk/UCAIR09/user_event.h)
```
/// An event of user explicitly rating a result.
class RateResultEvent: public UserEvent {
public:
        RateResultEvent();

        IMPLEMENT_CLONE(RateResultEvent)

        int result_pos; ///< rated result pos
        std::string rating; ///< explicit rating

        static const std::string type;
        std::string getType() const { return type; }
        std::string saveValue() const;
        void loadValue (const std::string &value);
};
```

[user\_event.cpp](http://code.google.com/p/ucair/source/diff?spec=svn22&r=22&format=side&path=/trunk/UCAIR09/user_event.cpp)
```
RateResultEvent::RateResultEvent() :
        result_pos(0) {
}

const string RateResultEvent::type("rate result");

string RateResultEvent::saveValue() const {
        return str(format("%d\t%s") % result_pos % rating);
}

void RateResultEvent::loadValue (const string &value) {
        size_t pos = value.find('\t');
        try {
                result_pos = lexical_cast<int>(value.substr(0, pos));
        }
        catch (bad_lexical_cast &) {
                throw Error() << ErrorMsg("Failed to load RateResultEvent from string");
        }
        rating = value.substr(pos + 1);
}
```

Note how the `loadValue` and `saveValue` methods serialize event attributes.


The detailed logic for persisting user events is inside [long\_term\_history\_manager.cpp](http://code.google.com/p/ucair/source/diff?spec=svn22&r=22&format=side&path=/trunk/UCAIR09/long_term_history_manager.cpp). In particular, when it's load user events, it set the search result rating if the event is of type `RateResultEvent`.

```
shared_ptr<RateResultEvent> rate_result_event = dynamic_pointer_cast<RateResultEvent>(event);
if (rate_result_event) {
       search_record.setResultRating(rate_result_event->result_pos, rate_result_event->rating);
       break;
}
```

# Step 4: Construct the relevance feedback model #

UCAIR focuses on language models, so it stores the term-probability mapping inside a `SearchModel` object:

[search\_model.h](http://code.google.com/p/ucair/source/browse/trunk/UCAIR09/search_model.h)
```
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
```

There is a `SearchModelGen` class that controls how to generate a `SearchModel`, and different language-model based retrieval methods should provide different implementations of `SearchModelGen`

[search\_model.h](http://code.google.com/p/ucair/source/browse/trunk/UCAIR09/search_model.h)
```
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
```

In our case we provide a subclass `RelevanceFeedbackModelGen` that implements relevance feedback:

[search\_model.h](http://code.google.com/p/ucair/source/diff?spec=svn22&r=22&format=side&path=/trunk/UCAIR09/search_model.h)
```
/// Generates search models for explicit relevance feedback using mixture model.
class RelevanceFeedbackModelGen: public SearchModelGen {
public:
        RelevanceFeedbackModelGen(const std::string &model_name,
                        const std::string &model_description,
                        double bg_coeff);

        bool isOutdated(const UserSearchRecord &search_record, const SearchModel &search_model) const;

        SearchModel getModel(const UserSearchRecord &search_record, const Search &search) const;

protected:
        bool isAdaptive(const UserSearchRecord &search_record) const;

private:
        double bg_coeff;
};
```

[search\_model.cpp](http://code.google.com/p/ucair/source/diff?spec=svn22&r=22&format=side&path=/trunk/UCAIR09/search_model.cpp)
```
RelevanceFeedbackModelGen::RelevanceFeedbackModelGen(const string &model_name, const string &model_description, double bg_coeff_) :
        SearchModelGen(model_name, model_description), bg_coeff(bg_coeff_) {
}

bool RelevanceFeedbackModelGen::isOutdated(const UserSearchRecord &search_record, const SearchModel &search_model) const {
        BOOST_FOREACH(const shared_ptr<UserEvent> &event, search_record.getEvents()) {
                if (event->timestamp <= search_model.getTimestamp()) {
                        continue;
                }
                if (typeid(*event) == typeid(RateResultEvent)) {
                        return true;
                }
        }
        return false;
}

bool RelevanceFeedbackModelGen::isAdaptive(const UserSearchRecord &search_record) const {
        return ! search_record.getRatedResults().empty();
}

SearchModel RelevanceFeedbackModelGen::getModel(const UserSearchRecord &search_record, const Search &search) const {
        SearchModel model(generateModelName(), generateModelDescription(), isAdaptive(search_record));

        map<int, double> term_counts;

        indexing::SimpleIndex *index = search_record.getIndex();
        if (index) {
                const map<int, string> &ratings = search_record.getRatedResults();

                typedef pair<int, string> P1;
                BOOST_FOREACH(const P1 &p1, ratings) {
                        int result_pos = p1.first;
                        string rating = p1.second;
                        if (UserSearchRecord::isRatingPositive(rating)) {
                                string doc_name = buildDocName(search_record.getSearchId(), result_pos);
                                int doc_id = index->getDocDict().getId(doc_name);
                                if (doc_id > 0) {
                                        const vector<pair<int, float> >* term_list = index->getTermList(doc_id);
                                        if (term_list) {
                                                typedef pair<int, float> P2;
                                                BOOST_FOREACH(const P2 &p2, *term_list) {
                                                        int term_id = p2.first;
                                                        double term_weight = p2.second;
                                                        if (term_weight > 0.0) {
                                                                map<int, double>::iterator itr;
                                                                tie(itr, tuples::ignore) = term_counts.insert(make_pair(term_id, 0.0));
                                                                itr->second += term_weight;
                                                        }
                                                }
                                        }
                                }
                        }
                }
        }


        vector<tuple<double, double, double> > values;
        for (map<int, double>::const_iterator itr = term_counts.begin(); itr != term_counts.end(); ++ itr) {
                double f = itr->second;
                double p = getIndexManager().getColProb(itr->first);
                values.push_back(make_tuple(f, p, 0.0));
        }
        estimateMixture(values, bg_coeff);
        int i = 0;
        for (map<int, double>::const_iterator itr = term_counts.begin(); itr != term_counts.end(); ++ itr, ++ i) {
                double q = values[i].get<2>();
                if (q > 0.0) {
                        model.probs.insert(make_pair(itr->first, q));
                }
        }
        truncate(*indexing::ValueMap::from(model.probs), 20, 0.001);
        return model;
}
```

As `RelevanceFeedbackModelGen` is a subclass of `SearchModelGen`, it needs to implement a few methods. First, we allow search models to be lazily evaluated, i.e., they are only generated when necessary. This is controlled by `isOutdated`, where we indicate the model needs to be regenerated only when there are new user ratings.

The `isAdaptive` method indicates whether the search model is adaptive. For relevance feedback, it is obvious the search model is adaptive only after user has provided feedback.

The actual code to generate a search model is within the `getModel` method. We use the language-modelling based feedback algorithm from [this paper of C. Zhai](http://portal.acm.org/citation.cfm?id=502585.502654).

We first concatenate all the search results with positive ratings into a pseudo document. Each `UserSearchRecord` maintains an index of all the retrieved search results. The index is of type `SimpleIndex`, which allows us to look all the term ids associated with a document id. `SimpleIndex` is a very simple custom-built inverted index that resides in memory (without a disk backup, so never persisted). It supports the indexing of language models and sufficient for the small amount of search log data that needs to be indexed.

The term counts in the pseudo documents are stored in a `map<int, double>` structure that maps term ids to term counts.

The model-based feedback algorithm assumes the pseudo-document is a mixture of a feedback language model and a background language model, and the goal is to estimate the feedback model that gives maximum likelihood for generating the document. In the original paper it is done by EM, but here we use the faster algorithm given in [Y. Zhang's paper](http://portal.acm.org/citation.cfm?id=1277948). For details please see the implementation of `estimateMixture()`. Note negative relevance feedback is still not supported. Interested readers can try implementing the algorithm in [X Wang's paper](http://portal.acm.org/citation.cfm?id=1390374).

Finally we truncate the model to 20 terms max.

UCAIR allows you to choose a search model from a drop down box, and then click the "Rerank" button to see the effects of the relevance feedback model. So we don't need to do anything more here. That's it.