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

struct unit_test {
	std::string name;
	std::string msg;
	bool passed;
};

struct module_test {
	std::string name;
	std::list<unit_test> units;
};

struct library_test {
	std::string name;
	std::list<module_test> modules;
};

class autotest {
private:
	std::string target_module;
	std::string target_test;
	std::list<std::string> modules;
	std::map<std::string, std::list<std::string>> tests_by_module;
	std::map<std::string, std::string> parent_module_by_test;
	library_test lib_test;
public:
	autotest();
	autotest(const std::list<std::string> & _modules, const std::map<std::string, std::list<std::string>> & _tests_by_module);
	~autotest();
	void print_modules() const;
	void print_tests() const;
	bool set_target_module(const std::string & _target_module);
	bool set_target_test(const std::string & _target_test);
	void run();
	const library_test get_results() const;
};

#endif /* AUTOTEST_H */
