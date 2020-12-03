#ifndef NETWORK_H_
#define NETWORK_H_

// C/C++ headers
#include <cstddef>
#include <vector>
#include <functional>
#include <memory>

// Engine headers
#include <activation.hpp>
#include <optimizer.hpp>
#include <vector.hpp>
#include <matrix.hpp>

namespace zhetapi {
		
	namespace ml {

		/*
		* Nerual Network
		*
		* Note that layers contain pointers to activations. These are not
		* allocated and are simply COPIES of what the USER allocates. The user
		* can expect that the resources they allocated will not be destroyed
		* within this class, and that they can use the resources afterwards. In
		* other words, pointer data is READ ONLY.
		*
		* @tparam T is the type with which calculations are performed
		* @tparam U is the type of activation parameter scheme, ie. unary or
		* binary
		*/
		template <class T>
		class NeuralNetwork {
		public:
			typedef std::pair <std::size_t, Activation <T> *> Layer;
		private:
			std::vector <Layer>		__layers;
			std::vector <Matrix <T>>	__weights;
			std::vector <Matrix <T>>	__momentum;
			std::function <T ()>		__random;
			std::size_t			__isize;
			std::size_t			__osize;

			std::vector <Vector <T>>	__xs;
			std::vector <Vector <T>>	__dxs;
		public:
			NeuralNetwork(const std::vector <Layer> &, const std::function <T ()> &);

			~NeuralNetwork();

			Vector <T> compute(const Vector <T> &);
			Vector <T> compute(const Vector <T> &, const std::vector <Matrix <T>> &);

			Vector <T> operator()(const Vector <T> &);

			void apply_gradient(const std::vector <Matrix <T>> &, T, T);

			std::vector <Matrix <T>> gradient(const Vector <T> &,
					const Vector <T> &, Optimizer <T> *);

			void learn(const Vector <T> &, const Vector <T> &,
					Optimizer <T> *, T);

			std::pair <size_t, T> train(size_t, T,
					Optimizer <T> *,
					const std::vector <Vector<T>> &,
					const std::vector <Vector <T>> &,
					const std::function <bool (const Vector <T> , const Vector <T>)> &,
					bool = false);
			void epochs(size_t, size_t, T,
					Optimizer <T> *,
					const std::vector <Vector <T>> &,
					const std::vector <Vector <T>> &,
					const std::function <bool (const Vector <T> , const Vector <T>)> &,
					bool = false);

			void randomize();

			// Printing weights
			void print() const;
		};

		/*
		 * NOTE: The pointers allocated and passed into this function
		 * should be left alone. They will be deallocated once the scope
		 * of the network object comes to its end. In other words, DO
		 * NOT FREE ACTIVATION POINTERS, and instead let the
		 * NeuralNetwork class do the work for you.
		 */
		template <class T>
		NeuralNetwork <T> ::NeuralNetwork(const std::vector <Layer> &layers,
				const std::function <T ()> &random) : __random(random),
				__isize(layers[0].first), __osize(layers[layers.size() - 1].first),
				__layers(layers)
		{
			size_t size = __layers.size();

			for (size_t i = 0; i < size - 1; i++) {
				// Add extra column for constants (biases)
				// std::cout << "Before" << std::endl;

				Matrix <T> mat(__layers[i + 1].first, __layers[i].first + 1);

				/* std::cout << "mat: " << mat << std::endl;
				std::cout << "\trows: " << mat.get_rows() << std::endl;
				std::cout << "\tcols: " << mat.get_cols() << std::endl; */

				__weights.push_back(mat);
				__momentum.push_back(mat);
			}
		}

		template <class T>
		NeuralNetwork <T> ::~NeuralNetwork()
		{
			for (auto layer : __layers)
				delete layer.second;
		}

		template <class T>
		Vector <T> NeuralNetwork <T> ::compute(const Vector <T> &in)
		{
			assert(in.size() == __isize);

			Vector <T> prv = in;
			Vector <T> tmp = (*__layers[0].second)(prv);

			__xs.clear();
			__dxs.clear();

			for (size_t i = 0; i < __weights.size(); i++) {
				__xs.insert(__xs.begin(), tmp.append_above(T (1)));

				prv = __weights[i] * Matrix <T> (tmp.append_above(T (1)));
				tmp = (*__layers[i + 1].second)(prv);

				Activation <T> *act = __layers[i].second->derivative();
				
				__dxs.insert(__dxs.begin(), (*act)(prv));

				delete act;
			}
			
			return tmp;
		}

		template <class T>
		Vector <T> NeuralNetwork <T> ::compute(const Vector <T> &in, const std::vector <Matrix <T>> &weights)
		{
			assert(in.size() == __isize);

			Vector <T> prv = in;
			Vector <T> tmp = (*__layers[0].second)(prv);

			for (size_t i = 0; i < __weights.size(); i++) {
				prv = weights[i] * Matrix <T> (tmp.append_above(T (1)));
				tmp = (*__layers[i + 1].second)(prv);				
			}
			
			return tmp;
		}

