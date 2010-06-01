#include "sqlitepp.h"
#include <cassert>
#include <sqlite3.h>

using namespace std;
using namespace boost;

namespace sqlite {

string encodeSQLSafe(const string &input){
	string output;
	for (string::const_iterator itr = input.begin(); itr != input.end(); ++ itr){
		output.push_back(*itr);
		if (*itr == '\'' || *itr == '\"' || *itr == '\\'){
			output.push_back(*itr);
		}
	}
	return output;
}

int initialize(){
	return sqlite3_initialize();
}

int shutdown(){
	return sqlite3_shutdown();
}

Connection::Connection(const string &fileName): db(NULL) {
	int rc = sqlite3_open(fileName.c_str(), &db);
	if (rc) throw Error() << ErrorInfo("Failed to open connection");
}

Connection::~Connection(){
	cached_statements.clear();
	sqlite3_close(db);
}

int Connection::getErrCode(){
	return sqlite3_errcode(db);
}

string Connection::getErrMsg(){
	const char *p = sqlite3_errmsg(db);
	return p ? p : "";
}

void Connection::setBusyTimeOut(int ms){
	sqlite3_busy_timeout(db, ms);
}

void Connection::execute(const std::string &sql){
	int rc = sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
	if (rc) throw Error() << ErrorInfo(getErrMsg());
}

long long Connection::getLastInsertRowId(){
	return sqlite3_last_insert_rowid(db);
}

int Connection::getChangeCount(){
	return sqlite3_changes(db);
}

PreparedStatementPtr Connection::prepare(const std::string &sql, bool cached){
	if (cached){
		map<string, PreparedStatementPtr>::iterator itr = cached_statements.find(sql);
		if (itr != cached_statements.end()){
			itr->second->reset(true);
			return itr->second;
		}
		else{
			PreparedStatementPtr statement(new PreparedStatement(*this, sql));
			cached_statements.insert(make_pair(sql, statement));
			return statement;
		}
	}
	else{
		PreparedStatementPtr statement(new PreparedStatement(*this, sql));
		return statement;
	}
}

void Connection::interrupt(){
	sqlite3_interrupt(db);
}

bool Connection::hasTable(const string &table_name) {
	try {
		PreparedStatementPtr stmt = prepare("SELECT COUNT(*) FROM " + table_name);
		if (stmt->step()) {
			return true;
		}
	}
	catch (sqlite::Error &) {
	}
	return false;
}

PreparedStatement::PreparedStatement(Connection &connection_, const string &sql_):
	connection(connection_),
	sql(sql_),
	state(UNSTARTED)
{
	int rc = sqlite3_prepare_v2(connection.raw(), sql.c_str(), sql.length(), &stmt, NULL);
	if (rc) throw Error() << ErrorInfo(connection.getErrMsg());
	column_count = sqlite3_column_count(stmt);
}

PreparedStatement::~PreparedStatement(){
	if (stmt){
		sqlite3_finalize(stmt);
	}
}

bool PreparedStatement::step(){
	assert(state != FINISHED);
	int rc = sqlite3_step(stmt);
	if (rc == SQLITE_DONE){
		state = FINISHED;
		return false;
	}
	else if (rc == SQLITE_ROW){
		state = STARTED;
		return true;
	}
	else{
		throw Error() << ErrorInfo(connection.getErrMsg());
	}
}

string PreparedStatement::getColumnName(int index){
	assert(index >= 0 && index < getColumnCount());
	const char* p = sqlite3_column_name(stmt, index);
	return p ? p : "";
}

ColumnType PreparedStatement::getColumnType(int index){
	assert(index >= 0 && index < getColumnCount());
	int type = sqlite3_column_type(stmt, index);
	switch (type){
	case SQLITE_INTEGER:
		return TYPE_INTEGER;
	case SQLITE_FLOAT:
		return TYPE_FLOAT;
	case SQLITE_TEXT:
		return TYPE_TEXT;
	case SQLITE_BLOB:
		return TYPE_BLOB;
	default:
		return TYPE_NULL;
	}
}

int PreparedStatement::getInt(int index){
	assert(state == STARTED);
	assert(index >= 0 && index < getColumnCount());
	return sqlite3_column_int(stmt, index);
}

long long PreparedStatement::getLong(int index){
	assert(state == STARTED);
	assert(index >= 0 && index < getColumnCount());
	return sqlite3_column_int64(stmt, index);
}

double PreparedStatement::getDouble(int index){
	assert(state == STARTED);
	assert(index >= 0 && index < getColumnCount());
	return sqlite3_column_double(stmt, index - 1);
}

string PreparedStatement::getString(int index){
	assert(state == STARTED);
	assert(index >= 0 && index < getColumnCount());
	const char* data = NULL;
	int size = 0;
	getString(index, data, size);
	if (size == 0){
		return "";
	}
	return string(data, size);
}

void PreparedStatement::getString(int index, const char* &data, int &size){
	assert(state == STARTED);
	assert(index >= 0 && index < getColumnCount());
	data = reinterpret_cast<const char*>(sqlite3_column_text(stmt, index));
	size = sqlite3_column_bytes(stmt, index);
}

void PreparedStatement::getBlob(int index, const void* &data, int &size){
	assert(state == STARTED);
	assert(index >= 0 && index < getColumnCount());
	data = reinterpret_cast<const char*>(sqlite3_column_blob(stmt, index));
	size = sqlite3_column_bytes(stmt, index);
}

void PreparedStatement::bind(int index, int data){
	int rc = sqlite3_bind_int(stmt, index, data);
	if (rc) throw Error() << ErrorInfo(connection.getErrMsg());
}

void PreparedStatement::bind(int index, long long data){
	int rc = sqlite3_bind_int64(stmt, index, data);
	if (rc) throw Error() << ErrorInfo(connection.getErrMsg());
}

void PreparedStatement::bind(int index, double data){
	int rc = sqlite3_bind_double(stmt, index, data);
	if (rc) throw Error() << ErrorInfo(connection.getErrMsg());
}

void PreparedStatement::bind(int index, const string &data, bool make_copy){
	bind(index, data.c_str(), data.length(), make_copy);
}

void PreparedStatement::bind(int index, const char *data, int size, bool make_copy){
	int rc = sqlite3_bind_text(stmt, index, data, size, make_copy ? SQLITE_TRANSIENT : SQLITE_STATIC);
	if (rc) throw Error() << ErrorInfo(connection.getErrMsg());
}

void PreparedStatement::bind(int index, const void *data, int size, bool make_copy){
	int rc = sqlite3_bind_blob(stmt, index, data, size, make_copy ? SQLITE_TRANSIENT : SQLITE_STATIC);
	if (rc) throw Error() << ErrorInfo(connection.getErrMsg());
}

void PreparedStatement::bindNULL(int index){
	int rc = sqlite3_bind_null(stmt, index);
	if (rc) throw Error() << ErrorInfo(connection.getErrMsg());
}

void PreparedStatement::clearBindings(){
	sqlite3_clear_bindings(stmt);
}

void PreparedStatement::reset(bool clear_bindings){
	sqlite3_reset(stmt);
	if (clear_bindings){
		clearBindings();
	}
	state = UNSTARTED;
}

} // namespace sqlite
