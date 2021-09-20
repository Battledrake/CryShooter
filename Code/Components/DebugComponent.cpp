#include "StdAfx.h"
#include "DebugComponent.h"

#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <CrySchematyc/Env/Elements/EnvComponent.h>
#include <CryCore/StaticInstanceList.h>
#include <CryPhysics/physinterface.h>

#include <DefaultComponents/Geometry/AdvancedAnimationComponent.h>

namespace
{
	static void RegisterDebugComponent(Schematyc::IEnvRegistrar& registrar)
	{
		Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
		{
			Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CDebugComponent));
		}
	}
	CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterDebugComponent)
}

void CDebugComponent::Initialize()
{
}

IEntityComponent::ComponentEventPriority CDebugComponent::GetEventPriority() const
{
	return 15;
}

Cry::Entity::EventFlags CDebugComponent::GetEventMask() const
{
	return
		Cry::Entity::EEvent::Initialize |
		Cry::Entity::EEvent::GameplayStarted |
		Cry::Entity::EEvent::PhysicsCollision;
}

void CDebugComponent::ProcessEvent(const SEntityEvent& event)
{
	switch (event.event)
	{
		case Cry::Entity::EEvent::Initialize:
			break;
		case Cry::Entity::EEvent::GameplayStarted:
		{
// 			pe_params_collision_class params;
// 			params.collisionClassOR.type = static_cast<uint32>(m_collisionType);
// 			params.collisionClassOR.ignore = static_cast<uint32>(m_collisionType);
// 			m_pEntity->GetPhysicalEntity()->SetParams(&params);
		}
		break;
		case Cry::Entity::EEvent::PhysicsCollision:
			if (EventPhysCollision* physCollision = reinterpret_cast<EventPhysCollision*>(event.nParam[0]))
			{
			}
			break;
		default:
			break;
	}
}