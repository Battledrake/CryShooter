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

	void AddAmmo(string equipmentName, int amount);
	int RemoveAmmo (const IEquippable* pEquippable, int desiredAmount);

	bool CheckIfHasEquipment(const IEquippable* pEquippable) const;
	bool CheckIfAmmoFull(const CWeaponComponent* pWeapon) const;
	int GetAmmoReserve(string weaponName) const { return m_ammoReserve.at(weaponName); }

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
	void ThrowWeapon(CWeaponComponent* pWeapon);

	CCharacterComponent* m_pCharacterComp;

	StaticDynArray<IEquippable*, (int)EEquipmentType::Count> m_equipmentSlots;
	StaticDynArray<CWeaponComponent*, 2> m_weapons; //0 = primary, 1 = secondary. Could increase to have more weapons.
	//TODO:	StaticDynArray<CGrenadeComponent*, 1> m_grenades; 

	int m_activeWeaponIndex;
	int m_activeGrenadeIndex;

	std::map<string, int> m_ammoReserve;
};