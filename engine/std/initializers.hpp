#ifndef STD_INITIALIZERS_H_
#define STD_INITIALIZERS_H_

// C++ headers
#include <cstdlib>
#include <random>

namespace zhetapi {

namespace ml {

template <class T>
struct RandomInitializer {
	// Use interval later
	T operator()() {
		return T (0.5 - rand()/((double) RAND_MAX));
	}
};

template <class T>
struct LeCun {
	std::default_random_engine		__gen;
	std::normal_distribution <double>	__dbt;	
public:
	explicit LeCun(size_t fan_in) : __gen(),
			__dbt(0, sqrt(T(1) / fan_in)) {}
	
	T operator()() {
		return __dbt(__gen);
	}
};

template <class T>
struct He {
	std::default_random_engine		__gen;
	std::normal_distribution <double>	__dbt;	
public:
	explicit He(size_t fan_in) : __gen(),
			__dbt(0, sqrt(T(2) / fan_in)) {}
	
	T operator()() {
		return __dbt(__gen);
	}
}; 

template <class T>
struct Xavier {
	std::default_random_engine		__gen;
	std::normal_distribution <double>	__dbt;	
public:
	explicit Xavier(size_t fan_avg) : __gen(),
			__dbt(0, sqrt(T(1) / fan_avg)) {}
	
	T operator()() {
		return __dbt(__gen);
	}
}; 

}

}

#endif
