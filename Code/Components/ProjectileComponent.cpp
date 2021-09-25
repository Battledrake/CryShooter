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
		}
	}
	CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterProjectileComponent)
}

void CProjectileComponent::InitializeProjectile(CCharacterComponent* pOwner)
{
	m_pOwner = pOwner;

	pe_params_particle particle;
	particle.collTypes = ent_all;
	particle.mass = 0.0f;
	particle.flags = particle_no_impulse | pef_log_collisions;
	particle.heading = m_pEntity->GetWorldRotation().GetColumn1();
	particle.velocity = m_moveSpeed;
	particle.pColliderToIgnore = pOwner->GetAnimComp()->GetCharacter()->GetPhysEntity();

	SEntityPhysicalizeParams entityParams;
	entityParams.type = PE_PARTICLE;
	entityParams.mass = particle.mass;
	entityParams.pParticle = &particle;
	entityParams.nSlot = -1;
	m_pEntity->Physicalize(entityParams);
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
							pDamageInterface->PassDamage(10.0f, pPhys->partid[1], pPhys->vloc[0]);
						}
					}
				}
			}
			gEnv->pEntitySystem->RemoveEntity(GetEntityId());
			break;
	}
}