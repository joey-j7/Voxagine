#pragma once
#include <functional>
#include <vector>
#include <algorithm>

template<class ...EventArgs>
class Event
{
public:
	struct Subscriber
	{
		Subscriber(std::function<void(EventArgs...)> _function, void* _pClass)
		{
			std::hash<void*> hasher;
			Function = _function;
			ID = static_cast<size_t>(hasher(_pClass));
		}

		std::function<void(EventArgs...)> Function;
		size_t ID;
	};

	void operator += (Subscriber sub)
	{
		m_Subscribers.push_back(sub);
	}

	void operator -= (void* _pClass)
	{
		size_t subID = m_Hasher(_pClass);
		typename std::vector<Subscriber>::iterator iter = m_Subscribers.begin();
		for (; iter != m_Subscribers.end(); ++iter)
		{
			if (iter->ID == subID)
			{
				m_Subscribers.erase(iter);
				break;
			}
		}
	}

	void operator()(EventArgs... e)
	{
		for (Subscriber& sub : std::vector<Subscriber>(m_Subscribers))
			sub.Function(e...);
	}

private:
	std::vector<Subscriber> m_Subscribers;
	std::hash<void*> m_Hasher;
};