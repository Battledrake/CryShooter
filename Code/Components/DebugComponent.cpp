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

inline bool SBodyDamageMultiplier::Serialize(Serialization::IArchive& archive)
{
	archive.openBlock("Body Parts", "Body Parts");
	archive(m_bodyPart, "bodypart", "BodyPart");
	archive(m_damageMultiplier, "damagemultiplier", "DamageMultiplier");
	archive.closeBlock();

	return true;
}

void CDebugComponent::Initialize()
{
	for (int i = 0; i < m_damageParts.Size(); ++i)
	{
// 		m_activeJointIds.emplace_back(m_pEntity->GetComponent<Cry::DefaultComponents::CAdvancedAnimationComponent>().GetCharacter()->GetIDefaultSkeleton().GetJointIDByName(m_damageParts.At(i).m_bodyPart));
	}
}

Cry::Entity::EventFlags CDebugComponent::GetEventMask() const
{
	return
		Cry::Entity::EEvent::Initialize |
		Cry::Entity::EEvent::PhysicsCollision;
}

void CDebugComponent::ProcessEvent(const SEntityEvent& event)
{
	switch (event.event)
	{
		case Cry::Entity::EEvent::Initialize:
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