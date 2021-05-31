#pragma once

#include <CryEntitySystem/IEntitySystem.h>
#include <CrySchematyc/Utils/SharedString.h>

#include "Interfaces/IInteractable.h"
#include "Components/WeaponComponent.h"

class CInterfaceComponent;

class CAmmoPickupComponent final : public IEntityComponent,
	IInteractable
{
public:
	CAmmoPickupComponent() = default;
	virtual ~CAmmoPickupComponent() = default;

	//IInteractable
	virtual void Observe(CCharacterComponent* pObserver, SObjectData& objectData) override;
	virtual void Interact(CCharacterComponent* pInteractor) override;
	//~IInteractable

	static void ReflectType(Schematyc::CTypeDesc<CAmmoPickupComponent>& desc)
	{
		desc.SetGUID("{5BDB3D82-2757-4210-8D7A-289172469E76}"_cry_guid);
		desc.SetEditorCategory("Custom");
		desc.SetLabel("AmmoPickup");
		desc.SetDescription("Ammo Pickup!");
		desc.AddMember(&CAmmoPickupComponent::m_weaponName, 'name', "WeaponName", "Weapon Name", "Determines the weapon the ammo is for", "GenericRifle");
		desc.AddMember(&CAmmoPickupComponent::m_ammoAmount, 'amt', "AmmoAmount", "Ammo Amount", "Determines amount of ammo to give on pickup", 25);
	}

protected:
	virtual void Initialize() override;
	virtual Cry::Entity::EventFlags GetEventMask() const override;
	virtual void ProcessEvent(const SEntityEvent& event) override;

private:
	CInterfaceComponent* m_pInterfaceComponent;

	Schematyc::CSharedString m_weaponName;
	int m_ammoAmount;
};