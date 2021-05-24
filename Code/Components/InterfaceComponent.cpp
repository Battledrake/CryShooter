#include "StdAfx.h"
#include "InterfaceComponent.h"

#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <CrySchematyc/Env/Elements/EnvComponent.h>
#include <CryCore/StaticInstanceList.h>

namespace {
	static void RegisterInterfaceComponent(Schematyc::IEnvRegistrar& registrar) {
		Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID()); {
			Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CInterfaceComponent)); {
			}
		}
	}
	CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterInterfaceComponent)
}

Cry::Entity::EventFlags CInterfaceComponent::GetEventMask() const
{
	return
		Cry::Entity::EEvent::Reset;
}

void CInterfaceComponent::ProcessEvent(const SEntityEvent& event)
{
	switch (event.event)
	{
		case Cry::Entity::EEvent::Reset:
			m_interfaceMap.clear();
	}
}