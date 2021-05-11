#pragma once
#include <boost/noncopyable.hpp>
#include <utility>
#include <string>
#include <map>
#include <mutex>

#include "function_traits.hpp"
#include "token_parser.h"
#include "common.h"
#include "utility.hpp"


namespace rpc
{
	//���������
	class invoker_function {
	private:
		std::function<void(token_parser &, std::string& result)> function_;
		std::size_t param_size_; //������С
	public:
		invoker_function() = default;
		invoker_function(const std::function<void(token_parser &, std::string&)>& function, std::size_t size);
		void operator()(token_parser &parser, std::string& result);
		const std::size_t param_size() const;
	};

	class router : boost::noncopyable
	{
	private:
		std::map<std::string, invoker_function> map_invokers_;//string��Ӧ���õ���
		std::mutex mtx_;
	public:
		static router& get();

		template<typename Function>
		void register_handler(std::string const & name, const Function& f);

		template<typename Function, typename Self>
		void register_handler(std::string const & name, const Function& f, Self* self);

		void remove_handler(std::string const& name);

		void route(const std::string& text, std::size_t length, const std::function<void(const char*)>& callback = nullptr);
		void route2(const std::string& text, std::size_t length, const std::function<void( std::string)>& callback = nullptr);

		router() = default;
	private:
		router(const router&) = delete;
		router(router&&) = delete;

		template<typename F, size_t... I, typename ... Args>
		static auto call_helper(const F& f, const std::index_sequence<I...>&, const std::tuple<Args...>& tup);

		template<typename F, typename ... Args>
		static typename std::enable_if<std::is_void<typename std::result_of<F(Args...)>::type>::value>::type
			call(const F& f, std::string&result, const std::tuple<Args...>& tp);
	
		template<typename F, typename ... Args>
		static typename std::enable_if<!std::is_void<typename std::result_of<F(Args...)>::type>::value>::type
			call(const F& f, std::string& result, const std::tuple<Args...>& tp);

		template<typename F, typename Self, size_t... Indexes, typename ... Args>
		static auto call_member_helper(const F& f, Self* self, const std::index_sequence<Indexes...>&, const std::tuple<Args...>& tup);

		template<typename F, typename Self, typename ... Args>
		static typename std::enable_if<std::is_void<typename std::result_of<F(Self, Args...)>::type>::value>::type
			call_member(const F& f, Self* self, std::string&, const std::tuple<Args...>& tp);

		template<typename F, typename Self, typename ... Args>
		static typename std::enable_if<!std::is_void<typename std::result_of<F(Self, Args...)>::type>::value>::type
			call_member(const F& f, Self* self, std::string& result, const std::tuple<Args...>& tp);

		template<typename Function, size_t N = 0, size_t M = function_traits<Function>::arity>
		struct invoker
		{
			template<typename Args>
			static void apply(const Function& func, token_parser & parser, std::string& result, Args const & args);

			template<typename Args, typename Self>
			static void apply_member(Function func, Self* self, token_parser & parser, std::string& result, const Args& args);
		};


		template<typename Function, size_t M>
		struct invoker<Function, M, M>
		{
			template<typename Args>
			static void apply(const Function& func, token_parser &, std::string& result, Args const & args);

			template<typename Args, typename Self>
			static void apply_member(const Function& func, Self* self, token_parser &parser, std::string& result, const Args& args);

		};

		//��ע���handler������map��
		template<typename Function>
		void register_nonmember_func(std::string const & name, const Function& f);

		template<typename Function, typename Self>
		void register_member_func(const std::string& name, const Function& f, Self* self);
	};


	template<typename Function>
	inline void router::register_handler(std::string const & name, const Function & f)
	{
		return register_nonmember_func(name, f);
	}

	template<typename Function, typename Self>
	inline void router::register_handler(std::string const & name, const Function & f, Self * self)
	{
		return register_member_func(name, f, self);
	}

	template<typename F, size_t ...I, typename ...Args>
	inline auto router::call_helper(const F & f, const std::index_sequence<I...>&, const std::tuple<Args...>& tup)
	{
		return f(std::get<I>(tup)...);
	}

