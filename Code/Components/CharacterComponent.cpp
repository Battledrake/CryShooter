#include "StdAfx.h"
#include "CharacterComponent.h"

#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <CrySchematyc/Env/Elements/EnvComponent.h>
#include <CryCore/StaticInstanceList.h>
#include <CryExtension/CryCreateClassInstance.h>
#include <Animation/PoseModifier/IKTorsoAim.h>

#include "Components/TempPlayer.h"
#include "Components/BodyDamageComponent.h"
#include "Components/EquipmentComponent.h"
#include "Components/WeaponComponent.h"
#include "Components/ProjectileComponent.h"
#include "Helpers/Conversions.h"


namespace
{
	static void RegisterCharacterComponent(Schematyc::IEnvRegistrar& registrar)
	{
		Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
		{
			Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CCharacterComponent));
			{
			}
		}
	}
	CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterCharacterComponent)
}

void CCharacterComponent::Initialize()
{
	m_pAnimComp = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CAdvancedAnimationComponent>();
	m_pCharController = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CCharacterControllerComponent>();
	m_pEquipmentComp = m_pEntity->GetOrCreateComponent<CEquipmentComponent>();
	m_pInterfaceComp = m_pEntity->GetOrCreateComponent<CInterfaceComponent>();
	m_pBodyDamageComp = m_pEntity->GetOrCreateComponent<CBodyDamageComponent>();

	CryCreateClassInstanceForInterface(cryiidof<IAnimationPoseModifierTorsoAim>(), m_ikTorsoAim);
}

Cry::Entity::EventFlags CCharacterComponent::GetEventMask() const
{
	return
		Cry::Entity::EEvent::GameplayStarted |
		Cry::Entity::EEvent::Update |
		Cry::Entity::EEvent::Reset;
}

void CCharacterComponent::ProcessEvent(const SEntityEvent& event)
{
	switch (event.event)
	{
		case Cry::Entity::EEvent::GameplayStarted:
		{
			m_pInterfaceComp->AddInterface<ITakeDamage>(this);

			SetIKJoints();
			SetCollisionParams();
		}
		break;
		case Cry::Entity::EEvent::Update:
		{
			if (!m_pCharController->IsOnGround() && !m_hasJumped && !m_isFalling)
			{
				m_isFalling = true;
				m_pAnimComp->QueueFragment("Jump");
			}
			if (m_pCharController->IsOnGround() && (m_hasJumped || m_isFalling) && m_jumpFrameId != gEnv->pRenderer->GetFrameID())
			{
				m_isFalling = false;
				m_hasJumped = false;
				m_pAnimComp->QueueFragment("Locomotion");
			}
		}
		break;
		case Cry::Entity::EEvent::Reset:
		{
			if (m_pActiveWeapon)
			{
				ClearAttachBinding(EnumToString(m_pActiveWeapon->GetWeaponType()));
				m_pActiveWeapon->GetEntity()->EnablePhysics(true);
				m_pActiveWeapon = nullptr;
			}

			m_pAnimComp->ResetCharacter();
			m_pAnimComp->QueueFragment("Locomotion");
		}
		break;
	}
}

// void CCharacterComponent::AddAmmo(string weaponName, int amount)
// {
// 	if (m_pActiveWeapon)
// 	{
// 		if (strcmp(m_pActiveWeapon->GetEquipmentName(), weaponName) == 0)
// 		{
// 			m_pActiveWeapon->GiveAmmo(amount);
//  			m_ammoAddedEvent.Invoke(m_pActiveWeapon->GetAmmoCount());
// 		}
// 	}
// }

void CCharacterComponent::ChangeCharacter(const Schematyc::CharacterFileName charFile, const Schematyc::CSharedString context)
{
	if (m_pActiveWeapon)
	{
		m_pActiveWeapon->DisableFiring();
		ClearAttachBinding(EnumToString(m_pActiveWeapon->GetWeaponType()));
	}


	m_pAnimComp->SetCharacterFile(charFile.value);
	if (context.length() > 0)
		m_pAnimComp->ActivateContext(context);

	SetIKJoints();
	SetCollisionParams();

	m_pAnimComp->QueueFragment("Locomotion");

	if (m_pActiveWeapon)
	{
		AttachWeapon();
	}
}

void CCharacterComponent::ProcessJump()
{
	if (m_pCharController->IsOnGround())
	{
		if (m_pActiveWeapon)
			m_pActiveWeapon->ProcessFire(false);

		m_pCharController->ChangeVelocity(Vec3(0.0f, 0.0f, 5.0f), Cry::DefaultComponents::CCharacterControllerComponent::EChangeVelocityMode::Jump);
		m_pAnimComp->QueueFragment("Jump");
		m_hasJumped = true;
		m_jumpFrameId = gEnv->pRenderer->GetFrameID();
	}
}

