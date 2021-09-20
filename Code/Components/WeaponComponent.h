#pragma once

#include <CryEntitySystem/IEntitySystem.h>
#include <CrySchematyc/Utils/SharedString.h>
#include <CrySchematyc/ResourceTypes.h>

#include "Interfaces/IInteractable.h"
#include "Interfaces/IEquippable.h"
#include "Events/SimpleEvent.h"

enum class EFireMode : uint8
{
	Single,
	Burst,
	Auto
};

enum class EWeaponType : uint8
{
	Rifle,
	Pistol
};

enum class EWeaponCategory : uint8
{
	Primary,
	Secondary,
	Both
};

enum class EHolsterSlot : uint8
{
	Back,
	Hip
};

static void ReflectType(Schematyc::CTypeDesc<EFireMode>& desc)
{
	desc.SetGUID("{ED416955-0939-436F-A50F-4AC7033BEC10}"_cry_guid);
	desc.SetLabel("Fire Modes");
	desc.SetDefaultValue(EFireMode::Auto);
	desc.AddConstant(EFireMode::Single, "Single", "Single Fire");
	desc.AddConstant(EFireMode::Burst, "Burst", "Burst Fire");
	desc.AddConstant(EFireMode::Auto, "Auto", "Auto Fire");
}

static void ReflectType(Schematyc::CTypeDesc<EWeaponType>& desc)
{
	desc.SetGUID("{ED5FBADA-0B8D-4419-BD3C-0AA781999414}"_cry_guid);
	desc.SetLabel("Weapon Type");
	desc.SetDescription("Determines animation tag and attach slot");
	desc.SetDefaultValue(EWeaponType::Rifle);
	desc.AddConstant(EWeaponType::Rifle, "Rifle", "Rifle");
	desc.AddConstant(EWeaponType::Pistol, "Pistol", "Pistol");
}

static void ReflectType(Schematyc::CTypeDesc<EWeaponCategory>& desc)
{
	desc.SetGUID("{C7719231-03CC-4EAF-8E35-9C9CCFA520C5}"_cry_guid);
	desc.SetLabel("Weapon Category");
	desc.SetDescription("Determines which weapon slot this weapon can be assigned to");
	desc.SetDefaultValue(EWeaponCategory::Primary);
	desc.AddConstant(EWeaponCategory::Primary, "Primary", "Primary");
	desc.AddConstant(EWeaponCategory::Secondary, "Secondary", "Secondary");
	desc.AddConstant(EWeaponCategory::Both, "Both", "Both");
}

static void ReflectType(Schematyc::CTypeDesc<EHolsterSlot>& desc)
{
	desc.SetGUID("{A8A42683-69A1-4F35-8D11-C876057C48D8}"_cry_guid);
	desc.SetLabel("Holster Type");
	desc.SetDescription("Determines where weapon is holstered when not active");
	desc.SetDefaultValue(EHolsterSlot::Back);
	desc.AddConstant(EHolsterSlot::Back, "Back", "Back");
	desc.AddConstant(EHolsterSlot::Hip, "Hip", "Hip");
}

class CInterfaceComponent;

