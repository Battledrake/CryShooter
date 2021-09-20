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
	m_pCharacterComp->m_characterChangedEvent.RegisterListener([this]()
	{
		if (m_weapons[m_activeWeaponIndex])
			AttachWeapon(m_weapons[m_activeWeaponIndex]);

		const int otherIndex = m_activeWeaponIndex == 0 ? 1 : 0;
		if (m_weapons.at(otherIndex))
			HolsterWeapon(m_weapons.at(otherIndex));
	});

	m_activeWeaponIndex = 0;
}

void CEquipmentComponent::TryAddEquipment(IEquippable* pEquipment)
{
	//If Character already has equipment, we take ammo and hide it instead of removing entity.
	//If ammo is full, just leave alone to pick up later.
	if (HasSameEquipment(pEquipment))
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
					PlayAudio(pWeapon->GetPickupAudio());
					pWeapon->GetEntity()->Hide(true);
					return;
				}
			}
			break;
			//TODO: For Grenades, we do same as weapon. For Armor, we return.
		}
	}

	//If doesn't have weapon and it's the same slot type as current weapon, swap and throw, otherwise store on entity.
	//If has weapon and is same type as stored weapon, we swap and throw stored weapon.
	switch (pEquipment->GetEquipmentType())
	{
		case EEquipmentType::Weapon:
		{
			if (CWeaponComponent* pWeapon = static_cast<CWeaponComponent*>(pEquipment))
			{
				pWeapon->GetEntity()->EnablePhysics(false);
				if ((int)pWeapon->GetWeaponCategory() == m_activeWeaponIndex || pWeapon->GetWeaponCategory() == EWeaponCategory::Both)
				{
					if (CWeaponComponent* pActiveWeapon = m_weapons.at(m_activeWeaponIndex))
					{
						const int otherIndex = m_activeWeaponIndex == 0 ? 1 : 0;
						if (!m_weapons.at(otherIndex))
						{
							if (pWeapon->GetWeaponCategory() == EWeaponCategory::Both)
							{
								m_weapons.at(otherIndex) = pWeapon;
								HolsterWeapon(pWeapon);
								PlayAudio(pWeapon->GetPickupAudio());
								return;
							}
							else if (pActiveWeapon->GetWeaponCategory() == EWeaponCategory::Both)
							{
								m_weapons.at(otherIndex) = pActiveWeapon;
								HolsterWeapon(pActiveWeapon);
							}
						}
						else
						{
							ClearWeaponAttach(pActiveWeapon);
							ThrowWeapon(pActiveWeapon);
						}
					}
					m_pCharacterComp->SetActiveWeapon(pWeapon);
					AttachWeapon(pWeapon);
					m_weapons.at(m_activeWeaponIndex) = pWeapon;
					m_equipmentSlots.at((int)EEquipmentType::Weapon) = pWeapon;
				}
				else
				{
					if (CWeaponComponent* pOldWeapon = m_weapons.at((int)pWeapon->GetWeaponCategory()))
					{
						ClearHolsterAttach(pOldWeapon);
						ThrowWeapon(pOldWeapon);
					}
					m_weapons.at((int)pWeapon->GetWeaponCategory()) = pWeapon;
					//If we have another weapon that's active, we holster this one
					if (m_equipmentSlots.at((int)EEquipmentType::Weapon))
					{
						HolsterWeapon(pWeapon);
					}
					else
					{
						//No other weapons, so we set this weapon to be the primary weapon
						m_pCharacterComp->SetActiveWeapon(pWeapon);
						AttachWeapon(pWeapon);
						m_equipmentSlots.at((int)EEquipmentType::Weapon) = pWeapon;
						m_activeWeaponIndex = (int)pWeapon->GetWeaponCategory();
					}
				}
				PlayAudio(pWeapon->GetPickupAudio());
			}
		}
		break;
	}
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

void CEquipmentComponent::ClearWeaponAttach(CWeaponComponent* pWeapon)
{
	if (IAttachment* pAttach = m_pCharacterComp->GetAnimComp()->GetCharacter()->GetIAttachmentManager()->GetInterfaceByName(EnumToString(pWeapon->GetWeaponType())))
	{
		pAttach->ClearBinding();
	}
}

