#ifndef GLOBAL_H_
#define GLOBAL_H_

// C/C++ headers
#include <iostream>
#include <vector>
#include <iterator>

// Engine headers
#include "../engine/all/zhplib.hpp"
#include "../engine/core/algorithm.hpp"
#include "../engine/lang/parser.hpp"
#include "../engine/lang/error_handling.hpp"

// Namespaces
using namespace std;
using namespace zhetapi;

extern bool verbose;
extern size_t line;
extern string file;
extern vector <string> global;
extern vector <string> idirs;

extern Engine *engine;

int parse(char = EOF);
int parse(string);

int compile_library(vector <string>, string);

int assess_libraries(vector <string>);

int import_library(string);

Token *execute(string);

vector <string> split(string);

// Include builtin
#include "builtin/basic_io.hpp"

#endif
