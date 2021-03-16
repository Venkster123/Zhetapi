#ifndef DNN_H_
#define DNN_H_

// C/C++ headers
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <memory>
#include <thread>
#include <vector>

#include <unistd.h>

// JSON library
#include <json/json.hpp>

// Engine headers
#include <core/kernels.hpp>

#include <dataset.hpp>
#include <display.hpp>
#include <layer.hpp>
#include <gradient.hpp>
#include <optimizer.hpp>

#ifdef ZHP_CUDA

#include <cuda/activation.cuh>
#include <cuda/erf.cuh>
#include <cuda/matrix.cuh>
#include <cuda/vector.cuh>

#else

#include <activation.hpp>
#include <erf.hpp>
#include <matrix.hpp>
#include <vector.hpp>

#endif

// Default path to engine
#ifndef ZHP_ENGINE_PATH

#define ZHP_ENGINE_PATH "engine"

#endif

namespace zhetapi {

namespace ml {

/**
 * @brief Neural network class
 */
template <class T = double>
class DNN {
public:
	// Exceptions
	class bad_io_dimensions {};
	class null_optimizer {};
	class null_loss_function {};
private:
	Layer <T> *		__layers	= nullptr;

	size_t			__size		= 0;
	size_t			__isize		= 0;
	size_t			__osize		= 0;

	// Remove erf and optimizer later
	Erf <T> *		__cost		= nullptr; // Safe to copy
	Optimizer <T> *         __opt		= nullptr;

	void clear();
public:
	DNN();
	DNN(const DNN &);
	DNN(size_t, const std::vector <Layer <T>> &);

	~DNN();

	DNN &operator=(const DNN &);

	// Saving and loading the network
	void save(const std::string &);
	void load(const std::string &);
	// void load_json(const std::string &);
	
	// Properties
	size_t size() const;
	size_t input_size() const;
	size_t output_size() const;

	// Setters
	void set_cost(Erf <T> *);
	void set_optimizer(Optimizer <T> *);

	void diagnose() const;
	void print() const;

	// Computation
	Vector <T> operator()(const Vector <T> &);

	Vector <T> compute(const Vector <T> &);
	
	void fit(const Vector <T> &, const Vector <T> &);
	void fit(const DataSet <T> &, const DataSet <T> &);

	void multithreaded_fit(const DataSet <T> &, const DataSet <T> &, size_t = 1);
	
	void apply_gradient(Matrix <T> *);