void CCharacterComponent::ProcessFire(bool isPressed)
{
	if (m_pActiveWeapon)
	{
		if (!m_isRunning && m_pCharController->IsOnGround())
		{
			m_pActiveWeapon->ProcessFire(isPressed);
			m_isShooting = isPressed;
		}
		else
		{
			m_pActiveWeapon->DisableFiring();
		}
	}
}

void CCharacterComponent::ProcessReload()
{
	if (m_pActiveWeapon)
	{
		m_pActiveWeapon->DisableFiring();

		//TODO: Mebbe reload animation and have this called on animevent.
		m_pActiveWeapon->Reload();

		m_reloadEvent.Invoke(m_pActiveWeapon->GetClipCount());
	}
}

void CCharacterComponent::ProcessSprinting(bool isPressed)
{
	if (!m_pCharController->IsOnGround())
	{
		m_isRunning = false;
		return;
	}

	if (isPressed)
	{
		if (m_pActiveWeapon)
			m_pActiveWeapon->DisableFiring();
		m_isRunning = true;
	}
	else
	{
		m_isRunning = false;
	}
}

void CCharacterComponent::SwitchFireMode()
{
	if (!m_pActiveWeapon)
		return;

	EFireMode fireMode = m_pActiveWeapon->SwitchFireModes();
	m_switchFireModeEvent.Invoke(EnumToString(fireMode));
}

void CCharacterComponent::SetActiveWeapon(CWeaponComponent* pWeapon)
{
	if (m_pActiveWeapon)
	{
		m_pAnimComp->SetTag(EnumToString(pWeapon->GetWeaponType()), false);
		ClearAttachBinding(EnumToString(m_pActiveWeapon->GetWeaponType()));
		m_pActiveWeapon->m_fireEvent.RemoveListener();
		m_pActiveWeapon->m_recoilEvent.RemoveListener();
		m_pActiveWeapon->m_ammoChangedEvent.RemoveListener();
	}
	m_pActiveWeapon = pWeapon;
	m_pActiveWeapon->GetEntity()->EnablePhysics(false);
	AttachWeapon();

	m_equipEvent.Invoke(m_pActiveWeapon->GetIconPath(), EnumToString(m_pActiveWeapon->GetFireMode()),
		m_pActiveWeapon->GetClipCount(), m_pActiveWeapon->GetClipCapacity(),
		m_pActiveWeapon->GetAmmoCount());

	m_pActiveWeapon->m_fireEvent.RegisterListener(std::bind(&CCharacterComponent::HandleWeaponFired, this));
	m_pActiveWeapon->m_recoilEvent.RegisterListener([this](Vec2 recoil)
	{
		if (CTempPlayerComponent* pPlayer = m_pOwner->GetComponent<CTempPlayerComponent>())
			pPlayer->AddRecoilEffect(recoil);
	});
	m_pActiveWeapon->m_ammoChangedEvent.RegisterListener([this](int ammoCount) { m_ammoChangedEvent.Invoke(ammoCount); });

	m_pAnimComp->SetTag(EnumToString(m_pActiveWeapon->GetWeaponType()), true);
	m_pAnimComp->QueueFragment("Locomotion");
}

void CCharacterComponent::HandleWeaponFired()
{
	int aimJoint = m_pAnimComp->GetCharacter()->GetIDefaultSkeleton().GetJointIDByName(m_aimJointName.c_str());
	const QuatT& jointQuat = m_pAnimComp->GetCharacter()->GetISkeletonPose()->GetAbsJointByID(aimJoint);
	Matrix34 charMatrix = m_pEntity->GetWorldTM();
	charMatrix.AddTranslation(charMatrix.TransformVector(jointQuat.t));
	charMatrix.SetRotation33(Matrix33(m_pOwner->GetWorldRotation()));

	Vec3 ownerOrigin = m_pOwner->GetWorldPos();
	Vec3 charOrigin = m_pEntity->GetWorldPos() + Vec3(0.0f, 0.0f, 1.6f);
	const float distance = 100.0f;
	Vec3 ownerDir = m_pOwner->GetForwardDir() * distance;
	Vec3 charDir = charMatrix.GetColumn1() * distance;
	const unsigned int rayFlags = rwi_stop_at_pierceable | rwi_colltype_any;
	ray_hit ownerHitInfo;
	ray_hit charHitInfo;

	SEntitySpawnParams projectileParams;
	projectileParams.pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(m_pActiveWeapon->GetProjectileClass());
	projectileParams.vPosition = ownerOrigin;

	if (gEnv->pPhysicalWorld->RayWorldIntersection(ownerOrigin, ownerDir, ent_all, rayFlags, &ownerHitInfo, 1, m_pOwner->GetPhysicalEntity()))
	{
		charDir = (ownerHitInfo.pt - charOrigin).normalized() * distance;
	}
	if (gEnv->pPhysicalWorld->RayWorldIntersection(charOrigin, charDir, ent_all, rayFlags, &charHitInfo, 1, m_pEntity->GetPhysicalEntity()))
	{
		CCharacterComponent* pCharComp = nullptr;
		if (IEntity* pEntity = gEnv->pEntitySystem->GetEntityFromPhysics(charHitInfo.pCollider))
		{
			pCharComp = pEntity->GetComponent<CCharacterComponent>();
		}
		if (ownerHitInfo.pCollider != charHitInfo.pCollider && !pCharComp)
		{
			projectileParams.vPosition = charOrigin;
		}
	}

	projectileParams.qRotation = m_pOwner->GetWorldRotation();
	const float bulletScale = 0.05f;
	projectileParams.vScale = Vec3(bulletScale);
	if (IEntity* pProjectileEntity = gEnv->pEntitySystem->SpawnEntity(projectileParams))
	{
		if (CProjectileComponent* pProjectileComp = pProjectileEntity->GetComponent<CProjectileComponent>())
			pProjectileComp->InitializeProjectile(this);
	}

	m_wepFiredEvent.Invoke(m_pActiveWeapon->GetClipCount());
}

