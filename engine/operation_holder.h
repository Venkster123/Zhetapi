#ifndef OPERATION_HOLDER_H_
#define OPERATION_HOLDER_H_

#include <string>

#include "token.h"

enum codes {
	add,
	sub,
	mul,
	dvs
};

std::string strcodes[] = {
	"add",
	"subtract",
	"multiply",
	"divide"
};

struct operation_holder : public token {
	std::string rep;

	codes code;

	operation_holder(const std::string &);

	type caller() const override;
	token *copy() const override;
	std::string str() const override;

	virtual bool operator==(token *) const override;
};

operation_holder::operation_holder(const std::string &str) : rep(str)
{
	if (str == "+")
		code = add;
	else if (str == "-")
		code = sub;
	else if (str == "*")
		code = mul;
	else if (str == "/")
		code = dvs;
}

token::type operation_holder::caller() const
{
	return token::OPERATION_HOLDER;
}

token *operation_holder::copy() const
{
	return new operation_holder(rep);
}

std::string operation_holder::str() const
{
	return rep + " [" + strcodes[code] + "]";
}

bool operation_holder::operator==(token *tptr) const
{
	operation_holder *oph = dynamic_cast <operation_holder *> (tptr);

	if (oph == nullptr)
		return false;

	return oph->rep == rep;
}

#endif