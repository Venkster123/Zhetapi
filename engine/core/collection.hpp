#ifndef COLLECTION_H_
#define COLLECTION_H_

// C++ headers
#include <vector>

// Engine headers
#include <token.hpp>

namespace zhetapi {

class collection : public Token {
	std::vector <Token *>	_arr;
public:
	collection();
	collection(const std::vector <Token *> &);

	void reset();
	Token *next() const;
};

#endif
