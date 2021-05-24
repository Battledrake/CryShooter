#pragma once


//Simple event supports a single listener, used mostly for UI events
template<typename Signature, typename...Args>
class SimpleEvent
{
public:
	void Invoke(Args...args)
	{
		if (m_function)
			m_function(args...);
	}

	void RegisterListener(std::function<Signature(Args...)> function)
	{
		m_function = std::move(function);
	}

	void RemoveListener()
	{
		m_function = nullptr;
	}

private:
	std::function<Signature(Args...)> m_function;
};