	Matrix <T> *get_gradient(const Vector <T> &);
	Matrix <T> *get_gradient(const Vector <T> &, const Vector <T> &);
};

// Constructors and other memory related operations
template <class T>
DNN <T> ::DNN() {}

template <class T>
DNN <T> ::DNN(const DNN &other) :
		__size(other.__size), __isize(other.__isize),
		__osize(other.__osize), __cost(other.__cost),
		__opt(other.__opt)
{
	__layers = new Layer <T> [__size];

	for (size_t i = 0; i < __size; i++)
		__layers[i] = other.__layers[i];
}

/*
 * TODO: Add a input class from specifying input format.
 * NOTE: The pointers allocated and passed into this function
 * should be left alone. They will be deallocated once the scope
 * of the network object comes to its end. In other words, DO
 * NOT FREE ACTIVATION POINTERS, and instead let the
 * DNN class do the work for you.
 */
template <class T>
DNN <T> ::DNN(size_t isize, const std::vector <Layer <T>> &layers)
		: __size(layers.size()), __isize(isize)
{
	__layers = new Layer <T> [__size];

	size_t tmp = isize;
	for (int i = 0; i < __size; i++) {
		__layers[i] = layers[i];

		__layers[i].set_fan_in(tmp);

		__layers[i].initialize();

		tmp = __layers[i].get_fan_out();
	}

	__osize = tmp;
}

template <class T>
DNN <T> ::~DNN()
{
	clear();
}

template <class T>
DNN <T> &DNN <T> ::operator=(const DNN <T> &other)
{
	if (this != &other) {
		clear();

		__size = other.__size;
		__isize = other.__isize;
		__osize = other.__osize;

		__cost = other.__cost;
		__opt = other.__opt;

		__layers = new Layer <T> [__size];
		for (size_t i = 0; i < __size; i++)
			__layers[i] = other.__layers[i];
	}

	return *this;
}

template <class T>
void DNN <T> ::clear()
{
	delete[] __layers;
}

// Saving and loading
template <class T>
void DNN <T> ::save(const std::string &file)
{
	std::ofstream fout(file);

	fout.write((char *) &__size, sizeof(size_t));
	fout.write((char *) &__isize, sizeof(size_t));
	fout.write((char *) &__osize, sizeof(size_t));

	for (int i = 0; i < __size; i++)
		__layers[i].write(fout);
}

template <class T>
void DNN <T> ::load(const std::string &file)
{
	// Clear the current members
	clear();

	std::ifstream fin(file);

	fin.read((char *) &__size, sizeof(size_t));
	fin.read((char *) &__isize, sizeof(size_t));
	fin.read((char *) &__osize, sizeof(size_t));

	__layers = new Layer <T> [__size];
	for (size_t i = 0; i < __size; i++)
		__layers[i].read(fin);
}

// Properties
template <class T>
size_t DNN <T> ::size() const
{
	return __size;
}

template <class T>
size_t DNN <T> ::input_size() const
{
	return __isize;
}

template <class T>
size_t DNN <T> ::output_size() const
{
	return __osize;
}

/*
template <class T>
void DNN <T> ::load_json(const std::string &file)
{
	std::ifstream fin(file);

	nlohmann::json structure;

	fin >> structure;

	auto layers = structure["Layers"];

	// Allocate size information
	__size = layers.size();
	__isize = layers[0]["Neurons"];
	__osize = layers[__size - 1]["Neurons"];

	// Allocate matrices and activations
	__weights = new Matrix <T> [__size - 1];
	__momentum = new Matrix <T> [__size - 1];
	__layers = new Layer <T> [__size];

	// Allocate caches
	__a = std::vector <Vector <T>> (__size);
	__z = std::vector <Vector <T>> (__size - 1);

	std::vector <size_t> sizes;
	for (size_t i = 0; i < __size; i++) {
		auto layer = layers[i];

		auto activation = layer["Activation"];
		size_t neurons = layer["Neurons"];

		std::vector <T> args;

		for (auto arg : activation["Arguments"])
			args.push_back(arg);

		std::string name = activation["Name"];

		sizes.push_back(layer["Neurons"]);

		__layers[i].second = Activation <T> ::load(name, args);
	}

	for (size_t i = 1; i < __size; i++) {
		__weights[i - 1] = Matrix <T> (sizes[i], sizes[i - 1] + 1, T(0));
		__momentum[i - 1] = Matrix <T> (sizes[i], sizes[i - 1] + 1, T(0));
	}

	__random = default_initializer <T> {};
	__cmp = default_comparator;
} */

// Property managers and viewers
template <class T>
void DNN <T> ::set_cost(Erf <T> *cost)
{
	__cost = cost;
}

template <class T>
void DNN <T> ::set_optimizer(Optimizer <T> *opt)
{
	__opt = opt;
}

// Computation
template <class T>
Vector <T> DNN <T> ::operator()(const Vector <T> &in)
{
	return compute(in);
}

template <class T>
Vector <T> DNN <T> ::compute(const Vector <T> &in)
{
	Vector <T> tmp = in;

	for (size_t i = 0; i < __size; i++)
		tmp = __layers[i].forward_propogate(tmp);

	return tmp;
}

template <class T>
void DNN <T> ::fit(const Vector <T> &in, const Vector <T> &out)
{
	if ((in.size() != __isize) || (out.size() != __osize))
		throw bad_io_dimensions();

	if (!__opt)
		throw null_optimizer();
	
	if (!__cost)
		throw null_loss_function();

	Matrix <T> *J = __opt->gradient(__layers, __size, in, out, __cost);

	for (size_t i = 0; i < __size; i++)
		__layers[i].apply_gradient(J[i]);

	delete[] J;
}

template <class T>
void DNN <T> ::fit(const DataSet <T> &ins, const DataSet <T> &outs)
{
	if (ins.size() != outs.size())
		throw bad_io_dimensions();

	if ((ins[0].size() != __isize) || (outs[0].size() != __osize))
		throw bad_io_dimensions();

	if (!__opt)
		throw null_optimizer();
	
	if (!__cost)
		throw null_loss_function();

	Matrix <T> *J = __opt->batch_gradient(__layers, __size, ins, outs, __cost);

	for (size_t i = 0; i < __size; i++)
		__layers[i].apply_gradient(J[i]);

	delete[] J;
}

template <class T>
void DNN <T> ::multithreaded_fit(
		const DataSet <T> &ins,
		const DataSet <T> &outs,
		size_t threads)
{
	if (ins.size() != outs.size())
		throw bad_io_dimensions();

	if ((ins[0].size() != __isize) || (outs[0].size() != __osize))
		throw bad_io_dimensions();

	if (!__opt)
		throw null_optimizer();
	
	if (!__cost)
		throw null_loss_function();

	Matrix <T> *J = __opt->multithreaded_batch_gradient(
			__layers,
			__size,
			ins,
			outs,
			__cost,
			threads);

	for (size_t i = 0; i < __size; i++)
		__layers[i].apply_gradient(J[i]);

	delete[] J;
}

template <class T>
Matrix <T> *DNN <T> ::get_gradient(const Vector <T> &in)
{
	if (in.size() != __isize)
		throw bad_io_dimensions();

	Vector <T> *a = new Vector <T> [__size + 1];
	Vector <T> *z = new Vector <T> [__size];

	Matrix <T> *J = jacobian(__layers, __size, __osize, a, z, in);

	delete[] a;
	delete[] z;

	return J;
}

template <class T>
Matrix <T> *DNN <T> ::get_gradient(const Vector <T> &in, const Vector <T> &delta)
{
	if (in.size() != __isize || delta.size() != __osize)
		throw bad_io_dimensions();

	Vector <T> *a = new Vector <T> [__size + 1];
	Vector <T> *z = new Vector <T> [__size];

	Matrix <T> *J = jacobian(__layers, __size, __osize, a, z, in, delta);

	delete[] a;
	delete[] z;

	return J;
}

template <class T>
void DNN <T> ::apply_gradient(Matrix <T> *J)
{
	for (size_t i = 0; i < __size; i++)
		__layers[i].apply_gradient(J[i]);
}

// Miscellaneous
template <class T>
void DNN <T> ::diagnose() const
{
	for (size_t i = 0; i < __size; i++)
		__layers[i].diagnose();
}

template <class T>
void DNN <T> ::print() const
{
	for (size_t i = 0; i < __size; i++)
		__layers[i].print();
}

}

}

#endif
