#pragma once

#include <iostream>
#include <chrono>
#include <ctime>
#include <assert.h>
#include <string>

class SimpleTimer
{
public:

	SimpleTimer() = default;

	void Start(std::string name = "")
	{
		assert(m_name.empty() && "Did you forget to end the last timer?");
		m_name = name;
		std::cout << std::endl << "Timer '" << name << "' started." << std::endl;
		m_start = std::chrono::system_clock::now();
	};

	void End()
	{
		assert(!m_name.empty() && "Did you forget to start a timer?");
		auto end = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed_seconds = end - m_start;
		std::cout << "Timer '" << m_name << "' ended,\nelapsed time: " << elapsed_seconds.count() << "s" << std::endl;
		m_name.clear();
	};

private:

	std::chrono::system_clock::time_point m_start;
	std::string m_name;
};
