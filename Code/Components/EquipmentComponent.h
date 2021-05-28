#pragma once

#include <CryEntitySystem/IEntitySystem.h>

#include "Interfaces/IEquippable.h"

class CWeaponComponent;

class CEquipmentComponent final : public IEntityComponent {
public:
	CEquipmentComponent() = default;
	virtual ~CEquipmentComponent() = default;

	void AddEquipment(IEquippable* pEquipment);
	bool CheckIfHasEquipment(IEquippable* pEquippable) const;

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
	void HolsterWeapon();

	StaticDynArray<IEquippable*, (int)EEquipmentType::Count> m_equipmentSlots;
	CWeaponComponent* m_grenades[1];
};