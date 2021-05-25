#pragma once

#include <map>
#include <CryEntitySystem/IEntitySystem.h>
#include <DefaultComponents/Geometry/AdvancedAnimationComponent.h>
#include <DefaultComponents/Physics/CharacterControllerComponent.h>

#include "Components/InterfaceComponent.h"
#include "Interfaces/ITakeDamage.h"
#include "Events/SimpleEvent.h"

enum class ECollisionType
{
	Player = BIT(1),
	Enemy = BIT(2)
};

static void ReflectType(Schematyc::CTypeDesc<ECollisionType>& desc)
{
	desc.SetGUID("{80D5A6D9-D84A-4CC4-BFC6-ACB1AEF076C1}"_cry_guid);
	desc.SetLabel("Collision Type");
	desc.SetDescription("Determines what type of collision this entity represents; used for masking");
	desc.AddConstant(ECollisionType::Player, "player", "Player");
	desc.AddConstant(ECollisionType::Enemy, "enemy", "Enemy");
}

class CBodyDamageComponent;
class CWeaponComponent;
class CIKTorsoAim;

class CCharacterComponent final :
	public IEntityComponent,
	public ITakeDamage
{
public:
	CCharacterComponent() = default;
	virtual ~CCharacterComponent() = default;

	//Events
	SimpleEvent<void, string, string, int, int, int> m_equipEvent;
	SimpleEvent<void, int, int> m_wepFiredEvent;
	SimpleEvent<void, int> m_reloadEvent;
	SimpleEvent<void, string> m_switchFireModeEvent;
	SimpleEvent<void, int> m_ammoAddedEvent;
	//~Events

	Cry::DefaultComponents::CAdvancedAnimationComponent* GetAnimComp() { return m_pAnimComp; }
	std::vector<CWeaponComponent*> GetCharacterWeapons() const { return m_weapons; }

	void AddAmmo(string weaponName, int amount);
	void ChangeCharacter(const Schematyc::CharacterFileName charFile, const Schematyc::CSharedString context = "");
	void EquipWeapon(CWeaponComponent* weapon);
	void ProcessFire(bool isPressed);
	void ProcessJump();
	void ProcessReload();
	void ProcessSprinting(bool isPressed);
	void SetOwner(IEntity* pOwner) { m_pOwner = pOwner; }
	void SwitchFireMode();

	void UpdateMovement(float travelSpeed, float travelAngle);
	void UpdateLookOrientation(const Vec3& lookDirection);
	void UpdateRotation(const Quat& rotation);

	//ITakeDamage
	virtual void PassDamage(float amount, int partId = -1) override;
	//~ITakeDamage

	static void ReflectType(Schematyc::CTypeDesc<CCharacterComponent>& desc)
	{
		desc.SetGUID("{E733D7AE-AE23-404A-841E-24718780EEE7}"_cry_guid);
		desc.SetEditorCategory("Custom");
		desc.SetLabel("Character");
		desc.SetDescription("Creates characters");
		desc.AddMember(&CCharacterComponent::m_collisionType, 'col', "CollisionType", "Collision Type", "Determines collision id used for masking", ECollisionType::Enemy);
		desc.AddMember(&CCharacterComponent::m_moveSpeed, 'move', "MoveSpeed", "Move Speed", "Determines base move speed", 3.0f);
		desc.AddMember(&CCharacterComponent::m_runMultiplier, 'run', "RunMultiplier", "Run Multiplier", "Multiplies movespeed to get run speed", 2.0f);
		desc.AddMember(&CCharacterComponent::m_effectorJointName, 'ejn', "EffectorJointName", "Effector Joint Name", "Base joint of the character for ik orientation", "armature");
		desc.AddMember(&CCharacterComponent::m_aimJointName, 'ajn', "AimJointName", "Aim Joint Name", "Aiming joint for ik based movement", "mixamorig:spine");
	}

protected:
	virtual void Initialize() override;
	virtual Cry::Entity::EventFlags GetEventMask() const override;
	virtual void ProcessEvent(const SEntityEvent& event) override;

private:
	void AttachWeapon();
	void ClearAttachBinding(const char* szName);
	void HandleWeaponFired();
	void SetIKJoints();
	void SetCollisionParams();

	IEntity* m_pOwner;
	ECollisionType m_collisionType;

	Cry::DefaultComponents::CAdvancedAnimationComponent* m_pAnimComp;
	Cry::DefaultComponents::CCharacterControllerComponent* m_pCharController;
	CInterfaceComponent* m_pInterfaceComp;
	CBodyDamageComponent* m_pBodyDamageComp;

	Schematyc::CSharedString m_effectorJointName = "armature";
	Schematyc::CSharedString m_aimJointName = "mixamorig:spine";
	std::shared_ptr<CIKTorsoAim> m_ikTorsoAim;

	CWeaponComponent* m_pActiveWeapon;
	std::vector<CWeaponComponent*> m_weapons;

	float m_moveSpeed = 3.0f;
	float m_runMultiplier = 2.0f;

	bool m_isShooting;
	bool m_isRunning;
	bool m_hasJumped;
	bool m_isFalling;

	int m_jumpFrameId;

	//We hold ammo based on weaponname
	std::map<string, int> m_ammoMap;

	//TODO: Replace this with map use.
	int m_totalAmmo = 50;
};