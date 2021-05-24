#include "StdAfx.h"
#include "CharacterComponent.h"

#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <CrySchematyc/Env/Elements/EnvComponent.h>
#include <CryCore/StaticInstanceList.h>
#include <CryExtension/CryCreateClassInstance.h>
#include <Animation/PoseModifier/IKTorsoAim.h>

#include "Helpers/Conversions.h"
#include "BodyDamageComponent.h"
#include "WeaponComponent.h"
#include "TempPlayer.h"


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
	m_pAnimComp = m_pEntity->GetComponent<Cry::DefaultComponents::CAdvancedAnimationComponent>();
	m_pCharController = m_pEntity->GetComponent<Cry::DefaultComponents::CCharacterControllerComponent>();
	m_pCharController->GetMovementParameters().m_airControlRatio = 1.0f;
	m_pInterfaceComp = m_pEntity->GetOrCreateComponent<CInterfaceComponent>();
	m_pBodyDamageComp = m_pEntity->GetComponent<CBodyDamageComponent>();

	CryCreateClassInstanceForInterface(cryiidof<IAnimationPoseModifierTorsoAim>(), m_ikTorsoAim);
}

Cry::Entity::EventFlags CCharacterComponent::GetEventMask() const
{
	return
		Cry::Entity::EEvent::Initialize |
		Cry::Entity::EEvent::GameplayStarted |
		Cry::Entity::EEvent::Update |
		Cry::Entity::EEvent::Reset;
}

void CCharacterComponent::ProcessEvent(const SEntityEvent& event)
{
	switch (event.event)
	{
		case Cry::Entity::EEvent::Initialize:
		{
		}
		break;
		case Cry::Entity::EEvent::GameplayStarted:
		{
			m_pInterfaceComp->AddInterface<ITakeDamage>(this);

			SetIKJoints();
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
			ClearEquipBinding("rifle");
			if (m_pActiveWeapon)
			{
				m_pActiveWeapon->GetEntity()->EnablePhysics(true);
				m_pActiveWeapon = nullptr;
			}
			m_totalAmmo = 50;

			m_pAnimComp->ResetCharacter();
		}
		break;
	}
}

