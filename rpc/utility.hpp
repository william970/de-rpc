#pragma once
#include "common.h"
#include <boost/archive/text_oarchive.hpp>

namespace rpc
{
	template<typename T>
	static inline std::string make_serial(result_code t1, T t2)
	{
		std::stringstream ss;
		boost::archive::text_oarchive oa(ss);
		encode(t1, ss, oa);
		encode(t2, ss, oa);
		return ss.str();
	}

	template<typename T>
	inline void encode(T t, std::stringstream & ss, boost::archive::text_oarchive & oa)
	{
		oa << t;
	}

	template<typename T>
	std::string get_serial(result_code code, const T & r)
	{
		std::string buf;

		buf = make_serial(code, r);
		return buf;
	}
}
