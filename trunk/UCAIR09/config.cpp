#include "config.h"

using namespace std;

namespace util {

void getParam(const map<string, string> &params, const string &name, string &value){
	map<string, string>::const_iterator itr = params.find(name);
	if (itr == params.end()){
		throw runtime_error("Missing value for param " + name);
	}
	value = itr->second;
}

void readParams(istream &in, map<string, string> &params){
	string line;
	while (getline(in, line)){
		if (line.empty() || line[0] == '#'){
			continue;
		}
		unsigned pos0 = line.find('=');
		if (pos0 != string::npos){
			string name, value;
			unsigned pos1 = line.find_first_not_of(" \t\r");
			unsigned pos2 = line.find_last_not_of(" \t\r", pos0 - 1);
			if (pos1 <= pos2 && pos2 != string::npos){
				name = line.substr(pos1, pos2 - pos1 + 1);
			}
			pos1 = line.find_first_not_of(" \t\r", pos0 + 1);
			pos2 = line.find_last_not_of(" \t\r");
			if (pos1 <= pos2 && pos1 != string::npos){
				value = line.substr(pos1, pos2 - pos1 + 1);
			}
			params[name] = value;
		}
	}
}

} // namespace util
