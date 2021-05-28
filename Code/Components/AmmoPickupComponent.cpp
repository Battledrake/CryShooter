#include "StdAfx.h"
#include "AmmoPickupComponent.h"

#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <CrySchematyc/Env/Elements/EnvComponent.h>
#include <CryCore/StaticInstanceList.h>

#include "Components/CharacterComponent.h"
#include "Helpers/Conversions.h"

namespace
{
	static void RegisterAmmoPickupComponent(Schematyc::IEnvRegistrar& registrar)
	{
		Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
		{
			Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CAmmoPickupComponent));
		}
	}
	CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterAmmoPickupComponent)
}

void CAmmoPickupComponent::Initialize()
{
	m_pMesh = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CStaticMeshComponent>();
	m_pInterfaceComponent = m_pEntity->GetOrCreateComponent<CInterfaceComponent>();
}

Cry::Entity::EventFlags CAmmoPickupComponent::GetEventMask() const
{
	return Cry::Entity::EEvent::GameplayStarted;
}

void CAmmoPickupComponent::ProcessEvent(const SEntityEvent& event)
{
	switch (event.event)
	{
		case Cry::Entity::EEvent::GameplayStarted:
		{
			m_pInterfaceComponent->AddInterface<IInteractable>(this);
		}
		break;
	}
}

void CAmmoPickupComponent::Observe(CCharacterComponent* pObserver, SObjectData& objectData)
{
	objectData.objectKeyword = "Grab";
	objectData.objectName = m_weaponName.c_str();
	objectData.objectBonus = "Ammo";
}

void CAmmoPickupComponent::Interact(CCharacterComponent* pInteractor)
{
	pInteractor->AddAmmo(m_weaponName.c_str(), m_ammoAmount);
	m_pEntity->Hide(true);
}
