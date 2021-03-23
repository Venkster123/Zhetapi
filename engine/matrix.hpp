#ifndef MATRIX_H_
#define MATRIX_H_

// C/C++ headers
#ifndef __AVR			// AVR support

#include <cassert>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>

#endif					// AVR support

// Engine headers
#ifdef ZHP_CUDA

#include <cuda/tensor.cuh>

#else

#include <tensor.hpp>

#endif

#include <cuda/essentials.cuh>

// Redeclare minor as a matrix operation
#ifdef minor

#undef minor

#endif

// In function initialization
#define inline_init_mat(mat, rs, cs)			\
	Matrix <T> mat;					\
							\
	mat.__rows = rs;				\
	mat.__cols = cs;				\
	mat.__size = rs * cs;				\
							\
	mat.__array = new T[rs * cs];			\
							\
	memset(mat.__array, 0, rs * cs * sizeof(T));	\
							\
	mat.__dims = 2;					\
							\
	mat.__dim = new size_t[2];			\
							\
	mat.__dim[0] = rs;				\
	mat.__dim[1] = cs;

namespace zhetapi {
template <class T>
class Vector;

/**
 * @brief A general Matrix class
 * (could be a single row/col vector)
 * that supports conventional operations
 * that matrices in mathematics do.
 */
template <class T>
class Matrix : public Tensor <T> {
protected:
	// Remove later
	size_t  __rows	= 0;
	size_t  __cols	= 0;
public:
	__cuda_dual_prefix Matrix();
	__cuda_dual_prefix Matrix(const Matrix &);
	__cuda_dual_prefix Matrix(const Vector <T> &);

	// Scaled
	__cuda_dual_prefix Matrix(const Matrix &, T);

	__cuda_dual_prefix Matrix(size_t, size_t, T = T());

	// Cuda and avr ignore
	__avr_ignore(Matrix(size_t, size_t, std::function <T (size_t)>);)
	__avr_ignore(Matrix(size_t, size_t, std::function <T *(size_t)>);)
	
	__avr_ignore(Matrix(size_t, size_t, std::function <T (size_t, size_t)>);)
	__avr_ignore(Matrix(size_t, size_t, std::function <T *(size_t, size_t)>);)

	__avr_ignore(Matrix(const std::vector <T> &);)
	__avr_ignore(Matrix(const std::vector <Vector <T>> &);)
	__avr_ignore(Matrix(const std::vector <std::vector <T>> &);)
	__avr_ignore(Matrix(const std::initializer_list <Vector <T>> &);)
	__avr_ignore(Matrix(const std::initializer_list <std::initializer_list <T>> &);)

	__cuda_dual_prefix
	Matrix(size_t, size_t, T *, bool = true);

	/* template <class A>
	Matrix(A); */
	
	T norm() const;

	void resize(size_t, size_t);

	psize_t get_dimensions() const;

	Matrix slice(const psize_t &, const psize_t &) const;

	void set(size_t, size_t, T);

	const T &get(size_t, size_t) const;

	Vector <T> get_column(size_t) const;

	// Rading from a binary file (TODO: unignore later)
	__avr_ignore(void write(std::ofstream &) const;)
	__avr_ignore(void read(std::ifstream &);)

	// Concatenating matrices
	Matrix append_above(const Matrix &);
	Matrix append_below(const Matrix &);

	Matrix append_left(const Matrix &);
	Matrix append_right(const Matrix &);

	void operator*=(const Matrix &);
	void operator/=(const Matrix &);

	// Row operations
	void add_rows(size_t, size_t, T);
	
	void swap_rows(size_t, size_t);

	void multiply_row(size_t, T);

	void pow(const T &);

	// Miscellanious opertions
	__avr_ignore(void randomize(std::function <T ()>);)
	
	__cuda_dual_prefix
	void row_shur(const Vector <T> &);
	
	__cuda_dual_prefix
	void stable_shur(const Matrix <T> &);

	__cuda_dual_prefix
	void stable_shur_relaxed(const Matrix <T> &);

	// Values
	T determinant() const;

	T minor(size_t, size_t) const;
	T minor(const psize_t &) const;

	T cofactor(size_t, size_t) const;
	T cofactor(const psize_t &) const;

	Matrix inverse() const;
	Matrix adjugate() const;
	Matrix cofactor() const;

	bool symmetric() const;

	__avr_ignore(std::string display() const;)
	
#ifndef __AVR

	template <class U>
	friend std::ostream &operator<<(std::ostream &, const Matrix <U> &);

#endif

	// Special matrix generation
	static Matrix identity(size_t);

	// Miscellaneous functions
	template <class U>
	friend Vector <U> apt_and_mult(const Matrix <U> &, const Vector <U> &); 
	
	template <class U>
	friend Vector <U> rmt_and_mult(const Matrix <U> &, const Vector <U> &);

	template <class U>
	friend Matrix <U> vvt_mult(const Vector <U> &, const Vector <U> &); 

	class dimension_mismatch {};
protected:
	// TODO: Looks ugly here
	T determinant(const Matrix &) const;
public:
	const Matrix &operator=(const Matrix &);

	T *operator[](size_t);
	const T *operator[](size_t) const;

	size_t get_rows() const;
	size_t get_cols() const;
	
	Matrix transpose() const;

	void operator+=(const Matrix &);
	void operator-=(const Matrix &);
	
	void operator*=(const T &);
	void operator/=(const T &);

	// Matrix and matrix operations
	template <class U>
	friend Matrix <U> operator+(const Matrix <U> &, const Matrix <U> &);
	
	template <class U>
	friend Matrix <U> operator-(const Matrix <U> &, const Matrix <U> &);
	
	// template <class U>
	// friend Matrix <U> operator*(const Matrix <U> &, const Matrix <U> &);
	
	// Heterogenous multiplication
	template <class U, class V>
	friend Matrix <U> operator*(const Matrix <U> &, const Matrix <V> &);
	
	template <class U>
	friend Matrix <U> operator*(const Matrix <U> &, const U &);
	
	template <class U>
	friend Matrix <U> operator*(const U &, const Matrix <U> &);
	
	template <class U>
	friend Matrix <U> operator/(const Matrix <U> &, const U &);
	
	template <class U>
	friend Matrix <U> operator/(const U &, const Matrix <U> &);

	template <class U>
	friend bool operator==(const Matrix <U> &, const Matrix <U> &);
	
	// Miscellaneous operations
	template <class U>
	friend Matrix <U> shur(const Matrix <U> &, const Matrix <U> &);
	
	template <class U>
	friend Matrix <U> inv_shur(const Matrix <U> &, const Matrix <U> &);

	template <class A, class B, class C>
	friend Matrix <A> fma(const Matrix <A> &, const Matrix <B> &, const Matrix <C> &);

	template <class A, class B, class C>
	friend Matrix <A> fmak(const Matrix <A> &, const Matrix <B> &, const Matrix <C> &, A, A);
};

// TODO use __CUDACC__ instead of __zhp_cuda and make _cuda files
#include <primitives/matrix_prims.hpp>

#ifndef __AVR

#include <matrix_cpu.hpp>

#endif

}

#endif