	template<typename F, typename ...Args>
	static typename std::enable_if<std::is_void<typename std::result_of<F(Args...)>::type>::value>::type router::call(const F & f, std::string & result, const std::tuple<Args...>& tp)
	{
		call_helper(f, std::make_index_sequence<sizeof... (Args)>{}, tp);
		result =  rpc::get_serial(result_code::OK, 0);
	}

	template<typename F, typename ...Args>
	static typename std::enable_if<!std::is_void<typename std::result_of<F(Args...)>::type>::value>::type router::call(const F & f, std::string &result, const std::tuple<Args...>& tp)
	{
		auto r = call_helper(f, std::make_index_sequence<sizeof... (Args)>{}, tp);
		result =  rpc::get_serial(result_code::OK, r);
	}

	
	template<typename F, typename Self, size_t ...Indexes, typename ...Args>
	inline auto router::call_member_helper(const F & f, Self * self, const std::index_sequence<Indexes...>&, const std::tuple<Args...>& tup)
	{
		return (*self.*f)(std::get<Indexes>(tup)...);
	}
	
	template<typename F, typename Self, typename ...Args>
	inline typename std::enable_if<std::is_void<typename std::result_of<F(Self, Args...)>::type>::value>::type router::call_member(const F & f, Self * self, std::string & result, const std::tuple<Args...>& tp)
	{
		call_member_helper(f, self, typename std::make_index_sequence<sizeof... (Args)>{}, tp);
		get_serial(result_code::OK, 0);
	}

	template<typename F, typename Self, typename ...Args>
	inline typename std::enable_if<!std::is_void<typename std::result_of<F(Self, Args...)>::type>::value>::type router::call_member(const F & f, Self * self, std::string & result, const std::tuple<Args...>& tp)
	{
		auto r = call_member_helper(f, self, typename std::make_index_sequence<sizeof... (Args)>{}, tp);
		result = get_serial(result_code::OK, r);
	}
	
	template<typename Function>
	inline void router::register_nonmember_func(std::string const & name, const Function & f)
	{
		this->map_invokers_[name] = { std::bind(&invoker<Function>::template apply<std::tuple<>>, f,
			std::placeholders::_1, std::placeholders::_2, std::tuple<>()),
			function_traits<Function>::arity };
	}

	
	template<typename Function, typename Self>
	inline void router::register_member_func(const std::string & name, const Function & f, Self * self)
	{
		this->map_invokers_[name] = { std::bind(&invoker<Function>::template apply_member<std::tuple<>, Self>, f, self, std::placeholders::_1,
			std::placeholders::_2, std::tuple<>()), function_traits<Function>::arity };
	}
	

	template<typename Function, size_t M>
	template<typename Args>
	inline void  router::invoker<Function, M, M>::apply(const Function& func, token_parser &, std::string& result, Args const & args)
	{
		call(func, result, args);
	}

	
	template<typename Function, size_t M>
	template<typename Args, typename Self>
	inline void router::invoker<Function, M, M>::apply_member(const Function & func, Self * self, token_parser & parser, std::string & result, const Args & args)
	{
		call_member(func, self, result, args);
	}
	
	template<typename Function, size_t N, size_t M>
	template<typename Args, typename Self>
	inline void router::invoker<Function, N, M>::apply_member(Function func, Self * self, token_parser & parser, std::string & result, const Args & args)
	{
		typedef typename function_traits<Function>::template args<N>::type arg_type;

		try
		{
			invoker<Function, N + 1, M>::apply_member(func, self, parser, result, std::tuple_cat(args, std::make_tuple(parser.get<arg_type>())));
		}
		catch (const std::exception& e)
		{
			result = get_serial(result_code::EXCEPTION, (std::string)e.what());
		}
	}
	
	template<typename Function, size_t N, size_t M>
	template<typename Args>
	inline void router::invoker<Function, N, M>::apply(const Function & func, token_parser & parser, std::string & result, Args const & args)
	{
		typedef typename function_traits<Function>::template args<N>::type arg_type;
		try
		{
			invoker<Function, N + 1, M>::apply(func, parser, result,
				std::tuple_cat(args, std::make_tuple(parser.get<arg_type>())));
		}
		catch (std::exception& e)
		{
			result = get_serial(result_code::EXCEPTION, (std::string)e.what());
		}
		
	}

}