		template <class T>
		Vector <T> NeuralNetwork <T> ::operator()(const Vector <T> &in)
		{
			assert(in.size() == __isize);

			Vector <T> prv = in;
			Vector <T> tmp = (*__layers[0].second)(prv);

			__xs.clear();
			__dxs.clear();

			for (size_t i = 0; i < __weights.size(); i++) {
				__xs.insert(__xs.begin(), tmp.append_above(T (1)));

				prv = __weights[i] * Matrix <T> (tmp.append_above(T (1)));
				tmp = (*__layers[i + 1].second)(prv);

				Activation <T> *act = __layers[i].second->derivative();
				
				__dxs.insert(__dxs.begin(), (*act)(prv));

				delete act;
			}
			
			return tmp;
		}

		template <class T>
		void NeuralNetwork <T> ::apply_gradient(const std::vector <Matrix <T>> &grad,
				T alpha, T mu)
		{
			assert(__weights.size() == grad.size());
			for (int i = 0; i < __weights.size(); i++) {
				__momentum[i] = mu * __momentum[i] - alpha * grad[i];
				__weights[i] += __momentum[i];
			}
		}
		
		template <class T>
		std::vector <Matrix <T>> NeuralNetwork <T> ::gradient(const Vector <T> &in,
				const Vector <T> &out, Optimizer <T> *opt)
		{
			assert(in.size() == __isize);
			assert(out.size() == __osize);
			
			Vector <T> actual = (*this)(in);

			Optimizer <T> *dopt = opt->derivative();

			Vector <T> delta = (*dopt)(out, actual);

			std::vector <Matrix <T>> changes;

			int n = __weights.size();
			for (int i = n - 1; i >= 0; i--) {
				if (i != n - 1) {
					delta = __weights[i + 1].transpose() * delta;

					delta = delta.remove_top();
				}

				delta = shur(delta, __dxs[(n - 1) - i]);

				Matrix <T> xt = __xs[n - i - 1].transpose();

				changes.push_back(delta * xt);
			}

			std::reverse(changes.begin(), changes.end());

			// Free resources
			delete dopt;

			using namespace std;

			for (auto Ji : changes) {
				cout << "\n--------------------------" << endl;
				cout << "Ji: " << Ji << endl;
			}
			
			std::vector <Vector <T>> params;
			for (auto weight : __weights) {
				for (int i = 0; i < weight.get_cols(); i++)
					params.push_back(weight.get_column(i));
			}

			cout << "Size of params: " << params.size() << endl;
			for (auto col : params)
				cout << "Col: " << col << endl;

			return changes;
		}

		template <class T>
		void NeuralNetwork <T> ::learn(const Vector <T> &in, const Vector <T> &out, Optimizer <T> *opt, T alpha)
		{
			/* using namespace std;

			cout << "================================" << endl;
			
			cout << "Xs:" << endl;
			for (auto elem : __xs)
				cout << "X:\t" << elem << endl;
			
			cout << "Dxs:" << endl;
			for (auto elem : __dxs)
				cout << "Dx:\t" << elem << endl; */
			
			assert(in.size() == __isize);
			assert(out.size() == __osize);
			
			Vector <T> actual = (*this)(in);

			Vector <T> delta = (*(opt->derivative()))(out, actual);

			std::vector <Matrix <T>> changes;

			int n = __weights.size();
			for (int i = n - 1; i >= 0; i--) {
				/* cout << "=========================" << endl;

				cout << "weight[" << i << "]" << endl;
				print_dims(__weights[i]); */

				if (i != n - 1) {
					/* cout << "weight^T:" << endl;
					print_dims(__weights[i + 1].transpose()); */

					delta = __weights[i + 1].transpose() * delta;

					delta = delta.remove_top();

					/* cout << "delta: " << endl;
					print_dims(delta);

					Vector <T> tmp(__weights[i + 1].transpose() * delta);

					cout << "tmp alias:" << endl;
					print_dims(__weights[i + 1].transpose() * delta);
					
					cout << "pre tmp: " << endl;
					print_dims(tmp);

					tmp = tmp.remove_top();

					cout << "post tmp: " << endl;
					print_dims(tmp); */
					// delta = __weights[i + 1].transpose() * delta;
				}

				/* cout << "delta: " << endl;
				print_dims(delta);

				cout << "__dxs[(n - 1) - i]: " << endl;
				print_dims(__dxs[(n - 1) - i]); */

				delta = shur(delta, __dxs[(n - 1) - i]);

				Matrix <T> xt = __xs[n - i - 1].transpose();

				/* cout << "xt: " << endl;
				print_dims(xt);

				cout << "delta * xt:" << endl;
				print_dims(delta * xt); */

				changes.push_back(delta * xt);
			}

			// cout << "===P2: APPLY===" << endl;
			for (size_t i = 0; i < n; i++) {
				/* cout << "weights: " << endl;
				print_dims(__weights[n - (i + 1)]);

				cout << "changes: " << endl;
				print_dims(changes[i]); */

				__weights[n - (i + 1)] -= alpha * changes[i];
			}
		}

