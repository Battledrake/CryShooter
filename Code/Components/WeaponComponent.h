#pragma once

#include <CryEntitySystem/IEntitySystem.h>
#include <DefaultComponents/Geometry/AnimatedMeshComponent.h>

#include "Interfaces/IInteractable.h"
#include "Interfaces/IEquippable.h"
#include "Events/SimpleEvent.h"

enum class EFireMode : uint8
{
	Single,
	Burst,
	Auto
};

enum class EAnimationType : uint8
{
	Rifle,
	Pistol
};

enum class EWeaponSlot : uint8
{
	Primary,
	Secondary,
	Both
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

static void ReflectType(Schematyc::CTypeDesc<EAnimationType>& desc)
{
	desc.SetGUID("{ED5FBADA-0B8D-4419-BD3C-0AA781999414}"_cry_guid);
	desc.SetLabel("Animation Type");
	desc.SetDescription("Determines animation tag and attach slot");
	desc.SetDefaultValue(EAnimationType::Rifle);
	desc.AddConstant(EAnimationType::Rifle, "Rifle", "Rifle");
	desc.AddConstant(EAnimationType::Pistol, "Pistol", "Pistol");
}

static void ReflectType(Schematyc::CTypeDesc<EWeaponSlot>& desc)
{
	desc.SetGUID("{C7719231-03CC-4EAF-8E35-9C9CCFA520C5}"_cry_guid);
	desc.SetLabel("Weapon Slot");
	desc.SetDescription("Determines which Weapon slot this weapon belongs to");
	desc.SetDefaultValue(EWeaponSlot::Primary);
	desc.AddConstant(EWeaponSlot::Primary, "Primary", "Primary");
	desc.AddConstant(EWeaponSlot::Secondary, "Secondary", "Secondary");
	desc.AddConstant(EWeaponSlot::Both, "Both", "Both");
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

	string GetIconPath() const { return m_iconPath.value; }
	string GetProjectileClass() const { return m_projectileClass.value; }
	EFireMode GetFireMode() const { return m_currentFireMode; }
	EAnimationType GetWeaponType() const { return m_animType; }
	EEquipmentType GetEquipType() const { return m_equipmentType; }
	EWeaponSlot GetWeaponSlot() const { return m_weaponSlot; }
	int GetClipCount() const { return m_clipCount; }
	int GetClipCapacity() const { return m_clipCapacity; }
	int GetAmmoCount() const { return m_ammoCount; }
	int GetMaxAmmo() const { return m_maxAmmo; }

	void AddAmmo(int amount);
	void DisableFiring() { ProcessFire(false); m_isBursting = false; m_burstQueued = false; }
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
		desc.AddMember(&CWeaponComponent::m_iconPath, 'icon', "IconPath", "Icon Path", "Icon to load in weapon hud", "");
		desc.AddMember(&CWeaponComponent::m_projectileClass, 'proj', "ProjectileClass", "Projectile Class", "Entity class to spawn on fire", "");
		desc.AddMember(&CWeaponComponent::m_animType, 'anim', "AnimationType", "Animation Type", "Determines animation tag and attach slot", EAnimationType::Rifle);
		desc.AddMember(&CWeaponComponent::m_weaponSlot, 'slot', "WeaponSlot", "Weapon Slot", "Determines which weapon slot this weapon can belong to", EWeaponSlot::Primary);
		desc.AddMember(&CWeaponComponent::m_fireModes, 'mode', "FireModes", "Weapon Fire Modes", "Determines the firing modes the weapon supports", Schematyc::CArray<EFireMode>());
		desc.AddMember(&CWeaponComponent::m_fireRate, 'rate', "FireRate", "Weapon Fire Rate", "Rate per minute", 500);
		desc.AddMember(&CWeaponComponent::m_wepRecoil, 'coil', "WeaponRecoil", "Weapon Recoil", "Sets the recoil", Vec2(-2.0f, 5.0f));
		desc.AddMember(&CWeaponComponent::m_shotsInBurst, 'brst', "Burst", "Shots in Burst", "Number of shots fired per burst", 3);
		desc.AddMember(&CWeaponComponent::m_burstDelay, 'dlay', "BurstDelay", "Burst Delay", "Determines the length of time before firing another burst", 0.5f);
		desc.AddMember(&CWeaponComponent::m_clipCapacity, 'clip', "ClipCapacity", "Clip Capacity", "Amount of bullets held by clip", 30);
		desc.AddMember(&CWeaponComponent::m_ammoCount, 'ammo', "AmmoCount", "Ammo Count", "Ammo available outside of clip", 0);
		desc.AddMember(&CWeaponComponent::m_maxAmmo, 'max', "MaxAmmo", "Max Ammo", "Maximum amount of ammo weapon can have", 300);
	}

protected:
	virtual void Initialize() override;
	virtual Cry::Entity::EventFlags GetEventMask() const override;
	virtual void ProcessEvent(const SEntityEvent& event) override;

private:
	void BeginBurst();
	void Fire();

	Cry::DefaultComponents::CBaseMeshComponent* m_pMesh;
	CInterfaceComponent* m_pInterfaceComponent;

	Schematyc::CSharedString m_weaponName;
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
	int m_maxAmmo;

	EAnimationType m_animType;
	EEquipmentType m_equipmentType = EEquipmentType::Weapon;
	EWeaponSlot m_weaponSlot;
	EFireMode m_currentFireMode;
};