void CCharacterComponent::UpdateMovement(float travelSpeed, float travelAngle)
{
	Vec3 velocity(travelAngle, travelSpeed, 0.0f);
	velocity.Normalize();
	velocity.z = 0.0f;

	const float runMultiplier = m_isRunning && travelSpeed > 0 ? m_runMultiplier : 1.0f;

	m_pCharController->SetVelocity(m_pEntity->GetWorldRotation() * velocity * m_moveSpeed * runMultiplier);
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
		//TODO: This code works for players, but will fail with AI. WHY AM I NOT USING LOOKDIRECTION! EUGH, I knew I would need it, why I no utilize?!?.
		Vec3 camDirection = m_pEntity->GetChild(0)->GetWorldPos() + m_pEntity->GetChild(0)->GetForwardDir() * 10.0f;
		Vec3 entPos = m_pEntity->GetPos() + Vec3(0.0f, 0.0f, 1.5f);
		Vec3 finalDirection = camDirection - entPos;
		m_ikTorsoAim->SetTargetDirection(localRot * finalDirection);
		m_ikTorsoAim->SetBlendWeight(.6f);
		pSkelAnim->PushPoseModifier(0, m_ikTorsoAim);
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

		const int clipCapacity = m_pActiveWeapon->GetClipCapacity();
		const int clipCount = m_pActiveWeapon->GetClipCount();
		const int clipSpace = clipCapacity - clipCount;
		const int freeAmmo = m_totalAmmo - clipCount;
		const int fill = freeAmmo - clipSpace >= 0 ? clipSpace : freeAmmo;
		m_pActiveWeapon->ReloadClip(fill);

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

void CCharacterComponent::EquipWeapon(CWeaponComponent* weapon)
{
	if (m_pActiveWeapon)
	{
		m_pActiveWeapon->DisableFiring();

		ClearEquipBinding("rifle");

		m_totalAmmo -= m_pActiveWeapon->GetClipCount();

		m_pActiveWeapon->GetEntity()->EnablePhysics(true);

		pe_action_impulse impulseAction;
		const float initialVelocity = 50.0f;
		impulseAction.impulse = m_pEntity->GetForwardDir() * initialVelocity;
		impulseAction.angImpulse = Vec3(initialVelocity, 0, 0);
		m_pActiveWeapon->GetEntity()->GetPhysicalEntity()->Action(&impulseAction);

		m_pActiveWeapon->m_fireEvent.RemoveListener();
		m_pActiveWeapon->m_recoilEvent.RemoveListener();
		m_pActiveWeapon = nullptr;
	}


	weapon->GetEntity()->EnablePhysics(false);
	m_pActiveWeapon = weapon;
	AttachWeapon();

	m_totalAmmo += m_pActiveWeapon->GetClipCount();

	m_equipEvent.Invoke(m_pActiveWeapon->GetIconName(), EnumToString(m_pActiveWeapon->GetFireMode()),
		m_pActiveWeapon->GetClipCount(), m_pActiveWeapon->GetClipCapacity(), m_totalAmmo);

	m_pActiveWeapon->m_fireEvent.RegisterListener([this]()
	{
		m_totalAmmo--;
		m_wepFiredEvent.Invoke(m_pActiveWeapon->GetClipCount(), m_totalAmmo);
	});
	m_pActiveWeapon->m_recoilEvent.RegisterListener([this](Vec2 recoil)
	{
		if (CTempPlayerComponent* pPlayer = m_pOwner->GetComponent<CTempPlayerComponent>())
			pPlayer->AddRecoilEffect(recoil);
	});

	m_pAnimComp->SetTag(EnumToString(m_pActiveWeapon->GetAnimationType()), true);
	m_pAnimComp->QueueFragment("Locomotion");
}

void CCharacterComponent::ChangeCharacter(const Schematyc::CharacterFileName charFile, const Schematyc::CSharedString context)
{
	if (m_pActiveWeapon)
		m_pActiveWeapon->DisableFiring();

	ClearEquipBinding("rifle");

	m_pAnimComp->SetCharacterFile(charFile.value);
	if (context.length() > 0)
		m_pAnimComp->ActivateContext(context);

	SetIKJoints();

	m_pAnimComp->QueueFragment("Locomotion");

	if (m_pActiveWeapon)
	{
		AttachWeapon();
	}
}

void CCharacterComponent::AttachWeapon()
{
	m_pAnimComp->SetTag(EnumToString(m_pActiveWeapon->GetAnimationType()), true);

	if (IAttachment* pAttach = m_pAnimComp->GetCharacter()->GetIAttachmentManager()->GetInterfaceByName("rifle"))
	{
		CEntityAttachment* pWepAttach = new CEntityAttachment();
		pWepAttach->SetEntityId(m_pActiveWeapon->GetEntityId());
		pAttach->AddBinding(pWepAttach);
	}
}

void CCharacterComponent::ClearEquipBinding(const char* szName)
{
	if (IAttachment* pAttach = m_pAnimComp->GetCharacter()->GetIAttachmentManager()->GetInterfaceByName(szName))
	{
		pAttach->ClearBinding();
	}
}

void CCharacterComponent::SetIKJoints()
{
	int effectorJointId = m_pAnimComp->GetCharacter()->GetIDefaultSkeleton().GetJointIDByName(m_effectorJointName.c_str());
	int aimJointId = m_pAnimComp->GetCharacter()->GetIDefaultSkeleton().GetJointIDByName(m_aimJointName.c_str());
	m_ikTorsoAim->SetJoints(effectorJointId, aimJointId);
}

void CCharacterComponent::PassDamage(float amount, int partId)
{
	if (m_pBodyDamageComp)
	{
		m_pBodyDamageComp->CalculateDamage(amount, partId);
	}
}
