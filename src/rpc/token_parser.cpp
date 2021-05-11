#include "token_parser.h"

rpc::token_parser::token_parser(const std::string & str):
	ss_(str),
	ia_(ss_),
	param_size_ (0)
{

}

bool rpc::token_parser::empty() const
{
	return true;
}

std::size_t rpc::token_parser::param_size()
{
	return param_size_;
}
