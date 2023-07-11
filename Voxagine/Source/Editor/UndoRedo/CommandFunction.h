#pragma once

#include "Editor/UndoRedo/CommandFunctionWrapper.h"

#include <functional>
#include <tuple>

template<class ...FuncArgs>
class CommandFunction : public CommandFunctionWrapper
{
public:
	CommandFunction(std::function<void(FuncArgs...)> function, FuncArgs... values);
	virtual ~CommandFunction();

	virtual void ExecuteFunction();

private:
	template<typename F, typename Tuple, size_t ...S >
	void ApplyTupleImplementation(F&& rFunction, Tuple&& tuple, std::index_sequence<S...>);

	template<typename F, typename Tuple>
	void ApplyFromTuple(F&& rFunction, Tuple&& tuple);

private:
	std::function<void(FuncArgs...)> m_Function;
	std::tuple<FuncArgs...> m_FunctionValues;
};

#include "Editor/UndoRedo/CommandFunction.inl"