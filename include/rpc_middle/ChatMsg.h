#pragma once
#include<cstdlib>
#include<cassert>
#include<cstring>
class ChatMsg
{
	enum { header_length = 4 };
	enum { max_body_length = 512 };
public:
	void setMessage(const void *buffer, size_t bufferSize) {
		assert(bufferSize <= max_body_length);
		int buffersize = bufferSize;
		std::memcpy(data_ + 4, buffer, bufferSize);
		std::memcpy(data_, &buffersize, 4);
	}
private:
	char data_[header_length + max_body_length];
};

