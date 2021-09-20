#pragma once

#include <map>
#include <CryEntitySystem/IEntitySystem.h>

#include "Interfaces/IEquippable.h"

class CCharacterComponent;
class CWeaponComponent;

class CEquipmentComponent final : public IEntityComponent {
public:
	CEquipmentComponent() = default;
	virtual ~CEquipmentComponent() = default;

 	void TryAddEquipment(IEquippable* pEquippable);
	bool TryAddAmmo(string weaponName, int amount);

	bool HasSameEquipment(const IEquippable* equipmentName) const;
	bool HasWeapon(const string weaponName) const;
	bool IsAmmoFull(string weaponName) const;
	void PlayAudio(CryAudio::ControlId audioId);
	void SwapWeapons();

	static void ReflectType(Schematyc::CTypeDesc<CEquipmentComponent>& desc) {
		desc.SetGUID("{4738182F-4626-4A54-B467-8D68D9953C21}"_cry_guid);
		desc.SetEditorCategory("Custom");
		desc.SetLabel("Equipment");
		desc.SetDescription("This does your equipment handling");
	}

protected:
	virtual void Initialize() override;
	virtual Cry::Entity::EventFlags GetEventMask() const override;
	virtual void ProcessEvent(const SEntityEvent& event) override;

private:
	void ClearWeaponAttach(CWeaponComponent* pWeapon);
	void AttachWeapon(CWeaponComponent* pWeapon);
	void ClearHolsterAttach(CWeaponComponent* pWeapon);
	void HolsterWeapon(CWeaponComponent* pWeapon);
	void ThrowWeapon(CWeaponComponent* pWeapon);

	CCharacterComponent* m_pCharacterComp;

	StaticDynArray<IEquippable*, (int)EEquipmentType::Count> m_equipmentSlots;
	StaticDynArray<CWeaponComponent*, 2> m_weapons; //0 = primary, 1 = secondary.
	//TODO:	StaticDynArray<CGrenadeComponent*, 1> m_grenades; 

	int m_activeWeaponIndex;
	int m_activeGrenadeIndex;
};