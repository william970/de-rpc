#pragma once
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <boost/archive/text_iarchive.hpp>

namespace rpc
{
	class token_parser
	{
	public:
		
		token_parser(const std::string & str);

		template<typename RequestedType>
		typename std::decay<RequestedType>::type get();

		bool empty() const;
		std::size_t param_size();
		
	private:
		token_parser(const token_parser&) = delete;
		token_parser(token_parser&&) = delete;
		
	private:
		std::stringstream ss_;
		boost::archive::text_iarchive ia_;
		int param_size_;
	};
	
	template<typename RequestedType>
	inline typename std::decay<RequestedType>::type token_parser::get()
	{
		std::string buf = ss_.str();
		if (buf.empty())
			throw std::invalid_argument("unexpected end of input");

		try
		{
			using result_type = std::decay<RequestedType>::type;
			result_type result;
			ia_ & result;
		
			return result;
		}
		catch (std::exception& e)
		{
			throw std::invalid_argument(std::string("invalid argument: ") + (std::string)e.what());
		}
	}

}