		template <class T>
		std::pair <size_t, T> NeuralNetwork <T> ::train(size_t id, T alpha,
				Optimizer <T> *opt,
				const std::vector <Vector<T>> &ins,
				const std::vector <Vector<T>> &outs,
				const std::function <bool (const Vector <T>, const Vector <T>)> &crit,
				bool print)
		{
			assert(ins.size() == outs.size());
			
			using namespace std;

			const int len = 15;
			
			if (print)
				cout << "Batch #" << id << " (" << ins.size() << " samples)\t[";

			int passed = 0;
			int bars = 0;

			double opt_error = 0;
			double per_error = 0;

			std::vector <std::vector <Matrix <T>>> grads;

			int size = ins.size();
			for (int i = 0; i < size; i++) {
				Vector <T> actual = compute(ins[i]);

				if (crit(actual, outs[i]))
					passed++;

				grads.push_back(gradient(ins[i], outs[i], opt));
				
				opt_error += (*opt)(outs[i], actual)[0];
				per_error += 100 * (actual - outs[i]).norm()/outs[i].norm();

				if (print) {
					int delta = (len * (i + 1))/size;
					for (int i = 0; i < delta - bars; i++) {
						cout << "=";
						cout.flush();
					}

					bars = delta;
				}
			}

			std::vector <Matrix <T>> grad = grads[0];
			for (size_t i = 1; i < grads.size(); i++) {
				for (size_t j = 0; j < grad.size(); j++)
					grad[j] += grads[i][j];
			}
				
			for (size_t j = 0; j < grad.size(); j++)
				grad[j] /= (double) size;
			
			apply_gradient(grad, alpha, 0.7);

			if (print) {
				cout << "]\tpassed:\t" << passed << "/" << size << " ("
					<< 100 * ((double) passed)/size << "%)\t"
					<< "average error:\t" << per_error/size << "%"
					<< endl;
			}

			return std::make_pair(passed, opt_error);
		}

		template <class T>
		void NeuralNetwork <T> ::epochs(size_t runs, size_t batch, T alpha,
				Optimizer <T> *opt,
				const std::vector <Vector<T>> &ins,
				const std::vector <Vector<T>> &outs, const
				std::function<bool (const Vector <T>, const Vector <T>)> &crit,
				bool print)
		{
			assert(ins.size() == outs.size());

			using namespace std;

			std::vector <std::vector <Vector <T>>> ins_batched;
			std::vector <std::vector <Vector <T>>> outs_batched;

			size_t batches = 0;

			std::vector <Vector <T>> in_batch;
			std::vector <Vector <T>> out_batch;
			for (int i = 0; i < ins.size(); i++) {
				in_batch.push_back(ins[i]);
				out_batch.push_back(outs[i]);

				if (i % batch == batch - 1 || i == ins.size() - 1) {
					ins_batched.push_back(in_batch);
					outs_batched.push_back(out_batch);

					in_batch.clear();
					out_batch.clear();
				}
			}

			T perr = 0;
			T lr = alpha;

			T thresh = 0.1;

			size_t passed;
			for (int i = 0; i < runs; i++) {
				if (print) {
					::std::cout << ::std::string(20, '-') << ::std::endl;
					::std::cout << "Epoch #" << (i + 1) << " (" << lr << ")\n" << ::std::endl;
				}

				T err;

				passed = 0;
				for (int i = 0; i < ins_batched.size(); i++) {
					auto result = train(i + 1, lr, opt, ins_batched[i], outs_batched[i], crit, print);

					passed += result.first;

					err += result.second;
				}

				cout << "\nTotal Error: " << err - perr << endl;

				perr = err;

				if (fabs(err - perr) < thresh) {
					lr *= 0.99;

					if (thresh > 0.00001)
						thresh /= 10;
				} else if (fabs(err - perr) > 2 * thresh) {
					lr /= 0.93;

					if (thresh < 0.001)
						thresh *= 10;
				}
				
				if (print) {
					cout << "\nCases passed:\t" << passed << "/" << ins.size() << " (" << 100 * ((double) passed)/ins.size() << "%)" << endl;
				}
			}
		}

		template <class T>
		void NeuralNetwork <T> ::randomize()
		{
			for (auto &mat : __weights)
				mat.randomize(__random);
		}

		template <class T>
		void NeuralNetwork <T> ::print() const
		{
			std::cout << "================================" << std::endl;
			
			std::cout << "Weights:" << std::endl;

			size_t n = 0;
			for (auto mat : __weights)
				std::cout << "[" << ++n << "]\t" << mat << std::endl;
			
			std::cout << "================================" << std::endl;
		}

	}

}

#endif
