#ifndef __sqlitepp_h__
#define __sqlitepp_h__

#include <string>
#include <map>
#include <boost/exception/all.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/utility.hpp>

class sqlite3;
class sqlite3_stmt;

namespace sqlite{

std::string encodeSQLSafe(const std::string &str);

/// Initialize SQLite
int initialize();

/// Shut down SQLite
int shutdown();

typedef boost::error_info<struct tag_sqlite_error, std::string> ErrorInfo;

/// SQLite error
class Error: public boost::exception, public std::exception {
public:
	const char* what() const throw() { return "SQLite3 error"; }
};

class PreparedStatement;

typedef boost::shared_ptr<PreparedStatement> PreparedStatementPtr;

/// SQLite connection
class Connection: private boost::noncopyable {
public:

	/// Constructs a database connection from a SQLite file.
	Connection(const std::string &fileName);
	~Connection();

	/// Returns the error code from last error.
	int getErrCode();

	/// Returns the error message from last error.
	std::string getErrMsg();

	/// Sets how long it times out for a busy operation.
	void setBusyTimeOut(int ms);

	/// Interrupt current operations.
	void interrupt();

	/*! \brief Executes a SQL.
	 *
	 *	\throw sqlite::Error if there is an error
	 */
	void execute(const std::string &sql);

	/// Returns the last inserted row id.
	long long getLastInsertRowId();

	/// Returns how many rows have been changed.
	int getChangeCount();

	/*! \brief Prepares a SQL statement.
	 *
	 * 	\param sql sql statement to prepare
	 *  \param cached If true, cache the prepared statement so that future calls are quicker.
	 *	\return pointer to the prepared statement
	 */
	PreparedStatementPtr prepare(const std::string &sql, bool cached = false);

	/// Begins a transaction.
	void beginTransaction() { execute("BEGIN TRANSACTION"); }

	/// Commits a transaction.
	void commit() { execute("COMMIT"); }

	/// Rolls back a transaction.
	void rollback() { execute("ROLLBACK"); }

	/// Returns the raw sqlite3 pointer.
	sqlite3* raw() { return db; }

	/// Whether a non-empty table with a given name exists.
	bool hasTable(const std::string &table_name);

private:

	std::map<std::string, PreparedStatementPtr> cached_statements;
	sqlite3 *db;
};

enum ColumnType { TYPE_INTEGER, TYPE_FLOAT, TYPE_TEXT, TYPE_BLOB, TYPE_NULL };

class PreparedStatement: private boost::noncopyable{
public:

	/*! Constructs a prepared statement.
	 *
	 * 	\param db database connection
	 * 	\param sql SQL statement
	 */
	PreparedStatement(Connection &db, const std::string &sql);
	~PreparedStatement();

	/*! Binds a position with 32-bit integer.
	 *
	 * 	\param index starts at 1
	 * 	\param data 32-bit integer
	 */
	void bind(int index, int data);

	/*! Binds a position with 64-bit integer.
	 *
	 * 	\param index starts at 1
	 * 	\param data 64-bit integer
	 */
	void bind(int index, long long data);

	/*! Binds a position with double.
	 *
	 *  \param index starts at 1
	 *  \param data double
	 */
	void bind(int index, double data);

	/*! Binds a position with string.
	 *
	 *  \param index starts at 1
	 *  \param data string
	 *  \param make_copy If false, then data obj must live longer than the prepared statement.
	 */
	void bind(int index, const std::string &data, bool make_copy = true);

	/*! Binds a position with c-style string.
	 *
	 *  \param index starts at 1
	 *  \param data string pointer
	 *  \param size string length
	 *  \param make_copy If false, then data obj must live longer than the prepared statement.
	 */
	void bind(int index, const char* data, int size, bool make_copy = true);

	/*! Binds a position with binary data.
	 *
	 *  \param index starts at 1
	 *  \param data binary data pointer
	 *  \param size binary data length
	 *  \param make_copy If false, then data obj must live longer than the prepared statement.
	 */
	void bind(int index, const void *data, int size, bool make_copy = true);

	/*! Binds a position with NULL.
	 *
	 *  \param index starts at 1
	 */
	void bindNULL(int index);

	/// Clear all bindings.
	void clearBindings();

	/// Returns the number of columns.
	int getColumnCount() const { return column_count; }

	/// Returns the column name at a position (starts at 0).
	std::string getColumnName(int index);

	/// Returns the column type at a position (starts at 0).
	ColumnType getColumnType(int index);

	/// Returns the 32-bit integer at a position (starts at 0).
	int getInt(int index);

	/// Returns the 64-bit integer at a position (starts at 0).
	long long getLong(int index);

	/// Returns the double at a position (starts at 0)
	double getDouble(int index);

	/// Returns the string at a position (starts at 0)
	std::string getString(int index);

	/*! Returns a c-style string at a position.
	 *
	 * \param[in] index starts at 0
	 * \param[out] data string pointer
	 * \param[out] size string length
	 */
	void getString(int index, const char *&data, int &size);
	void getBlob(int index, const void *&data, int &size);

	/*! \brief Moves one step in the execution of the prepared statement.
	 *
	 *  \return true if there are more data
	 */
	bool step();

	/*! \brief Resets the execution of the prepared statement.
	 *
	 *  \param clear_bindings If true, also call clearBindings()
	 */
	void reset(bool clear_bindings = true);

	/// Returns the associated connection object.
	Connection& getConnection() { return connection; }

	/// Returns the raw sqlite pointer
	sqlite3_stmt* raw() { return stmt; }

	/// Returns the SQL string.
	const std::string& getSQL() { return sql; }

private:

	Connection &connection;
	const std::string sql;
	sqlite3_stmt *stmt;
	int column_count;
	enum { UNSTARTED, STARTED, FINISHED } state;
};

}; // namespace sqlite

#endif
