#pragma once

extern "C" {
	#ifdef USE_MCBSP
		#include <mcbsp.h>	// multi core bsp
	#else
		#include "bsp.h"	// bsp on cartesius
		#define MCBSP_PROCESSOR_INDEX_DATATYPE int
		#define MCBSP_BYTESIZE_TYPE int
	#endif
}

namespace bsp {
	typedef MCBSP_PROCESSOR_INDEX_DATATYPE processor_index;
	typedef MCBSP_BYTESIZE_TYPE size_type;

	inline void     begin(processor_index P) { bsp_begin(P); }
	inline void     end()                    { bsp_end(); }
	inline void     init(void (*spmd)(), int argc, char ** argv){ bsp_init(spmd, argc, argv); }
	inline processor_index nprocs()          { return bsp_nprocs(); }
	inline processor_index pid()             { return bsp_pid(); }
	inline double   time()                   { return bsp_time(); }
	inline void     sync()                   { bsp_sync(); }

	// As we know the type, we don't bother the programmer with bytes and sizeof. Instead simply use the number of elements.
	template <typename T>
	void push_reg(T* value, size_type number = 1)
	{ bsp_push_reg(static_cast<void*>(value), number * sizeof(T)); }

	template <typename T>
	void pop_reg(T* value)
	{ bsp_pop_reg(static_cast<void*>(value)); }

	// The offset too is in terms of number of elements, not bytes.
	template <typename T>
	void put(processor_index p, T const * source, T * destination, size_type offset = 0, size_type number = 1)
	{ bsp_put(p, static_cast<void const *>(source), static_cast<void*>(destination), offset * sizeof(T), number * sizeof(T)); }

	template <typename T>
	void get(processor_index p, T const * source, size_type offset, T * destination, size_type number = 1)
	{ bsp_get(p, static_cast<void const *>(source), offset, static_cast<void*>(destination), number * sizeof(T)); }
}
