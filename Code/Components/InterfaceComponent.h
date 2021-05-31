#pragma once

#include <CryEntitySystem/IEntitySystem.h>
#include <Interfaces/IInterfaceBase.h>
#include <unordered_map>

class CInterfaceComponent final : public IEntityComponent
{
public:
	CInterfaceComponent() = default;
	virtual ~CInterfaceComponent() = default;

	template<class InterfaceType>
	InterfaceType* GetInterface()
	{
		std::unordered_map<const char*, IInterfaceBase*>::iterator it = m_interfaceMap.find(typeid(InterfaceType).name());
		if (it != m_interfaceMap.end())
		{
			return static_cast<InterfaceType*>(it->second);
		}
		return nullptr;
	}

	template<class InterfaceType>
	bool AddInterface(InterfaceType* interfaceRef)
	{
		if (IInterfaceBase* pBase = static_cast<IInterfaceBase*>(interfaceRef))
		{
			m_interfaceMap.insert({ typeid(InterfaceType).name(), pBase });
			return true;
		};
		return false;
	}

	template<class InterfaceType>
	void RemoveInterface()
	{
		std::unordered_map<const char*, IInterfaceBase*>::iterator it = m_interfaceMap.find(typeid(InterfaceType).name());
		if (it != m_interfaceMap.end())
		{
			m_interfaceMap.erase(it);
		}
	}

	static void ReflectType(Schematyc::CTypeDesc<CInterfaceComponent>& desc)
	{
		desc.SetGUID("{7F8BBB39-46D3-4D32-9868-918C22638962}"_cry_guid);
		desc.SetComponentFlags({ IEntityComponent::EFlags::Singleton, IEntityComponent::EFlags::HiddenFromUser });
	}

protected:
	virtual Cry::Entity::EventFlags GetEventMask() const override;
	virtual void ProcessEvent(const SEntityEvent& event) override;

private:
	std::unordered_map<const char*, IInterfaceBase*> m_interfaceMap;
};