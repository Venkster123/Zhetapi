#include "global.hpp"

#define __lineup(c) 						\
	if (c == '\n')						\
		line++;

// Global scope code
vector <string> global;

static int parse_parenthesized(string &parenthesized)
{
	char c;

	while (isspace(c = getchar())) {
		__lineup(c);
	}

	if (c != '(')
		return -1;

	while ((c = getchar()) != ')')
		parenthesized += c;

	return 0;
}

static int parse_block()
{
	char c;
	
	while (isspace(c = getchar())) {
		__lineup(c);
	}

	if (c == '{') {
		parse('}');
	} else {
		fseek(stdin, -1, SEEK_CUR);

		parse('\n');

		__lineup(c);
	}

	return 0;
}

static int parse_block_ignore()
{
	char c;
	
	while (isspace(c = getchar())) {
		__lineup(c);
	}

	if (c == '{') {
		while ((c = getchar()) != '}');
	} else {
		fseek(stdin, -1, SEEK_CUR);

		while ((c = getchar()) != '\n');

		__lineup(c);
	}

	return 0;
}

static void check(string &keyword)
{
	string parenthesized;
	string block;

	if (keyword == "if") {
		// cout << "IF" << endl;
		
		if (parse_parenthesized(parenthesized)) {
			printf("Syntax error at line %lu: missing parenthesis after an if\n", line);
			exit(-1);
		} else {
			// cout << "\tparen = \"" << parenthesized << "\"" << endl;
		}

		// cout << "evaluated (paren):" << endl;

		Token *t = execute(parenthesized);

		// cout << "\ttoken: " << t->str() << endl;

		if (*t == op_true) {
			// cout << "\tequals true!" << endl;
			parse_block();
		} else {
			parse_block_ignore();
		}
		
		keyword.clear();
	}

	/* if (keyword == "for") {
		cout << "FOR" << endl;

		if (parse_parenthesized(parenthesized)) {
			printf("Syntax error at line %lu: missing parenthesis after a for\n", line);
			exit(-1);
		} else {
			cout << "\tparen = \"" << parenthesized << "\"" << endl;
		}
		
		if (parse_block(block)) {
			printf("Syntax error at line %lu: missing statement after a for\n", line);
			exit(-1);
		} else {
			cout << "\tblock = \"" << block << "\"" << endl;
		}
		
		keyword.clear();
	}

	if (keyword == "while") {
		cout << "WHILE" << endl;

		if (parse_parenthesized(parenthesized)) {
			printf("Syntax error at line %lu: missing parenthesis after a while\n", line);
			exit(-1);
		} else {
			cout << "\tparen = \"" << parenthesized << "\"" << endl;
		}
		
		if (parse_block(block)) {
			printf("Syntax error at line %lu: missing statement after a while\n", line);
			exit(-1);
		} else {
			cout << "\tblock = \"" << block << "\"" << endl;
		}
		
		keyword.clear();
	} */
}

// Parsing machine
int parse(char ex)
{
	static bool quoted = false;
	static bool paren = false;

	string tmp;
	char c;

	while ((c = getchar()) != ex) {
		if (!quoted) {
			if (c == '\"')
				quoted = true;
			if (c == '(')
				paren = true;
			if (c == ')' && paren == true)
				paren = false;
			
			if (c == '\n' || (!paren && c == ',')) {
				if (!tmp.empty()) {
					execute(tmp);

					tmp.clear();
				}
			} else if (!isspace(c)) {
				tmp += c;
			}
		} else {
			if (c == '\"')
				quoted = false;
			
			tmp += c;
		}

		__lineup(c);

		// cout << "tmp = " << tmp << endl;
		check(tmp);
	}

	// Flush last instruction
	if (!tmp.empty()) {
		execute(tmp);

		tmp.clear();
	}

	return 0;
}

// Splitting equalities
vector <string> split(string str)
{
	bool quoted = false;

	vector <string> out;
	size_t n;

	n = str.length();

	string tmp;
	for (size_t i = 0; i < n; i++) {
		if (!quoted) {
			if (str[i] == '=') {
				if (!tmp.empty()) {
					out.push_back(tmp);

					tmp.clear();
				}
			} else {
				if (str[i] == '\"')
					quoted = true;
				
				tmp += str[i];
			}
		} else {
			if (str[i] == '\"')
				quoted = false;
			
			tmp += str[i];
		}
	}

	if (!tmp.empty())
		out.push_back(tmp);

	return out;
}
