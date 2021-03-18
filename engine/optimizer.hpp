#ifndef OPTIMIZER_H_
#define OPTIMIZER_H_

// Engine headers
#include <dataset.hpp>
#include <gradient.hpp>

namespace zhetapi {

namespace ml {

// Optimizer class
template <class T>
class Optimizer {
protected:
	T		__eta		= 0;

	// Functions
	Optimizer(T);
public:
	void set_learning_rate(T);
	
	virtual Matrix <T> *update(
			Matrix <T> *,
			size_t) = 0;
};

template <class T>
Optimizer <T> ::Optimizer(T lr) : __eta(lr) {}

template <class T>
void Optimizer <T> ::set_learning_rate(T lr)
{
	__eta = lr;
}

}

}

#endif
