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
}

void CEquipmentComponent::TryAddEquipment(IEquippable* pEquipment)
{
	if (HasEquipment(pEquipment))
	{
		switch (pEquipment->GetEquipmentType())
		{
			case EEquipmentType::Weapon:
			{
				if (CWeaponComponent* pWeapon = static_cast<CWeaponComponent*>(pEquipment))
				{
					if (IsAmmoFull(pWeapon->GetEquipmentName()))
						return;

					TryAddAmmo(pWeapon->GetEquipmentName(), pWeapon->GetClipCount());
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

bool CEquipmentComponent::HasEquipment(const IEquippable* pEquippable) const
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

bool CEquipmentComponent::HasWeapon(const string weaponName) const
{
	for (int i = 0; i < m_weapons.capacity(); ++i)
	{
		if (m_weapons.at(i))
		{
			if (strcmp(m_weapons.at(i)->GetEquipmentName(), weaponName) == 0)
			{
				return true;
			}
		}
	}
	return false;
}

bool CEquipmentComponent::IsAmmoFull(const string weaponName) const
{
	bool isFull = false;
	for (int i = 0; i < m_weapons.capacity(); ++i)
	{
		if (m_weapons[i])
		{
			if (strcmp(m_weapons[i]->GetEquipmentName(), weaponName) == 0)
			{
				isFull = m_weapons[i]->GetAmmoCount() >= m_weapons[i]->GetMaxAmmo();
			}
		}
	}
	return isFull;
}

bool CEquipmentComponent::TryAddAmmo(string weaponName, int amount)
{
	for (int i = 0; i < m_weapons.capacity(); ++i)
	{
		if (m_weapons.at(i))
		{
			if (strcmp(m_weapons.at(i)->GetEquipmentName(), weaponName) == 0)
			{
				if (m_weapons.at(i)->GetAmmoCount() < m_weapons.at(i)->GetMaxAmmo())
				{
					m_weapons.at(i)->AddAmmo(amount);
					return true;
				}
			}
		}
	}
	return false;
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

			Initialize();
		}
		break;
	}
}