void CCharacterComponent::UpdateMovement(float travelSpeed, float travelAngle)
{
	Vec3 velocity(travelAngle, travelSpeed, 0.0f);
	velocity.Normalize();
	velocity.z = 0.0f;

	const float runMultiplier = m_isRunning && travelSpeed > 0 ? m_runMultiplier : 1.0f;
	const float backWalk = travelSpeed < 0 ? 0.75f : 1.0f;

	m_pCharController->SetVelocity(m_pEntity->GetWorldRotation() * velocity * m_moveSpeed * runMultiplier * backWalk);
}

void CCharacterComponent::UpdateRotation(const Quat& rotation)
{
	m_pEntity->SetRotation(rotation);
}

void CCharacterComponent::UpdateLookOrientation(const Vec3& lookDirection)
{

	if (ISkeletonAnim* pSkelAnim = m_pAnimComp->GetCharacter()->GetISkeletonAnim())
	{
		Quat localRot = !m_pEntity->GetWorldRotation();
		m_ikTorsoAim->SetTargetDirection(localRot * lookDirection);
		m_ikTorsoAim->SetBlendWeight(1.0f);
		pSkelAnim->PushPoseModifier(0, m_ikTorsoAim);
	}
}

void CCharacterComponent::ClearAttachBinding(const char* szName)
{
	if (IAttachment* pAttach = m_pAnimComp->GetCharacter()->GetIAttachmentManager()->GetInterfaceByName(szName))
	{
		pAttach->ClearBinding();
	}
}

void CCharacterComponent::AttachWeapon()
{
	m_pAnimComp->SetTag(EnumToString(m_pActiveWeapon->GetWeaponType()), true);

	if (IAttachment* pAttach = m_pAnimComp->GetCharacter()->GetIAttachmentManager()->GetInterfaceByName(EnumToString(m_pActiveWeapon->GetWeaponType())))
	{
		CEntityAttachment* pWepAttach = new CEntityAttachment();
		pWepAttach->SetEntityId(m_pActiveWeapon->GetEntityId());
		pAttach->AddBinding(pWepAttach);
	}
}

void CCharacterComponent::SetIKJoints()
{
	int effectorJointId = m_pAnimComp->GetCharacter()->GetIDefaultSkeleton().GetJointIDByName(m_effectorJointName.c_str());
	int aimJointId = m_pAnimComp->GetCharacter()->GetIDefaultSkeleton().GetJointIDByName(m_aimJointName.c_str());
	m_ikTorsoAim->SetJoints(effectorJointId, aimJointId);
}

//This has to be set everytime we switch from first and third person, since their physical entities are different.
//Could separate the entity, since that only needs to be set once, but i'd prefer not to have two places with similar code.
void CCharacterComponent::SetCollisionParams()
{
	pe_params_collision_class params;
	params.collisionClassOR.type = static_cast<uint32>(m_collisionType);
	params.collisionClassOR.ignore = static_cast<uint32>(m_collisionType);
	m_pEntity->GetPhysicalEntity()->SetParams(&params);
	m_pAnimComp->GetCharacter()->GetPhysEntity()->SetParams(&params);
}

void CCharacterComponent::PassDamage(float amount, int partId)
{
	if (m_pBodyDamageComp)
	{
		m_pBodyDamageComp->CalculateDamage(amount, partId);
	}
}
