#include "StdAfx.h"
#include "ProjectileComponent.h"

#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <CrySchematyc/Env/Elements/EnvComponent.h>
#include <CryCore/StaticInstanceList.h>

#include <DefaultComponents/Geometry/AdvancedAnimationComponent.h>

namespace
{
	static void RegisterProjectileComponent(Schematyc::IEnvRegistrar& registrar)
	{
		Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
		{
			Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CProjectileComponent));
			{
			}
		}
	}
	CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterProjectileComponent)
}

void CProjectileComponent::Initialize()
{
	m_pParticle = m_pEntity->GetComponent<Cry::DefaultComponents::CPhysParticleComponent>();

	if (auto* pPhysics = GetEntity()->GetPhysics())
	{
		pe_action_impulse impulseAction;

		impulseAction.impulse = GetEntity()->GetWorldRotation().GetColumn1() * m_moveSpeed;

		pPhysics->Action(&impulseAction);
	}
}

Cry::Entity::EventFlags CProjectileComponent::GetEventMask() const
{
	return
		Cry::Entity::EEvent::PhysicsCollision;
}

void CProjectileComponent::ProcessEvent(const SEntityEvent& event)
{
	switch (event.event)
	{
		case Cry::Entity::EEvent::PhysicsCollision:
			if (EventPhysCollision* pPhys = reinterpret_cast<EventPhysCollision*>(event.nParam[0]))
			{
				if (IEntity* pEntity = gEnv->pEntitySystem->GetEntityFromPhysics(pPhys->pEntity[1]))
				{
					if (CInterfaceComponent* pInterfaceComp = pEntity->GetComponent<CInterfaceComponent>())
					{
						if (ITakeDamage* pDamageInterface = pInterfaceComp->GetInterface<ITakeDamage>())
						{
							pDamageInterface->PassDamage(10.0f, pPhys->partid[1]);
						}
					}
				}
			}
			gEnv->pEntitySystem->RemoveEntity(GetEntityId());
			break;
	}
}
