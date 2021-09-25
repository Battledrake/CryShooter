#include "StdAfx.h"
#include "BodyDamageComponent.h"

#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <CrySchematyc/Env/Elements/EnvComponent.h>
#include <CryCore/StaticInstanceList.h>

#include <DefaultComponents/Geometry/AdvancedAnimationComponent.h>
#include <CryRenderer/IRenderAuxGeom.h>

#include <Components/HealthComponent.h>

namespace
{
	static void RegisterBodyDamageComponent(Schematyc::IEnvRegistrar& registrar)
	{
		Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
		{
			Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CBodyDamageComponent));
			{
			}
		}
	}
	CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterBodyDamageComponent)
}

bool CBodyDamageComponent::SBodyPartsDefinition::Serialize(Serialization::IArchive& ar)
{
	ar.openBlock("PartDefinition", "Part Definition");
	ar(m_jointName, "jointname", "Joint Name");
	ar(m_damageMultiplier, "damagemultiplier", "Damage Multiplier");
	ar.closeBlock();

	return true;
}

void CBodyDamageComponent::Initialize()
{
	m_pHealthComp = m_pEntity->GetComponent<CHealthComponent>();
}

Cry::Entity::EventFlags CBodyDamageComponent::GetEventMask() const
{
	return
		Cry::Entity::EEvent::GameplayStarted |
		Cry::Entity::EEvent::Reset;
}

void CBodyDamageComponent::ProcessEvent(const SEntityEvent& event)
{
	switch (event.event)
	{
		case Cry::Entity::EEvent::GameplayStarted:
			FillJointMap();
			break;
		case Cry::Entity::EEvent::Reset:
			m_jointIdMap.clear();
	}
}

void CBodyDamageComponent::FillJointMap()
{
	if (Cry::DefaultComponents::CAdvancedAnimationComponent* pAnimComp = m_pEntity->GetComponent<Cry::DefaultComponents::CAdvancedAnimationComponent>())
	{
		if (ICharacterInstance* pChar = pAnimComp->GetCharacter())
		{
			for (int i = 0; i < m_bodyPartsArray.Size(); ++i)
			{
				int jointId = pChar->GetIDefaultSkeleton().GetJointIDByName(m_bodyPartsArray.At(i).m_jointName.c_str());
				if (jointId == -1)
				{
					CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING,
						"Entity: %s - body damage component can not find joint: %s",
						m_pEntity->GetName(), m_bodyPartsArray.At(i).m_jointName.c_str());
				}
				if (m_jointIdMap.find(jointId) != m_jointIdMap.end())
				{
					CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING,
						"Entity: %s - body damage component has duplicated joint: %s.",
						m_pEntity->GetName(), m_bodyPartsArray.At(i).m_jointName.c_str());
				}

				m_jointIdMap[jointId] = m_bodyPartsArray.At(i);
			}
		}
	}
}

void CBodyDamageComponent::CalculateDamage(float damage, int jointId, Vec3 hitVelocity)
{
	float calcDmg = damage;
	string jointName = "other";
	auto jointIter = m_jointIdMap.find(jointId);
	if (jointIter != m_jointIdMap.end())
	{
		calcDmg *= jointIter->second.m_damageMultiplier;
		jointName = jointIter->second.m_jointName.c_str();
	}

 	if (m_pHealthComp)
 		m_pHealthComp->ApplyDamage(calcDmg, jointId, hitVelocity);
}
