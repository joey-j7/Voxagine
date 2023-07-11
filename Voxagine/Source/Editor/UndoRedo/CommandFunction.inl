#include "CommandFunction.h"
#pragma once

template<class ...FuncArgs>
CommandFunction<FuncArgs...>::CommandFunction(std::function<void(FuncArgs...)> function, FuncArgs... values)
	: m_Function(function)
	, m_FunctionValues(values...)
{
}
template<class ...FuncArgs>
CommandFunction<FuncArgs...>::~CommandFunction()
{
}

template<class ...FuncArgs>
inline void CommandFunction<FuncArgs...>::ExecuteFunction()
{
	ApplyFromTuple(m_Function, m_FunctionValues);
};

template<class ...FuncArgs>
template<typename F, typename Tuple, size_t ...S>
void CommandFunction<FuncArgs...>::ApplyTupleImplementation(F && rFunction, Tuple && tuple, std::index_sequence<S...>)
{
	std::forward<F>(rFunction)(std::get<S>(std::forward<Tuple>(tuple))...);
}

template<class ...FuncArgs>
template<typename F, typename Tuple>
void CommandFunction<FuncArgs...>::ApplyFromTuple(F && rFunction, Tuple && tuple)
{
	std::size_t constexpr tSize = std::tuple_size<typename std::remove_reference<Tuple>::type>::value;
	ApplyTupleImplementation(std::forward<F>(rFunction), std::forward<Tuple>(tuple), std::make_index_sequence<tSize>());
}