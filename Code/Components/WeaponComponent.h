#pragma once

#include <CryEntitySystem/IEntitySystem.h>
#include <DefaultComponents/Geometry/AnimatedMeshComponent.h>

#include "Interfaces/IInteractable.h"
//
// 			if (CWeaponComponent* pWep = pHitEnt->GetComponent<CWeaponComponent>())
// 			{
// 				m_pCharacter->EquipWeapon(pWep);
// 			}

enum class EFireMode : uint8
{
	Single,
	Burst,
	Auto
};

enum class EWeaponType : uint8
{
	Primary,
	Secondary,
	Both
};

enum class EAnimationType : uint8
{
	Rifle,
	Pistol
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
	desc.SetDescription("Determines which equipment slot weapon belongs to");
	desc.SetDefaultValue(EWeaponType::Primary);
	desc.AddConstant(EWeaponType::Primary, "Primary", "Primary");
	desc.AddConstant(EWeaponType::Secondary, "Secondary", "Secondary");
	desc.AddConstant(EWeaponType::Both, "Both", "Both");
}

static void ReflectType(Schematyc::CTypeDesc<EAnimationType>& desc)
{
	desc.SetGUID("{D5F09339-E774-405E-85D6-8A93873B2502}"_cry_guid);
	desc.SetLabel("Animation Type");
	desc.SetDescription("Determines which animation tag to set");
	desc.SetDefaultValue(EAnimationType::Rifle);
	desc.AddConstant(EAnimationType::Rifle, "Rifle", "Rifle");
	desc.AddConstant(EAnimationType::Pistol, "Pistol", "Pistol");
}


class InterfaceComponent;

class CWeaponComponent final : public IEntityComponent,
	public IInteractable
{
public:
	CWeaponComponent() = default;
	virtual ~CWeaponComponent() = default;

	//Events
	SimpleEvent<void> m_fireEvent;
	SimpleEvent<void, Vec2> m_recoilEvent;
	//~Events

	//IInteractable
	virtual void Observe(CCharacterComponent* pObserver, SObjectData& data) override;
	virtual void Interact(CCharacterComponent* pInteractor) override;
	//~IInteractable

	string GetWeaponName() const { return m_weaponName.c_str(); }
	string GetIconName() const { return m_iconName.c_str(); }
	EFireMode GetFireMode() const { return m_currentFireMode; }
	EWeaponType GetWeaponType() const { return m_weaponType; }
	EAnimationType GetAnimationType() const { return m_animType; }
	int GetClipCapacity() const { return m_clipCapacity; }
	int GetClipCount() const { return m_clipCount; }

	void DisableFiring() { ProcessFire(false); m_isBursting = false; m_burstQueued = false; }
	void ProcessFire(bool isPressed);
	void ReloadClip(int amount) { m_clipCount += amount; }
	EFireMode SwitchFireModes();

	// Reflect type to set a unique identifier for this component
	// and provide additional information to expose it in the sandbox
	static void ReflectType(Schematyc::CTypeDesc<CWeaponComponent>& desc)
	{
		desc.SetGUID("{ACC5078E-BCCC-404E-8989-2152FCFCABCD}"_cry_guid);
		desc.SetEditorCategory("Custom");
		desc.SetLabel("WeaponComponent");
		desc.SetDescription("Creates weapons");
		desc.AddMember(&CWeaponComponent::m_weaponName, 'name', "WeaponName", "Weapon Name", "Name of weapon, used for interaction icon", "");
		desc.AddMember(&CWeaponComponent::m_iconName, 'icon', "IconName", "Icon Name", "Name of icon to load for Hud", "");
		desc.AddMember(&CWeaponComponent::m_weaponType, 'type', "WeaponType", "Weapon Type", "Type of weapon, used for equipping", EWeaponType::Primary);
		desc.AddMember(&CWeaponComponent::m_animType, 'anim', "AnimationType", "Animation Type", "Determines animation tag to activate", EAnimationType::Rifle);
		desc.AddMember(&CWeaponComponent::m_fireModes, 'mode', "FireModes", "Weapon Fire Modes", "Determines the firing modes the weapon supports", Schematyc::CArray<EFireMode>());
		desc.AddMember(&CWeaponComponent::m_fireRate, 'rate', "FireRate", "Weapon Fire Rate", "Rate per minute", 500);
		desc.AddMember(&CWeaponComponent::m_wepRecoil, 'coil', "WeaponRecoil", "Weapon Recoil", "Sets the recoil", Vec2(-2.0f, 5.0f));
		desc.AddMember(&CWeaponComponent::m_shotsInBurst, 'brst', "Burst", "Shots in Burst", "Number of shots fired per burst", 3);
		desc.AddMember(&CWeaponComponent::m_burstDelay, 'dlay', "BurstDelay", "Burst Delay", "Determines the length of time before firing another burst", 0.5f);
		desc.AddMember(&CWeaponComponent::m_clipCapacity, 'clip', "ClipCapacity", "Clip Capacity", "Amount of bullets held by clip", 30);
	}

protected:
	virtual void Initialize() override;
	virtual Cry::Entity::EventFlags GetEventMask() const override;
	virtual void ProcessEvent(const SEntityEvent& event) override;

private:
	void BeginBurst();
	void Fire();

	CInterfaceComponent* m_pInterfaceComponent;
	Cry::DefaultComponents::CBaseMeshComponent* m_pMesh;

	Schematyc::CSharedString m_weaponName;
	Schematyc::CSharedString m_iconName;
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
	float m_burstTimer = 0;;
	int m_clipCount = 0;
	int m_clipCapacity = 30;

	EWeaponType m_weaponType;
	EAnimationType m_animType;
	EFireMode m_currentFireMode;
};