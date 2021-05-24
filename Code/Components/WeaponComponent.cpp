#include "StdAfx.h"
#include "WeaponComponent.h"

#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <CrySchematyc/Env/Elements/EnvComponent.h>
#include <CryCore/StaticInstanceList.h>

namespace
{
	static void RegisterWeaponComponent(Schematyc::IEnvRegistrar& registrar)
	{
		Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
		{
			Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CWeaponComponent));
			{
			}
		}
	}
	CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterWeaponComponent)
}

void CWeaponComponent::Initialize()
{
	m_pInterfaceComponent = m_pEntity->GetOrCreateComponent<CInterfaceComponent>();
	m_pMesh = m_pEntity->GetComponent<Cry::DefaultComponents::CBaseMeshComponent>();

	m_currentFireMode = m_fireModes.At(0);

	m_clipCount = m_clipCapacity;

	//Put here to prevent divide by 0 errors
	m_fireRate = m_fireRate > 0 ? m_fireRate : 1;
}

Cry::Entity::EventFlags CWeaponComponent::GetEventMask() const
{
	return
		Cry::Entity::EEvent::GameplayStarted |
		Cry::Entity::EEvent::Update |
		Cry::Entity::EEvent::Reset;
}

void CWeaponComponent::ProcessEvent(const SEntityEvent& event)
{
	switch (event.event)
	{
		case Cry::Entity::EEvent::GameplayStarted:
		{
			m_pInterfaceComponent->AddInterface<IInteractable>(this);
		}
		break;
		case Cry::Entity::EEvent::Update:
		{
			if (m_burstTimer > 0)
				m_burstTimer -= event.fParam[0];
			if (m_fireTimer > 0)
				m_fireTimer -= event.fParam[0];

			if ((m_isAutoing || m_isBursting) && m_fireTimer <= 0)
			{
				Fire();
			}
			if (!m_isBursting && m_burstQueued && m_burstTimer <= 0)
			{
				BeginBurst();
			}
		}
		break;
		case Cry::Entity::EEvent::Reset:
		{
			m_clipCount = m_clipCapacity;
			m_currentFireMode = m_fireModes.At(0);
		}
		break;
	}
}

void CWeaponComponent::ProcessFire(bool isPressed)
{
	if (isPressed && m_clipCapacity <= 0)
	{
		//TODO: Play click sound when trying to fire
	}

	switch (m_currentFireMode)
	{
		case EFireMode::Single:
		{
			if (isPressed)
			{
				Fire();
			}
		}
		break;
		case EFireMode::Burst:
		{
			if (isPressed)
			{
				if (!m_isBursting && m_burstTimer <= 0)
					BeginBurst();
				else if(m_burstTimer > 0)
					m_burstQueued = true;
			}
		}
		break;
		case EFireMode::Auto:
		{
			m_isAutoing = isPressed;
		}
		break;
	}
}

void CWeaponComponent::BeginBurst()
{
	m_burstQueued = false;
	m_isBursting = true;
	m_burstCounter = m_shotsInBurst;
	Fire();
}

void CWeaponComponent::Fire()
{
	if (m_clipCount > 0)
	{
		int rateAdjuster = 60;

		if (m_isBursting)
		{
			rateAdjuster = 45; //Burst shoots a little faster in exchange for delay between firing
			m_burstCounter--;
			if (m_burstCounter <= 0)
			{
				m_isBursting = false;
				m_burstTimer = m_burstDelay;
			}
		}

		m_clipCount--;
		m_fireTimer = 1 / ((float)m_fireRate / rateAdjuster);
		m_fireEvent.Invoke();
		m_recoilEvent.Invoke(m_wepRecoil);
	}
	else
	{
		m_isBursting = false;
		m_isAutoing = false;
	}
}

EFireMode CWeaponComponent::SwitchFireModes()
{
	if (m_isAutoing || m_isBursting || m_burstTimer > 0)
		return m_currentFireMode;

	m_fireModesIndex++;
	if (m_fireModesIndex >= m_fireModes.Size())
		m_fireModesIndex = 0;

	m_currentFireMode = m_fireModes.At(m_fireModesIndex);

	return m_currentFireMode;
}

void CWeaponComponent::Observe(CCharacterComponent* pObserver, SObjectData& data)
{
	data.objectKeyword = "Take";
	data.objectName = m_weaponName.c_str();
}

void CWeaponComponent::Interact(CCharacterComponent* pInteractor)
{
	pInteractor->EquipWeapon(this);
}
