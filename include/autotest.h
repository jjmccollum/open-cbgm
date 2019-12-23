/*
 * autotest.h
 *
 *  Created on: Dec 23, 2019
 *      Author: jjmccollum
 */

#ifndef AUTOTEST_H
#define AUTOTEST_H

#include <string>
#include <list>
#include <map>

using namespace std;

struct unit_test {
	string name;
	string msg;
	bool passed;
};

struct module_test {
	string name;
	list<unit_test> units;
};

struct library_test {
	string name;
	list<module_test> modules;
};

class autotest {
private:
	string target_module;
	string target_test;
	list<string> modules;
	map<string, list<string>> tests_by_module;
	map<string, string> parent_module_by_test;
	library_test lib_test;
public:
	autotest();
	autotest(const list<string> & _modules, const map<string, list<string>> & _tests_by_module);
	~autotest();
	void print_modules() const;
	void print_tests() const;
	bool set_target_module(const string & _target_module);
	bool set_target_test(const string & _target_test);
	void run();
	const library_test get_results() const;
};

#endif /* AUTOTEST_H */
