#include <engine.hpp>

#include <core/common.hpp>
#include <core/node_manager.hpp>

namespace zhetapi {

node_manager::node_manager() {}

node_manager::node_manager(const node_manager &other)
		: __engine(other.__engine), __tree(other.__tree),
		__refs(other.__refs), __params(other.__params)
{
	rereference(__tree);
}

node_manager::node_manager(const node &tree, Engine *engine)
		: __engine(engine), __tree(tree)
{
	// Unpack variable clusters
	expand(__tree);

	// Label the tree
	label(__tree);
	count_up(__tree);
}

node_manager::node_manager(const node &tree, const Args &args, Engine *engine)
		: __engine(engine), __tree(tree), __params(args)
{
	// TODO: Put this in a method
	//
	// Fill references
	node tmp;
	for (std::string str : args) {
		tmp = nf_zero();

		tmp.__label = l_variable;

		__refs.push_back(tmp);
	}

	// Unpack variable clusters
	expand(__tree);

	// Label the tree
	label(__tree);
	count_up(__tree);
	
	rereference(__tree);
}

node_manager::node_manager(const std::string &str, Engine *engine)
		: __engine(engine)
{
	zhetapi::parser pr;

	siter iter = str.begin();
	siter end = str.end();

	bool r = qi::phrase_parse(iter, end, pr, qi::space, __tree);

	// Unpack variable clusters
	expand(__tree);

	// Label the tree
	label(__tree);
	count_up(__tree);
}

node_manager::node_manager(const std::string &str,
		const std::vector <std::string> &params,
		Engine *engine) 
		: __params(params), __engine(engine) 
{
	parser pr;

	siter iter = str.begin();
	siter end = str.end();

	bool r = qi::phrase_parse(iter, end, pr, qi::space, __tree);

	// Fill references
	node tmp;
	for (std::string str : params) {
		tmp = nf_zero();

		tmp.__label = l_variable;

		__refs.push_back(tmp);
	}

	// Unpack variable clusters
	expand(__tree);

	// Label the tree
	label(__tree);
	count_up(__tree);
}

node_manager &node_manager::operator=(const node_manager &other)
{
	if (this != &other) {
		__engine = other.__engine;
		__tree = other.__tree;
		__refs = other.__refs;
		__params = other.__params;

		rereference(__tree);
	}

	return *this;
}

// Properties
bool node_manager::empty() const
{
	return __tree.empty();
}

size_t node_manager::num_args() const
{
	return __params.size();
}

const node &node_manager::get_tree() const
{
	return __tree;
}

// Setters
void node_manager::set_label(lbl label)
{
	__tree.__label = label;
}

void node_manager::set_engine(Engine *engine)
{
	__engine = engine;
}

// Value finding methods
Token *node_manager::value() const
{
	return value(__tree);
}

// Sequential value (returns null for now)
Token *node_manager::sequential_value() const
{
	// Assumes that the top node is a sequential
	for (node nd : __tree.__leaves)
		value(nd);
	
	return nullptr;
}

Token *node_manager::value(node tree) const
{
	std::vector <Token *> values;

	Token *tptr;
	Token *vptr;

	Variable v;
	Variable *vp;

	algorithm *aptr;

	std::string ident;

	int size;

	// If null token, resort to special execution modes
	// (Use this instead of sequential_value for
	// algorithms; dont remove it though)
	if (tree.null()) {
		if (tree.label() == l_assignment_chain) {
			// Evaluate first node
			Token *tmp = value(tree[0]);

			// Assign for the other nodes

			// Add index operator for nodes
			size_t nleaves = tree.child_count(); // Use a method instead

			for (size_t i = 1; i < nleaves; i++) {
				// Ensure that the node has type lvalue
				if (tree[i].label() != l_lvalue)
					throw std::runtime_error("Need an lvalue on the left side of an \'=\'");
				
				lvalue *lv = tree[i].cast <lvalue> ();

				lv->assign(tmp);
			}

			return nullptr;
		} else {
			throw std::runtime_error("Unknown execution mode \'" + strlabs[tree.__label] + "\'");
		}
	}

	// else: this func
	
	// TODO: Add a method for nodes to cast the token (ie. tree.cast <type> ())
	
	switch (tree.caller()) {
	case Token::opd:
		return tree.copy_token();
	case Token::oph:	
		size = tree.__leaves.size();
		for (node leaf : tree.__leaves)
			values.push_back(value(leaf));

		tptr = __engine->compute((dynamic_cast <operation_holder *>
						(tree.__tptr))->rep, values);
		
		if (tree.__label == l_post_modifier) {
			vptr = tree.__leaves[0].__tptr;

			vp = dynamic_cast <Variable *> (vptr);

			ident = vp->symbol();
			
			v = Variable(tptr, ident);

			__engine->put(v);

			tptr = vp->get();

			return tptr->copy();
		} else if (tree.__label == l_pre_modifier) {
			vptr = tree.__leaves[0].__tptr;

			// zhetapi_cast({vptr}, vp);
			vp = dynamic_cast <Variable *> (vptr);

			ident = vp->symbol();
			
			v = Variable(tptr, ident);

			__engine->put(v);
		}

		return tptr->copy();
	case Token::var:
		return (tree.cast <Variable> ())->get()->copy();
	case Token::token_rvalue:
		return (tree.cast <rvalue> ())->get()->copy();
	case Token::ndr:
		return (tree.cast <node_reference> ())->get()->copy_token();
	case Token::token_node_list:
		return (tree.cast <node_list> ())->evaluate(__engine);
	case Token::ftn:
		if (tree.__leaves.empty())
			return tree.__tptr->copy();
		
		for (node leaf : tree.__leaves)
			values.push_back(value(leaf));

		tptr = (*(dynamic_cast <Function *> (tree.__tptr)))(values);

		return tptr->copy();
	case Token::reg:
		for (node leaf : tree.__leaves)
			values.push_back(value(leaf));

		tptr = (*(dynamic_cast <Registrable *> (tree.__tptr)))(values);

		if (tptr)
			return tptr->copy();

		break;
	case Token::alg:
		for (node leaf : tree.__leaves)
			values.push_back(value(leaf));
		
		aptr = dynamic_cast <algorithm *> (tree.__tptr);
		tptr = aptr->execute(__engine, values);

		if (tptr)
			return tptr->copy();

		break;
	default:
		break;
	}

	return nullptr;
}

Token *node_manager::substitute_and_compute(std::vector <Token *>
		&toks, size_t total_threads)
{
	assert(__refs.size() == toks.size());
	for (size_t i = 0; i < __refs.size(); i++) {
		__refs[i] = node(toks[i], {});

		label(__refs[i]);
	}

	return value(__tree);
}

Token *node_manager::substitute_and_seq_compute(Engine *ext,
		const std::vector <Token *> &toks, size_t total_threads)
{
	assert(__refs.size() == toks.size());
	for (size_t i = 0; i < __refs.size(); i++) {
		__refs[i] = node(toks[i], {});

		label(__refs[i]);
	}

	return sequential_value();
}

void node_manager::append(const node &n)
{
	__tree.append(n);

	// Add the rest of the elements
	count_up(__tree);
}

void node_manager::append(const node_manager &nm)
{
	__tree.append(nm.__tree);

	// Add the rest of the elements
	count_up(__tree);
}

// Better names
void node_manager::append_front(const node &n)
{
	__tree.append_front(n);

	// Add the rest of the elements
	count_up(__tree);
}

void node_manager::append_front(const node_manager &nm)
{
	__tree.append_front(nm.__tree);

	// Add the rest of the elements
	count_up(__tree);
}

void node_manager::add_args(const std::vector <std::string> &args)
{
	// Fill references
	node tmp;
	for (std::string str : args) {
		tmp = nf_zero();

		tmp.__label = l_variable;

		__refs.push_back(tmp);
	}

	// Fix broken variable references
	rereference(__tree);
}

// Expansion methods
void node_manager::expand(node &ref)
{
	if (ref.__tptr->caller() == Token::vcl) {
		/*
		 * Excluding the parameters, the variable cluster should
		 * always be a leaf of the tree.
		 */

		variable_cluster *vclptr = dynamic_cast
			<variable_cluster *> (ref.__tptr);

		ref = expand(vclptr->__cluster, ref.__leaves);
	}

	for (node &leaf : ref.__leaves)
		expand(leaf);
}

node node_manager::expand(const std::string &str, const std::vector <node> &leaves)
{
	typedef std::vector <std::pair <std::vector <node>, std::string>> ctx;
		
	ctx contexts;

	contexts.push_back({{}, ""});

	// Check once for differential
	Token *dtok = __engine->get("d");
	auto ditr = std::find(__params.begin(), __params.end(), "d");

	for (size_t i = 0; i < str.length(); i++) {
		ctx tmp;

		for (auto &pr : contexts) {
			pr.second += str[i];
		
			auto itr = std::find(__params.begin(), __params.end(), pr.second);

			size_t index = std::distance(__params.begin(), itr);

			Token *tptr = __engine->get(pr.second);

			// Potential differential node
			Token *dptr = nullptr;
			auto diff = __params.end();
			if ((pr.second)[0] == 'd'
				&& (dtok == nullptr)
				&& (ditr == __params.end())) {
				// Priority on parameters
				diff = find(__params.begin(), __params.end(), pr.second.substr(1));
				dptr = __engine->get(pr.second.substr(1));
			}

			size_t dindex = std::distance(__params.begin(), diff);

			bool matches = true;

			node t;
			if (__engine->present(pr.second)) {
				t = node(new operation_holder(pr.second), {});
			} else if (itr != __params.end()) {
				t = node(new node_reference(&__refs[index], pr.second, index, true), {});
			} else if (tptr != nullptr) {
				// Delaying actual evaluation to
				// evaluation - better for algorithms,
				// where values are not always known
				// for sure
				//
				// TODO: Add special case for base scope,
				// where dependencies can be ignored (does
				// not include if/else statements)
				//
				// TODO: Add a block class (maybe oversees
				// the algorithm class as well)
				//
				// Only use rvalue for variables
				if (tptr->caller() == Token::var) {
					rvalue *rv = new rvalue((dynamic_cast <Variable *> (tptr))->symbol(), __engine);

					t = node(rv);
				} else {
					// t = node(new rvalue(pr.second, __engine), {});
					t = node(tptr);
				}
			} else if (diff != __params.end()) {
				t = node(new node_differential(new node_reference(&__refs[dindex], pr.second.substr(1), dindex, true)), {});
			} else if (dptr != nullptr) {
				t = node(new node_differential(dptr), {});
			} else {
				matches = false;
			}

			if (matches) {
				tmp.push_back(pr);

				pr.first.push_back(t);
				pr.second.clear();
			}
		}

		for (auto pr : tmp)
			contexts.push_back(pr);
	}

	/*
	 * Extract the optimal choice. This decision is made based on
	 * the number of Tokens read. The heurestic used chooses a
	 * node list which has undergone complete parsing (no leftover
	 * string), and whose size is minimal.
	 */
	std::vector <node> choice;

	bool valid = false;
	for (auto pr : contexts) {
		if (pr.second.empty()) {
			valid = true;

			if (choice.size() == 0)
				choice = pr.first;
			else if (choice.size() > pr.first.size())
				choice = pr.first;
		}
	}
	
	/*
	 * If tmp is not empty, it implies that we could not find a
	 * match for it, and therefore the parsing is incomplete.
	 */
	if (!valid)
		throw undefined_symbol(str);

	/*
	 * The very last Token is attributed the leaves
	 */
	choice[choice.size() - 1].__leaves = leaves;

	/*
	 * Binary fusing. Advantageous to linear fusing in the way in
	 * which it produces a tree with fewer multiplication nodes.
	 */
	while (choice.size() > 1) {
		std::vector <node> tmp;

		size_t n = choice.size();

		for (size_t i = 0; i < n/2; i++) {
			tmp.push_back(node(new operation_holder("*"),
						{choice[i], choice[i +
						1]}));
		}

		if (n % 2)
			tmp.push_back(choice[n - 1]);
	
		choice = tmp;
	}


	return choice[0];
}

// Counting nodes
size_t node_manager::count_up(node &ref)
{
	size_t total = 1;
	for (auto &child : ref.__leaves)
		total += count_up(child);
	
	ref.__nodes = total;

	return total;
}

// Simplication methods
void node_manager::simplify()
{
	simplify(__tree);
}

void node_manager::simplify(node &ref)
{
	if (ref.__label == l_operation_constant) {
		ref.transfer(node(value(ref), l_constant, {}));

		return;
	}

	operation_holder *ophptr = ref.cast <operation_holder> ();

	if (ophptr && (ophptr->code == add || ophptr->code == sub)) {
		// Fix subtraction and what not
		// simplify_separable(ref);
	} else if (ophptr && (ophptr->code == mul || ophptr->code == dvs)) {
		simplify_mult_div(ref, ophptr->code);
	} else {
		for (auto &child : ref.__leaves)
			simplify(child);
	}
}

void node_manager::simplify_separable(node &ref)
{
	Token *opd = new opd_z(0);
	Token *zero = new opd_z(0);

	std::stack <node> process;

	std::vector <node> sums;
	
	process.push(ref);

	node top;
	while (!process.empty()) {
		top = process.top();

		process.pop();

		operation_holder *ophptr = dynamic_cast <operation_holder *> (top.__tptr);

		if (ophptr && (ophptr->code == add || ophptr->code == sub)) {
			process.push(top[0]);
			process.push(top[1]);
		} else {
			sums.push_back(top);
		}
	}

	std::vector <node> variables;
	
	std::stack <node> constants;
	for (auto nm : sums) {
		if (is_constant(nm.label()))
			constants.push(nm);
		else
			variables.push_back(nm);
	}
	
	while (!constants.empty()) {
		node nd = constants.top();

		constants.pop();

		opd = __engine->compute("+", {opd, nd.__tptr});
	}

	// Still needs to deal with variables
	std::vector <node> all = variables;

	if (!tokcmp(opd, zero))
		all.push_back(node(opd));

	// Next step is to fold the vector
	while (all.size() > 1) {
		std::vector <node> tmp;

		size_t n = all.size();

		for (size_t i = 0; i < n/2; i++) {
			tmp.push_back(
				node(new operation_holder("+"),
					{
						all[i],
						all[i + 1]
					}
				)
			);
		}

		if (n % 2)
			tmp.push_back(all[n - 1]);
	
		all = tmp;
	}

	ref.transfer(all[0]);
}

void node_manager::simplify_mult_div(node &ref, codes c)
{
	if (c == dvs) {
		lbl l1 = ref.__leaves[0].__label;
		lbl l2 = ref.__leaves[1].__label;

		if (l1 == l_differential && l2 == l_differential) {
			Token *t1 = ref.__leaves[0].__tptr;
			Token *t2 = ref.__leaves[1].__tptr;

			t1 = (dynamic_cast <node_differential *> (t1))->get();
			t2 = (dynamic_cast <node_differential *> (t2))->get();

			Function *ftn = nullptr;
			std::string var;

			if (t1->caller() == Token::ftn)
				ftn = dynamic_cast <Function *> (t1);
			
			if (t2->caller() == Token::ndr)
				var = (dynamic_cast <node_reference *> (t2))->symbol();

			if (ftn && ftn->is_variable(var)) {
				Function f = ftn->differentiate(var);

				ref.__leaves.clear();
				ref.__tptr = f.copy();
			}
		}
	}
}

// Differentiation
void node_manager::differentiate(const std::string &str)
{
	for (size_t i = 0; i < __refs.size(); i++) {
		if (__params[i] == str)
			__refs[i].__label = l_variable;
		else
			__refs[i].__label = l_variable_constant;
	}

	label(__tree);

	differentiate(__tree);

	simplify();
}

// Post-label usage
void node_manager::differentiate(node &ref)
{
	if (is_constant(ref.__label)) {
		ref.transfer(nf_zero());

		return;
	}

	switch (ref.__label) {
	case l_separable:
		differentiate(ref.__leaves[0]);
		differentiate(ref.__leaves[1]);
		break;
	case l_multiplied:
		differentiate_mul(ref);
		break;
	case l_power:
		differentiate_pow(ref);
		break;
	case l_natural_log:
		differentiate_ln(ref);
		break;
	case l_binary_log:
		differentiate_lg(ref);
		break;
	case l_constant_base_log:
		differentiate_const_log(ref);
		break;
	case l_trigonometric:
		differentiate_trig(ref);
		break;
	case l_hyperbolic:
		differentiate_hyp(ref);
		break;
	case l_variable:
		ref.transfer(nf_one());
		break;
	default:
		break;
	}
}

// Refactoring methods
void node_manager::refactor_reference(const std::string &str, Token *tptr)
{
	refactor_reference(__tree, str, tptr);
}

void node_manager::refactor_reference(
		node &ref,
		const std::string &str,
		Token *tptr)
{
	node_reference *ndr = dynamic_cast <node_reference *> (ref.__tptr);
	
	if (ndr && ndr->symbol() == str)
		ref.__tptr = tptr->copy();

	for (node &leaf : ref.__leaves)
		refactor_reference(leaf, str, tptr);
}

// Generation methods
void node_manager::generate(std::string &name) const
{
	std::ofstream fout(name + ".cpp");

	fout << "#include <all/zhplib.hpp>\n";
	fout << "\n";

	fout << "extern \"C\" {\n";
	// Make more robust to T and U
	fout << "\tzhetapi::Engine " << name << "_engine;\n";

	fout << "\n";
	fout << "\tzhetapi::Token *" << name << "(";

	for (size_t i = 0; i < __refs.size(); i++) {
		fout << "zhetapi::Token *in" << (i + 1);

		if (i < __refs.size() - 1)
			fout << ", ";
	}

	fout << ")\n";
	fout << "\t{\n";

	// Counters
	size_t const_count = 1;
	size_t inter_count = 1;

	// Inside the function
	std::string ret = generate(name, __tree, fout, const_count, inter_count);

	fout << "\t\treturn " << ret << ";\n";
	fout << "\t}\n";
	fout << "}" << std::endl;
}

std::string node_manager::generate(std::string name, node ref,
		std::ofstream &fout, size_t &const_count, size_t
		&inter_count) const
{
	std::vector <std::string> idents;

	for (auto leaf : ref.__leaves)
		idents.push_back(generate(name, leaf, fout, const_count, inter_count));
	
	if (is_constant_operand(ref.__label)) {
		fout << "\t\tzhetapi::Token *c" << const_count++ << " = ";
		fout << "new zhetapi::Operand <"
			<< types::proper_symbol(typeid(*(ref.__tptr)))
			<< "> (" << ref.__tptr->str() << ");\n";

		return "c" + std::to_string(const_count - 1);
	} else if (ref.__tptr->caller() == Token::ndr) {
		node_reference *ndr = dynamic_cast <node_reference *> (ref.__tptr);

		return "in" + std::to_string(ndr->index() + 1);
	} else {
		// Assuming we have an operation
		operation_holder *ophtr = dynamic_cast <operation_holder *> (ref.__tptr);

		fout << "\t\tzhetapi::Token *inter" << inter_count++ <<
			" = " << name << "_engine.compute(\"" <<
			ophtr->rep << "\", {";

		for (size_t i = 0; i < idents.size(); i++) {
			fout << idents[i];

			if (i < idents.size() - 1)
				fout << ", ";
		}

		fout << "});\n";

		return "inter" + std::to_string(inter_count - 1);
	}
}

// Displaying utilities
std::string node_manager::display() const
{
	return display(__tree);
}

std::string node_manager::display(node ref) const
{
	switch (ref.__tptr->caller()) {
	case Token::opd:
		return ref.__tptr->str();
	case Token::oph:
		return display_operation(ref);
	case Token::ndr:
		if ((dynamic_cast <node_reference *> (ref.__tptr))->is_variable())
			return (dynamic_cast <node_reference *> (ref.__tptr))->symbol();
		
		return display(*(dynamic_cast <node_reference *> (ref.__tptr)->get()));
	default:
		break;
	}

	return "?";
}

std::string node_manager::display_operation(node ref) const
{
	std::string str = (dynamic_cast <operation_holder *> (ref.__tptr))->rep;
	
	operation_holder *ophptr = dynamic_cast <operation_holder *> (ref.__tptr);

	switch (ophptr->code) {
	case add:
	case sub:
	case mul:
		return display_pemdas(ref, ref.__leaves[0]) + " "
			+ str + " " + display_pemdas(ref, ref.__leaves[1]);
	case dvs:
		return display_pemdas(ref, ref.__leaves[0])
			+ str + display_pemdas(ref, ref.__leaves[1]);
	case pwr:
		return display_pemdas(ref, ref.__leaves[0]) + str
			+ display_pemdas(ref, ref.__leaves[1]);
	case sxn:
	case cxs:
	case txn:
	case sec:
	case csc:
	case cot:

	case snh:
	case csh:
	case tnh:
	case cch:
	case sch:
	case cth:

	case xln:
	case xlg:
		return str + "(" + display_pemdas(ref, ref.__leaves[0]) + ")";
	case lxg:
		// Fix bug with single/double argument overload
		return str + "(" + display_pemdas(ref, ref.__leaves[0])
			+ ", " + display_pemdas(ref, ref.__leaves[1]) + ")";
	default:
		break;
	}

	return str;
}

std::string node_manager::display_pemdas(node ref, node child) const
{
	operation_holder *rophptr = dynamic_cast <operation_holder *> (ref.__tptr);
	operation_holder *cophptr = dynamic_cast <operation_holder *> (child.__tptr);

	if (!cophptr)
		return display(child);

	switch (rophptr->code) {
	case mul:
		if ((cophptr->code == add) || (cophptr->code == sub))
			return display(child);
		
		return display(child);
	case dvs:
		return "(" + display(child) + ")";
	case pwr:
		if ((cophptr->code == add) || (cophptr->code == sub)
			|| (cophptr->code == mul) || (cophptr->code == dvs))
			return "(" + display(child) + ")";
		
		return display(child);
	default:
		break;
	}
	
	return display(child);
}

// Printing utilities
void node_manager::print(bool address) const
{
	node_reference::address = address;

	if (address)
		__tree.print();
	else	
		__tree.print_no_address();

	if (__refs.size()) {
		std::cout << "Refs [" << __refs.size() << "]" << std::endl;
		
		for (auto &ref : __refs) {
			if (address)
				ref.print();
			else
				ref.print_no_address();
		}
	}
}

// Labeling utilities
void node_manager::label(node &ref)
{
	switch (ref.__tptr->caller()) {
	case Token::opd:
		ref.__label = constant_label(ref.__tptr);
		break;
	case Token::oph:
		for (node &leaf : ref.__leaves)
			label(leaf);

		label_operation(ref);

		break;
	case Token::ftn:
		for (node &leaf : ref.__leaves)
			label(leaf);
		
		/* Also add a different labeling if it is constant,
		 * probably needs to be called an operation constant
		 */
		ref.__label = l_function;
		break;
	case Token::var:
		ref.__label = l_variable;
		break;
	case Token::ndr:
		// Transfer labels, makes things easier
		ref.__label = (dynamic_cast <node_reference *>
				(ref.__tptr))->get()->__label;
		break;
	case Token::ndd:
		ref.__label = l_differential;
		break;
	case Token::reg:
		for (node &leaf : ref.__leaves)
			label(leaf);

		ref.__label = l_registrable;
		break;
	default:
		break;
	}
}

void node_manager::label_operation(node &ref)
{
	operation_holder *ophptr = dynamic_cast <operation_holder *> (ref.__tptr);

	bool constant = true;
	for (auto child : ref.__leaves) {
		if (!is_constant(child.__label)) {
			constant = false;
			break;
		}
	}

	if (constant) {
		ref.__label = l_operation_constant;
		return;
	}

	switch (ophptr->code) {
	case add:
	case sub:
		ref.__label = l_separable;
		break;
	case mul:
		ref.__label = l_multiplied;
		break;
	case dvs:
		ref.__label = l_divided;
		break;
	case pwr:
		if ((ref.__leaves[0].__label == l_variable)
			&& (is_constant(ref.__leaves[1].__label)))
			ref.__label = l_power;
		break;
	case xln:
		ref.__label = l_natural_log;
		break;
	case xlg:
		ref.__label = l_binary_log;
		break;
	case lxg:
		if (is_constant(ref.__leaves[0].__label) &&
			!is_constant(ref.__leaves[1].__label))
			ref.__label = l_constant_base_log;
		break;
	case sxn:
	case cxs:
	case txn:
	case sec:
	case csc:
	case cot:
		ref.__label = l_trigonometric;
		break;
	case snh:
	case csh:
	case tnh:
	case cch:
	case sch:
	case cth:
		ref.__label = l_hyperbolic;
		break;
	case pin:
	case pde:
		ref.__label = l_post_modifier;
		break;
	case rin:
	case rde:
		ref.__label = l_pre_modifier;
		break;
	default:
		break;
	}
}

void node_manager::rereference(node &ref)
{
	if (ref.__tptr && (ref.__tptr->caller() == Token::ndr)) {
		std::string tmp = (dynamic_cast <node_reference *> (ref.__tptr))->symbol();

		auto itr = find(__params.begin(), __params.end(), tmp);

		size_t index = std::distance(__params.begin(), itr);

		// Need a new method to clear/reset
		ref.__tptr = new node_reference(&__refs[index], tmp, index, true);
	}

	for (node &leaf : ref.__leaves)
		rereference(leaf);
}

// Arithmetic
node_manager operator+(const node_manager &a, const node_manager &b)
{
	// TODO: Add a union operation for Engines
	return node_manager(
		node(new operation_holder("+"), {
			a.get_tree(),
			b.get_tree()
		}),
		args_union(a.__params, b.__params),
		a.__engine
	);
}

node_manager operator-(const node_manager &a, const node_manager &b)
{
	// TODO: Add a method to make this a one-liner
	return node_manager(
		node(new operation_holder("-"), {
			a.get_tree(),
			b.get_tree()
		}),
		args_union(a.__params, b.__params),
		a.__engine
	);
}

// Static methods
bool node_manager::loose_match(const node_manager &a, const node_manager &b)
{
	return node::loose_match(a.__tree, b.__tree);
}

// Node factories
node node_manager::nf_one()
{
	return node(new opd_z(1), {});
}

node node_manager::nf_zero()
{
	return node(new opd_z(0), {});
}

}
