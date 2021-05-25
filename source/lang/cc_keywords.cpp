#include <lang/parser.hpp>

namespace zhetapi {

static node_manager cc_if(Feeder *feeder,
		Engine *context,
		const Args &args,
		Pardon &pardon,
		State *state)
{
	// Return
	node_manager nif;

	char c;
	while ((c = feeder->feed()) != '(');

	std::string paren = feeder->extract_parenthesized();

	// Skip construction step or something
	node_manager ncond(context, paren, args, pardon);

	// TODO: Add a skip whitespace
	while (isspace(c = feeder->feed()));
	if (c != '{')
		feeder->backup(1);
	
	feeder->set_end((c == '{') ? '}' : '\n');

	node_manager nblock = cc_parse(feeder, context,
			args, pardon);

	nif.set_label(l_if_branch);
	nif.append(ncond);
	nif.append(nblock);

	// Reset terminal
	feeder->set_end();

	return nif;
}

static node_manager cc_elif(Feeder *feeder,
		Engine *context,
		const Args &args,
		Pardon &pardon,
		State *state)
{
	// Return
	node_manager nelif;

	// Check the state
	char c;
	while ((c = feeder->feed()) != '(');

	std::string paren = feeder->extract_parenthesized();

	// Skip construction step or something
	node_manager ncond(context, paren, args, pardon);

	// TODO: Add a skip whitespace
	while (isspace(c = feeder->feed()));
	if (c != '{')
		feeder->backup(1);
		
	feeder->set_end((c == '{') ? '}' : '\n');

	node_manager nblock = cc_parse(feeder, context,
			args, pardon);

	nelif.set_label(l_elif_branch);
	nelif.append(ncond);
	nelif.append(nblock);

	// Reset terminal
	feeder->set_end();

	return nelif;
}

static node_manager cc_else(Feeder *feeder,
		Engine *context,
		const Args &args,
		Pardon &pardon,
		State *state)
{
	// Return
	node_manager nelse;

	char c;
	// TODO: Add a skip whitespace
	while (isspace(c = feeder->feed()));
	if (c != '{')
		feeder->backup(1);
		
	feeder->set_end((c == '{') ? '}' : '\n');

	node_manager nblock = cc_parse(feeder, context,
			args, pardon);

	nelse.set_label(l_else_branch);
	nelse.append(nblock);

	// Reset terminal
	feeder->set_end();

	return nelse;
}

static node_manager cc_while(Feeder *feeder,
		Engine *ctx,
		const Args &args,
		Pardon &pardon,
		State *state)
{
	// Return
	node_manager nwhile;

	char c;
	while ((c = feeder->feed()) != '(');

	std::string paren = feeder->extract_parenthesized();

	// Skip construction step or something
	node_manager ncond(ctx, paren, args, pardon);

	while (isspace(c = feeder->feed()));
	if (c != '{')
		feeder->backup(1);
		
	feeder->set_end((c == '{') ? '}' : '\n');

	node_manager nloop = cc_parse(feeder, ctx, args, pardon);

	nwhile.set_label(l_while_loop);
	nwhile.append(ncond);
	nwhile.append(nloop);

	// Reset terminal
	feeder->set_end();

	return nwhile;
}

node_manager cc_keyword(std::string &cache,
		Feeder *feeder,
		Engine *context,
		const Args &args,
		Pardon &pardon,
		State *state)
{
	using Processor = std::function <node_manager (Feeder *,
			Engine *,
			const Args &,
			Pardon &,
			State *)>;
	
	static const Symtab <Processor> keywords {
		{"if", cc_if},
		{"elif", cc_elif},
		{"else", cc_else},
		{"while", cc_while}
	};

	if (keywords.find(cache) == keywords.end())
		return node_manager();
	
	// Clear cache if the keyword extraction is successful
	node_manager nm = (keywords.at(cache))(feeder, context,
			args, pardon, state);

	if (!nm.empty())
		cache.clear();
	
	return nm;
}

}