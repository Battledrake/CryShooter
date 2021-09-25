#include "StdAfx.h"
#include "HealthComponent.h"

#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <CrySchematyc/Env/Elements/EnvComponent.h>
#include <CryCore/StaticInstanceList.h>

namespace {
	static void RegisterHealthComponent(Schematyc::IEnvRegistrar& registrar) {
		Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID()); {
			Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CHealthComponent)); {
			}
		}
	}
	CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterHealthComponent)
}

void CHealthComponent::Initialize()
{
}

Cry::Entity::EventFlags CHealthComponent::GetEventMask() const
{
	return Cry::Entity::EEvent::GameplayStarted |
		Cry::Entity::EEvent::Reset;
}

void CHealthComponent::ProcessEvent(const SEntityEvent& event)
{
	switch (event.event)
	{
		case Cry::Entity::EEvent::GameplayStarted:
		{
			m_currentHealth = m_startingHealth;
		}
		break;
		case Cry::Entity::EEvent::Reset:
		{
		}
		break;
	}
}

void CHealthComponent::ApplyDamage(float amount, int partId, Vec3 hitVelocity)
{
	if (m_currentHealth > 0)
		m_currentHealth -= (int)amount;
	else
		return;

	if (m_currentHealth <= 0)
		Die(partId, hitVelocity);
}

void CHealthComponent::Die(int partId, Vec3 hitVelocity)
{
	SEntityPhysicalizeParams params;
	params.nSlot = 0;
	params.mass = 120.0f;
	params.fStiffnessScale = 1.0f;
	params.bCopyJointVelocities = true;

	pe_player_dimensions playerDim;
	pe_player_dynamics playerDyn;
	playerDyn.gravity.z = 15.0f;
	playerDyn.kInertia = 5.5f;

	params.pPlayerDimensions = &playerDim;
	params.pPlayerDynamics = &playerDyn;


	params.type = PE_ARTICULATED;
	m_pEntity->Physicalize(params);

	pe_simulation_params simParams;
	simParams.damping = 2.0f;
	m_pEntity->GetPhysicalEntity()->SetParams(&simParams);

	pe_action_impulse impulse;
	impulse.impulse = hitVelocity.normalized() * 10.0f;
	//Uncomment this to apply velocity directly to hit location. Lower impulse when doing so.
	// 	impulse.partid = partId;

	m_pEntity->GetPhysicalEntity()->Action(&impulse);
}