class CWeaponComponent final : public IEntityComponent,
	public IInteractable, public IEquippable
{
public:
	CWeaponComponent() = default;
	virtual ~CWeaponComponent() = default;

	//Events
	SimpleEvent<> m_fireEvent;
	SimpleEvent<Vec2> m_recoilEvent;
	SimpleEvent<int> m_ammoChangedEvent;
	//~Events

	//IInteractable
	virtual void Observe(CCharacterComponent* pObserver, SObjectData& data) override;
	virtual void Interact(CCharacterComponent* pInteractor) override;
	//~IInteractable

	//IEquippable
	virtual string GetEquipmentName() const override { return m_weaponName.c_str(); }
	virtual EEquipmentType GetEquipmentType() const override { return m_equipmentType; }
	//~IEquippable

	CryAudio::ControlId GetWeaponSelectAudio() { return CryAudio::StringToId(m_selectAudio.value); }
	CryAudio::ControlId GetPickupAudio() { return CryAudio::StringToId(m_pickupAudio.value); }
	string GetIconPath() const { return m_iconPath.value; }
	string GetProjectileClass() const { return m_projectileClass.value; }
	EFireMode GetFireMode() const { return m_currentFireMode; }
	EWeaponType GetWeaponType() const { return m_weaponType; }
	EWeaponCategory GetWeaponCategory() const { return m_weaponCategory; }
	EHolsterSlot GetHolsterSlot() const { return m_holsterSlot; }
	int GetClipCount() const { return m_clipCount; }
	int GetClipCapacity() const { return m_clipCapacity; }
	int GetAmmoCount() const { return m_ammoCount; }
	int GetMaxAmmo() const { return m_maxAmmo; }

	void AddAmmo(int amount);
	void DisableFiring() { ProcessFire(false); m_isBursting = false; m_burstQueued = false; }
	void PlayAudio(string audioName);
	void ProcessFire(bool isPressed);
	void Reload();
	EFireMode SwitchFireModes();

	static void ReflectType(Schematyc::CTypeDesc<CWeaponComponent>& desc)
	{
		desc.SetGUID("{ACC5078E-BCCC-404E-8989-2152FCFCABCD}"_cry_guid);
		desc.SetEditorCategory("Custom");
		desc.SetLabel("WeaponComponent");
		desc.SetDescription("Creates weapons");
		desc.AddMember(&CWeaponComponent::m_weaponName, 'name', "WeaponName", "Weapon Name", "Name of weapon, used to identify and differentiate", "");
		desc.AddMember(&CWeaponComponent::m_fireAudio, 'wfa', "FireAudio", "Fire Audio", "Sound gun make on shoot", "");
		desc.AddMember(&CWeaponComponent::m_reloadAudio, 'wra', "ReloadAudio", "Reload Audio", "Sound gun make on reload", "");
		desc.AddMember(&CWeaponComponent::m_dryFireAudio, 'wdfa', "DryFireAudio", "Dryfire Audio", "Sound gun make when empty", "");
		desc.AddMember(&CWeaponComponent::m_pickupAudio, 'wpa', "PickupAudio", "Pickup Audio", "Sound made when picked up", "");
		desc.AddMember(&CWeaponComponent::m_selectAudio, 'wsa', "SelectAudio", "Select Audio", "Sound gun make on select", "");
		desc.AddMember(&CWeaponComponent::m_switchFireModeAudio, 'sfma', "SwitchFireModeAudio", "Switch Fire Mode Audio", "Sound gun make when fire mode changed", "");
		desc.AddMember(&CWeaponComponent::m_iconPath, 'icon', "IconPath", "Icon Path", "Icon to load in weapon hud", "");
		desc.AddMember(&CWeaponComponent::m_projectileClass, 'proj', "ProjectileClass", "Projectile Class", "Entity class to spawn on fire", "");
		desc.AddMember(&CWeaponComponent::m_weaponType, 'type', "WeaponType", "Weapon Type", "Determines animation tag and attach slot", EWeaponType::Rifle);
		desc.AddMember(&CWeaponComponent::m_weaponCategory, 'cat', "WeaponCategory", "Weapon Category", "Determines which weapon slot this weapon can be assigned to", EWeaponCategory::Primary);
		desc.AddMember(&CWeaponComponent::m_holsterSlot, 'slot', "HolsterSlot", "Holster Slot", "Determines where weapon is holstered when not active", EHolsterSlot::Back);
		desc.AddMember(&CWeaponComponent::m_fireModes, 'mode', "FireModes", "Weapon Fire Modes", "Determines the firing modes the weapon supports", Schematyc::CArray<EFireMode>());
		desc.AddMember(&CWeaponComponent::m_fireRate, 'rate', "FireRate", "Weapon Fire Rate", "Rate per minute", 500);
		desc.AddMember(&CWeaponComponent::m_wepRecoil, 'coil', "WeaponRecoil", "Weapon Recoil", "Sets the recoil", Vec2(-2.0f, 5.0f));
		desc.AddMember(&CWeaponComponent::m_shotsInBurst, 'brst', "Burst", "Shots in Burst", "Number of shots fired per burst", 3);
		desc.AddMember(&CWeaponComponent::m_burstDelay, 'dlay', "BurstDelay", "Burst Delay", "Determines the length of time before firing another burst", 0.5f);
		desc.AddMember(&CWeaponComponent::m_clipCapacity, 'clip', "ClipCapacity", "Clip Capacity", "Amount of bullets clip can hold", 30);
		desc.AddMember(&CWeaponComponent::m_startingAmmo, 'ammo', "StartingAmmo", "Starting Ammo", "Ammo available outside of clip at start", 0);
		desc.AddMember(&CWeaponComponent::m_maxAmmo, 'max', "MaxAmmo", "Max Ammo", "Maximum amount of ammo weapon can have", 300);
	}

protected:
	virtual void Initialize() override;
	virtual Cry::Entity::EventFlags GetEventMask() const override;
	virtual void ProcessEvent(const SEntityEvent& event) override;

private:
	void BeginBurst();
	void Fire();

	CInterfaceComponent* m_pInterfaceComponent;

	Schematyc::CSharedString m_weaponName;
	Schematyc::AudioTriggerName m_fireAudio;
	Schematyc::AudioTriggerName m_reloadAudio;
	Schematyc::AudioTriggerName m_dryFireAudio;
	Schematyc::AudioTriggerName m_pickupAudio;
	Schematyc::AudioTriggerName m_selectAudio;
	Schematyc::AudioTriggerName m_switchFireModeAudio;
	Schematyc::TextureFileName m_iconPath;
	Schematyc::EntityClassName m_projectileClass;
	Schematyc::CArray<EFireMode> m_fireModes;
	int m_fireModesIndex;

	bool m_isAutoing = false;
	bool m_isBursting = false;
	bool m_burstQueued = false;

	int m_fireRate = 1;
	Vec2 m_wepRecoil;
	int m_shotsInBurst = 3;
	int m_burstCounter = 0;
	float m_burstDelay = 0.5f;

	float m_fireTimer = 0.0f;
	float m_burstTimer = 0;
	int m_clipCount = 0;
	int m_clipCapacity = 30;
	int m_ammoCount = 0;
	int m_startingAmmo = 0;
	int m_maxAmmo;

	EWeaponType m_weaponType;
	EEquipmentType m_equipmentType = EEquipmentType::Weapon;
	EWeaponCategory m_weaponCategory;
	EHolsterSlot m_holsterSlot;
	EFireMode m_currentFireMode;
};