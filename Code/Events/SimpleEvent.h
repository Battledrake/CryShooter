#pragma once

//Simple event supports a single listener, used mostly for UI events
template<typename...Args>
class SimpleEvent
{
public:
	void Invoke(Args...args)
	{
		if (m_function)
			m_function(args...);
	}

	void RegisterListener(std::function<void(Args...)> function)
	{
		m_function = std::move(function);
	}

	void RemoveListener()
	{
		m_function = nullptr;
	}

private:
	std::function<void(Args...)> m_function;
};