void CEquipmentComponent::AttachWeapon(CWeaponComponent* pWeapon)
{
	if (IAttachment* pAttach = m_pCharacterComp->GetAnimComp()->GetCharacter()->GetIAttachmentManager()->GetInterfaceByName(EnumToString(pWeapon->GetWeaponType())))
	{
		CEntityAttachment* pWepAttach = new CEntityAttachment();
		pWepAttach->SetEntityId(pWeapon->GetEntityId());
		pAttach->AddBinding(pWepAttach);
		PlayAudio(pWeapon->GetWeaponSelectAudio());
	}
}

void CEquipmentComponent::ClearHolsterAttach(CWeaponComponent* pWeapon)
{
	if (IAttachment* pAttach = m_pCharacterComp->GetAnimComp()->GetCharacter()->GetIAttachmentManager()->GetInterfaceByName(EnumToString(pWeapon->GetHolsterSlot())))
	{
		pAttach->ClearBinding();
	}
}

void CEquipmentComponent::HolsterWeapon(CWeaponComponent* pWeapon)
{
	if (IAttachment* pAttach = m_pCharacterComp->GetAnimComp()->GetCharacter()->GetIAttachmentManager()->GetInterfaceByName(EnumToString(pWeapon->GetHolsterSlot())))
	{
		CEntityAttachment* pWepAttach = new CEntityAttachment();
		pWepAttach->SetEntityId(pWeapon->GetEntityId());
		pAttach->AddBinding(pWepAttach);
	}
}

void CEquipmentComponent::SwapWeapons()
{
	const int otherIndex = m_activeWeaponIndex == 0 ? 1 : 0;
	if (CWeaponComponent* pWeaponToEquip = m_weapons.at(otherIndex))
	{
		CWeaponComponent* pWeaponToHolster = m_weapons.at(m_activeWeaponIndex);
		ClearHolsterAttach(pWeaponToEquip);
		ClearWeaponAttach(pWeaponToHolster);
		HolsterWeapon(pWeaponToHolster);
		AttachWeapon(pWeaponToEquip);
		m_pCharacterComp->SetActiveWeapon(pWeaponToEquip);
		m_activeWeaponIndex = otherIndex;
		m_equipmentSlots.at((int)EEquipmentType::Weapon) = m_weapons.at(m_activeWeaponIndex);
	}
	else
	{
		return;
	}
}

void CEquipmentComponent::ThrowWeapon(CWeaponComponent* pWeapon)
{
	pWeapon->GetEntity()->EnablePhysics(true);

	pe_action_impulse impulseAction;
	const float initialVelocity = 25.0f;
	impulseAction.impulse = m_pEntity->GetForwardDir() * initialVelocity;
	impulseAction.angImpulse = Vec3(initialVelocity, 0.0f, 0.0f);
	pWeapon->GetEntity()->GetPhysicalEntity()->Action(&impulseAction);
}

bool CEquipmentComponent::HasSameEquipment(const IEquippable* pEquippable) const
{
	bool hasEquipment = false;
	switch (pEquippable->GetEquipmentType())
	{
		case EEquipmentType::Weapon:
		{
			for (int i = 0; i < m_weapons.capacity(); ++i)
			{
				if (m_weapons.at(i))
				{
					if (strcmp(m_weapons.at(i)->GetEquipmentName(), pEquippable->GetEquipmentName()) == 0)
						hasEquipment = true;
				}
			}
		}
		break;
	}
	return hasEquipment;
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
		if (m_weapons.at(i))
		{
			if (strcmp(m_weapons.at(i)->GetEquipmentName(), weaponName) == 0)
			{
				isFull = m_weapons.at(i)->GetAmmoCount() >= m_weapons.at(i)->GetMaxAmmo();
			}
		}
	}
	return isFull;
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
			for (int i = 0; i < m_equipmentSlots.capacity(); ++i)
			{
				if (m_equipmentSlots.at(i))
					m_equipmentSlots.at(i) = nullptr;
			}
			for (int i = 0; i < m_weapons.capacity(); ++i)
			{
				if (m_weapons.at(i))
				{
					if (i == m_activeWeaponIndex)
						ClearWeaponAttach(m_weapons.at(i));
					else
						ClearHolsterAttach(m_weapons.at(i));
					m_weapons.at(i) = nullptr;
				}
			}

			Initialize();
		}
		break;
	}
}

void CEquipmentComponent::PlayAudio(CryAudio::ControlId audioId)
{
	CryAudio::SExecuteTriggerData triggerData(audioId, (const char*)nullptr,
		CryAudio::EOcclusionType::Ignore, m_pEntity->GetWorldPos());
	gEnv->pAudioSystem->ExecuteTriggerEx(triggerData);
}