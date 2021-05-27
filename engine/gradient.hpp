#ifndef GRADIENT_H_
#define GRADIENT_H_

#ifndef __AVR	// Does not support AVR

// C/C++ headers
#include <mutex>
#include <thread>

#endif		// Does not support AVR

// Engine headers
#include "matrix.hpp"
#include "erf.hpp"
#include "vector.hpp"
#include "layer.hpp"
#include "dataset.hpp"

namespace zhetapi {

namespace ml {

template <class T>
Vector <T> simple_compute(
		Layer <T> *layers,
		size_t size,
		const Vector <T> &in)
{
	Vector <T> prv = in;
	Vector <T> tmp = in;

	size_t i = 0;
	while (i < size)
		layers[i++].forward_propogate(tmp, prv);

	return tmp;
}

template <class T>
Vector <T> simple_compute_cached(
		Layer <T> *layers,
		size_t size,
		Vector <T> *a,
		Vector <T> *z,
		const Vector <T> &in)
{
	Vector <T> prv = in;
	Vector <T> tmp = in;

	size_t i = 0;
	while (i < size) {
		a[i] = tmp.append_above(T (1));

		layers[i].forward_propogate(tmp, prv);
		
		z[i++] = layers[i]._dact->compute(prv);
	}

	a[i] = tmp;

	return tmp;
}

// Jacobian during the processing of input to output
template <class T>
Matrix <T> *jacobian_kernel(
		Layer <T> *layers,
		size_t size,
		size_t osize,
		Vector <T> *a,
		Vector <T> *z,
		const Vector <T> &in)
{
	// Compute the actual value
	Vector <T> actual = simple_compute_cached(layers, size, a, z, in);

	// Construction the Jacobian using backpropogation
	Matrix <T> *J = new Matrix <T> [size];

	Vector <T> delta(osize, 1);
	
	for (int i = size - 1; i >= 0; i--) {
		if (i < size - 1) {
			delta = AVR_SWITCH(
				vvt_mult(delta, a[i]),
				std::move(rmt_and_mult(layers[i + 1]._mat, delta))
			);
		}

		delta.stable_shur(z[i]);

		J[i] = AVR_SWITCH(
			vvt_mult(delta, a[i]),
			std::move(vvt_mult(delta, a[i]))
		);
	}

	// Return the gradient
	return J;
}

// Jacobian during the processing of input to output
template <class T>
Matrix <T> *jacobian_kernel(
		Layer <T> *layers,
		size_t size,
		size_t osize,
		Vector <T> *a,
		Vector <T> *z,
		const Vector <T> &in,
		Vector <T> &delta)
{
	// Compute the actual value
	Vector <T> actual = simple_compute_cached(layers, size, a, z, in);

	// Construction the Jacobian using backpropogation
	Matrix <T> *J = new Matrix <T> [size];
	
	for (int i = size - 1; i >= 0; i--) {
		if (i < size - 1) {
			delta = AVR_SWITCH(
				rmt_and_mult(layers[i + 1]._mat, delta),
				std::move(rmt_and_mult(layers[i + 1]._mat, delta))
			);
		}

		delta.stable_shur(z[i]);

		J[i] = AVR_SWITCH(
			vvt_mult(delta, a[i]),
			std::move(vvt_mult(delta, a[i]))
		);
	}

	// Return the gradient
	return J;
}

template <class T>
Matrix <T> *jacobian_kernel_check(
		Layer <T> *layers,
		size_t size,
		size_t osize,
		Vector <T> *a,
		Vector <T> *z,
		const Vector <T> &in)
{
}

template <class T>
Matrix <T> *simple_gradient(
		Layer <T> *layers,
		size_t size,
		Vector <T> *a,
		Vector <T> *z,
		const Vector <T> &in,
		const Vector <T> &out,
		Erf <T> *cost)
{
	// Compute the actual value
	Vector <T> actual = simple_compute_cached(layers, size, a, z, in);

	// Get the derivative of the cost
	Erf <T> *dcost = cost->derivative();

	// Construction the Jacobian using backpropogation
	Matrix <T> *J = new Matrix <T> [size];

	Vector <T> delta = dcost->compute(out, actual);
	
	for (int i = size - 1; i >= 0; i--) {
		if (i < size - 1) {
			delta = AVR_SWITCH(
				rmt_and_mult(layers[i + 1]._mat, delta),
				std::move(rmt_and_mult(layers[i + 1]._mat, delta))
			);
		}

		delta.stable_shur(z[i]);

		J[i] = AVR_SWITCH(
			vvt_mult(delta, a[i]),
			std::move(vvt_mult(delta, a[i]))
		);
	}

	// Free resources
	delete dcost;

	// Return the gradient
	return J;
}

#ifndef __AVR	// Does not support AVR

// TODO: Should there be an alternative for DataSet?
template <class T>
Matrix <T> *simple_batch_gradient(
		Layer <T> *layers,
		size_t size,
		Vector <T> *a,
		Vector <T> *z,
		const DataSet <T> &ins,
		const DataSet <T> &outs,
		Erf <T> *cost)
{
	size_t ds = ins.size();

	Matrix <T> *J = simple_gradient(
			layers,
			size,
			a,
			z,
			ins[0],
			outs[0],
			cost);

	for (size_t i = 1; i < ds; i++) {
		Matrix <T> *Q = nullptr;

		Q = simple_gradient(layers, size, a,
					z, ins[i], outs[i], cost);

		for (size_t k = 0; k < size; k++)
			J[k] += Q[k];

		delete[] Q;
	}

	for (size_t i = 0; i < size; i++)
		J[i] /= T(ds);

	return J;
}

template <class T>
Matrix <T> *simple_multithreaded_batch_gradient(
		Layer <T> *layers,
		size_t size,
		const DataSet <T> &ins,
		const DataSet <T> &outs,
		Erf <T> *cost,
		size_t threads)
{
	size_t ds = ins.size();


	Vector <T> *ta = new Vector <T> [size + 1];
	Vector <T> *tz = new Vector <T> [size];

	Matrix <T> *J = simple_gradient(layers, size, ta,
			tz, ins[0], outs[0], cost);

	std::mutex task_mtx;
	std::mutex addm_mtx;

	size_t task = 1;
	auto ftn = [&]() {
		Vector <T> *a = new Vector <T> [size + 1]; 
		Vector <T> *z = new Vector <T> [size]; 
		
		Matrix <T> *Q;
		int t;

		while (true) {
			t = -1;

			task_mtx.lock();

			if (task < ds) {
				t = task;

				task++;
			}

			task_mtx.unlock();

			if (t < 0)
				break;

			Q = simple_gradient(layers, size, a,
					z, ins[t], outs[t], cost);
			
			addm_mtx.lock();

			for (size_t k = 0; k < size; k++)
				J[k] += Q[k];

			addm_mtx.unlock();

			delete[] Q;
		}

		delete[] a;
		delete[] z;
	};

	std::thread *army = new std::thread[threads];
	for (size_t i = 0; i < threads; i++)
		army[i] = std::thread(ftn);

	for (size_t i = 0; i < threads; i++)
		army[i].join();

	for (size_t i = 0; i < size; i++)
		J[i] /= T(ds);
	
	delete[] ta;
	delete[] tz;

	return J;
}

#endif		// Does not support AVR

}

}

#endif
