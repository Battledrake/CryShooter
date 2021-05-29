#include "StdAfx.h"
#include "EquipmentComponent.h"

#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <CrySchematyc/Env/Elements/EnvComponent.h>
#include <CryCore/StaticInstanceList.h>

#include "Components/CharacterComponent.h"

namespace
{
	static void RegisterEquipmentComponent(Schematyc::IEnvRegistrar& registrar)
	{
		Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
		{
			Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CEquipmentComponent));
		}
	}
	CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterEquipmentComponent)
}

void CEquipmentComponent::Initialize()
{
	m_pCharacterComp = m_pEntity->GetComponent<CCharacterComponent>();

	m_activeWeaponIndex = 0;

	m_ammoReserve["AssaultRifle"] = 300;
	m_ammoReserve["Pistola"] = 250;
	m_ammoReserve["Shotgun"] = 250;
}

void CEquipmentComponent::TryAddEquipment(IEquippable* pEquipment)
{
	CryLogAlways("WeaponArray: %i", m_weapons.capacity());
	if (CheckIfHasEquipment(pEquipment))
	{
		switch (pEquipment->GetEquipmentType())
		{
			case EEquipmentType::Weapon:
			{
				if (CWeaponComponent* pWeapon = static_cast<CWeaponComponent*>(pEquipment))
				{
					if (CheckIfAmmoFull(pWeapon))
						return;

					AddAmmo(pWeapon->GetEquipmentName(), pWeapon->GetClipCount());
					pWeapon->GetEntity()->Hide(true);
					return;
				}
			}
			break;
			//TODO: For Grenades, we do same as weapon. For Armor, we return.
		}
	}

	switch (pEquipment->GetEquipmentType())
	{
		case EEquipmentType::Weapon:
		{
			if (CWeaponComponent* pWeapon = static_cast<CWeaponComponent*>(pEquipment))
			{
				// 				if (m_weapons.empty())
				// 				{
				// 					m_pCharacterComp->SetActiveWeapon(pWeapon);
				// 					if (pWeapon->GetWeaponSlot() == EWeaponSlot::Primary || pWeapon->GetWeaponSlot() == EWeaponSlot::Both)
				// 					{
				// 						m_activeWeaponIndex = 0;
				// 					}
				// 					else
				// 					{
				// 						m_activeWeaponIndex = 1;
				// 					}
				// 					m_weapons[m_activeWeaponIndex] = pWeapon;
				// 					m_equipmentSlots[(int)EEquipmentType::Weapon] = pWeapon;
				// 					return;
				// 				}
								//Here we're checking if new weapon is the same slot type as the active weapon.
				if ((int)pWeapon->GetWeaponSlot() == m_activeWeaponIndex || pWeapon->GetWeaponSlot() == EWeaponSlot::Both)
				{
					m_pCharacterComp->SetActiveWeapon(pWeapon);
					if (CWeaponComponent* pOldWeapon = m_weapons[m_activeWeaponIndex])
					{
						ThrowWeapon(pOldWeapon);
					}
					m_weapons[m_activeWeaponIndex] = pWeapon;
					m_equipmentSlots[(int)EEquipmentType::Weapon] = pWeapon;
				}
				else
				{
					if (CWeaponComponent* pOldWeapon = m_weapons[(int)pWeapon->GetWeaponSlot()])
					{
						pOldWeapon->GetEntity()->Hide(false);
						pOldWeapon->GetEntity()->DetachThis();
						ThrowWeapon(pOldWeapon);
					}
					m_weapons[(int)pWeapon->GetWeaponSlot()] = pWeapon;
					if (m_equipmentSlots[(int)EEquipmentType::Weapon])
					{
						m_pEntity->AttachChild(pWeapon->GetEntity());
						pWeapon->GetEntity()->Hide(true);
					}
					else
					{
						m_pCharacterComp->SetActiveWeapon(pWeapon);
						m_equipmentSlots[(int)EEquipmentType::Weapon] = pWeapon;
						m_activeWeaponIndex = (int)pWeapon->GetWeaponSlot();
					}
				}
			}
		}
		break;
	}
}

void CEquipmentComponent::ThrowWeapon(CWeaponComponent* pWeapon)
{
	pWeapon->GetEntity()->EnablePhysics(true);

	pe_action_impulse impulseAction;
	const float initialVelocity = 50.0f;
	impulseAction.impulse = m_pEntity->GetForwardDir() * initialVelocity;
	impulseAction.angImpulse = Vec3(initialVelocity, 0, 0);
	pWeapon->GetEntity()->GetPhysicalEntity()->Action(&impulseAction);
}

bool CEquipmentComponent::CheckIfHasEquipment(const IEquippable* pEquippable) const
{
	switch (pEquippable->GetEquipmentType())
	{
		case EEquipmentType::Weapon:
		{
			for (int i = 0; i < m_weapons.capacity(); ++i)
			{
				if (m_weapons.at(i))
				{
					return strcmp(m_weapons.at(i)->GetEquipmentName(), pEquippable->GetEquipmentName()) == 0;
				}
			}
		}
		break;
	}
	return false;
}

bool CEquipmentComponent::CheckIfAmmoFull(const CWeaponComponent* pWeapon) const
{
	return m_ammoReserve.at(pWeapon->GetEquipmentName()) >= pWeapon->GetMaxAmmo();
}

void CEquipmentComponent::AddAmmo(string equipmentName, int amount)
{
	int& ammoReserve = m_ammoReserve.at(equipmentName);
	ammoReserve += amount;
}

int CEquipmentComponent::RemoveAmmo(const IEquippable* pEquippable, int desiredAmount)
{
	int& ammoReserve = m_ammoReserve.at(pEquippable->GetEquipmentName());
	int amountToGive = ammoReserve - desiredAmount >= 0 ? desiredAmount : ammoReserve;
	ammoReserve -= amountToGive;
	return amountToGive;
}

Cry::Entity::EventFlags CEquipmentComponent::GetEventMask() const
{
	return Cry::Entity::EEvent::Reset;
}

void CEquipmentComponent::ProcessEvent(const SEntityEvent& event)
{
	switch (event.event)
	{
		case Cry::Entity::EEvent::Reset:
		{
			m_equipmentSlots.clear();
			m_weapons.clear();
			m_ammoReserve.clear();

			Initialize();
		}
		break;
	}
}
