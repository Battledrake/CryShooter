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
		}
	}
	CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterWeaponComponent)
}

void CWeaponComponent::Initialize()
{
	m_pInterfaceComponent = m_pEntity->GetOrCreateComponent<CInterfaceComponent>();

	m_fireModesIndex = 0;
	m_currentFireMode = m_fireModes.At(m_fireModesIndex);

	m_clipCount = m_clipCapacity;
	m_ammoCount = m_startingAmmo;

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
			m_pInterfaceComponent->AddInterface<IEquippable>(this);	
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
			m_ammoCount = m_startingAmmo;
			m_fireModesIndex = 0;
			m_currentFireMode = m_fireModes.At(m_fireModesIndex);
			m_pEntity->EnablePhysics(true);
		}
		break;
	}
}

void CWeaponComponent::ProcessFire(bool isPressed)
{
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
				else if (m_burstTimer > 0)
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
		PlayAudio(m_fireAudio.value);

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
		PlayAudio(m_dryFireAudio.value);

		m_isBursting = false;
		m_isAutoing = false;
	}
}

void CWeaponComponent::AddAmmo(int amount)
{
	m_ammoCount += amount;
	if (m_ammoCount > m_maxAmmo)
		m_ammoCount = m_maxAmmo;
	m_ammoChangedEvent.Invoke(m_ammoCount);
}

void CWeaponComponent::Reload()
{
	if (m_ammoCount <= 0 || m_clipCount == m_clipCapacity)
		return;

	PlayAudio(m_reloadAudio.value);

	const int clipSpace = m_clipCapacity - m_clipCount;
	const int fill = m_ammoCount - clipSpace >= 0 ? clipSpace : m_ammoCount;
	m_clipCount += fill;
	m_ammoCount -= fill;
	m_ammoChangedEvent.Invoke(m_ammoCount);
}

EFireMode CWeaponComponent::SwitchFireModes()
{
	if (m_isAutoing || m_isBursting || m_burstTimer > 0)
		return m_currentFireMode;

	m_fireModesIndex++;
	if (m_fireModesIndex >= m_fireModes.Size())
		m_fireModesIndex = 0;

	m_currentFireMode = m_fireModes.At(m_fireModesIndex);

	PlayAudio(m_switchFireModeAudio.value);

	return m_currentFireMode;
}

void CWeaponComponent::Observe(CCharacterComponent* pObserver, SObjectData& data)
{
	data.objectKeyword = "Take";
 	data.objectName = m_weaponName.c_str();

	if (pObserver)
	{
		if (CEquipmentComponent* pEquipComp = pObserver->GetEntity()->GetComponent<CEquipmentComponent>())
		{
			if (pEquipComp->HasWeapon(m_weaponName.c_str()))
			{
				data.objectBonus = "Ammo";
				if (pEquipComp->IsAmmoFull(m_weaponName.c_str()))
				{
					data.objectKeyword = "";
					data.objectBonus = "Full";
				}
			}
		}
	}
}

void CWeaponComponent::Interact(CCharacterComponent* pInteractor)
{
	pInteractor->GetEquipmentComponent()->TryAddEquipment(this);
}

void CWeaponComponent::PlayAudio(string audioName)
{
	CryAudio::SExecuteTriggerData triggerData(CryAudio::StringToId(audioName),
		(const char*)nullptr, CryAudio::EOcclusionType::Ignore, m_pEntity->GetWorldPos());
	gEnv->pAudioSystem->ExecuteTriggerEx(triggerData);
}
