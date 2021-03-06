#ifndef ALGORITHM_H_
#define ALGORITHM_H_

// C++ headers
#include <set>
#include <string>
#include <vector>

// Engine headers
#include "common.hpp"
#include "functor.hpp"
#include "node_manager.hpp"

namespace zhetapi {

class Engine;
class node_manager;

// Algorithm class
class algorithm : public Functor {
	std::string			_ident		= "";
	std::string			_alg		= "";

	std::set <std::string>		_pardon;
	Args				_args		= {};

	node_manager			_compiled	= node_manager();
	
	// void generate(Engine *, std::string str, node_manager &);
public:
	algorithm();
	algorithm(const algorithm &);
	algorithm(const std::string &,
		const std::string &,
		const std::vector <std::string> &);
	algorithm(const std::string &,
		const std::string &,
		const std::vector <std::string> &,
		const node_manager &);
	
	void compile(Engine *);	
	
	Token *evaluate(Engine *, const std::vector <Token *> &) override;

	// Put somewhere else
	// std::vector <std::string> split(std::string str);
	
	const std::string &symbol() const;

	// Debugging
	void print() const;

	bool empty() const;

	// Virtual functions
	type caller() const override;
	Token *copy() const override;
	std::string dbg_str() const override;

	virtual bool operator==(Token *) const override;
};

}

#endif
