#include <operand.hpp>

#include <core/types.hpp>

// Macros
#define id_spec(type)					\
	template <>					\
	size_t Operand <type> ::id() const		\
	{						\
		return zhp_id <Operand <type>> ();	\
	}

namespace zhetapi {

// Specializations
template <>
std::string Operand <bool> ::dbg_str() const
{
	return (_val) ? "true" : "false";
}

// ID specs
id_spec(Z)
id_spec(Q)
id_spec(R)

id_spec(B)
id_spec(S)

id_spec(CmpZ)
id_spec(CmpQ)
id_spec(CmpR)

id_spec(VecZ)
id_spec(VecQ)
id_spec(VecR)

id_spec(VecCmpZ)
id_spec(VecCmpQ)
id_spec(VecCmpR)

id_spec(MatZ)
id_spec(MatQ)
id_spec(MatR)

id_spec(MatCmpZ)
id_spec(MatCmpQ)
id_spec(MatCmpR)

}