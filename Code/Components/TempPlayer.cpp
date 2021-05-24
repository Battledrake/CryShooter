#include "StdAfx.h"
#include "TempPlayer.h"

#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <CrySchematyc/Env/Elements/EnvComponent.h>
#include <CryCore/StaticInstanceList.h>
#include <FlashUI/FlashUI.h>

#include "Components/CharacterComponent.h"
#include "Interfaces/IInteractable.h"

namespace
{
	static void RegisterTempPlayerComponent(Schematyc::IEnvRegistrar& registrar)
	{
		Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
		{
			Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CTempPlayerComponent));
		}
	}
	CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterTempPlayerComponent);
}

void CTempPlayerComponent::Initialize()
{
	m_inputFlags.Clear();
	m_mouseDeltaRotation = ZERO;

	m_pCameraComponent = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CCameraComponent>();
	m_pAudioListenerComponent = m_pEntity->GetOrCreateComponent<Cry::Audio::DefaultComponents::CListenerComponent>();
	m_pInputComponent = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CInputComponent>();

	m_pCameraComponent->SetNearPlane(0.01f);

	RegisterInputs();

	m_pInteractableUI = gEnv->pFlashUI->GetUIElement("InteractableText");
}

Cry::Entity::EventFlags CTempPlayerComponent::GetEventMask() const
{
	return
		Cry::Entity::EEvent::GameplayStarted |
		Cry::Entity::EEvent::Update |
		Cry::Entity::EEvent::Reset;
}

void CTempPlayerComponent::ProcessEvent(const SEntityEvent& event)
{
	switch (event.event)
	{
		case Cry::Entity::EEvent::GameplayStarted:
		{
			//TODO:This has to be placed here because the component gets a reference to the character.
			m_pUIComponent = m_pEntity->GetOrCreateComponent<CUIComponent>();

			m_lookOrientation = IDENTITY;
		}
		break;
		case Cry::Entity::EEvent::Update:
		{
			const float frameTime = event.fParam[0];

			const float spectatorSpeed = 20.5f;
			Vec3 velocity = ZERO;

			float travelSpeed = 0;
			float travelAngle = 0;

			// Check input to calculate local space velocity
			if (m_inputFlags & EInputFlag::MoveLeft)
			{
				travelAngle -= 1.0f;
			}
			if (m_inputFlags & EInputFlag::MoveRight)
			{
				travelAngle += 1.0f;
			}
			if (m_inputFlags & EInputFlag::MoveForward)
			{
				travelSpeed += 1.0f;
			}
			if (m_inputFlags & EInputFlag::MoveBack)
			{
				travelSpeed -= 1.0f;
			}

			const float rotationSpeed = 0.002f;
			const float rotationLimitsMinPitch = -0.84f;
			const float rotationLimitsMaxPitch = 1.5f;

			m_mouseDeltaRotation = m_mouseDeltaSmoothingFilter.Push(m_mouseDeltaRotation).Get();

			Ang3 playerYPR = CCamera::CreateAnglesYPR(Matrix33(m_pEntity->GetLocalTM()));
			playerYPR.y = CLAMP(playerYPR.y + m_mouseDeltaRotation.y * rotationSpeed, rotationLimitsMinPitch, rotationLimitsMaxPitch);
			playerYPR.x += m_currentViewMode == EViewMode::Spectator ? m_mouseDeltaRotation.x * rotationSpeed : 0.0f;
			playerYPR.z = 0;

			Quat camOrientation = Quat(CCamera::CreateOrientationYPR(playerYPR));
			m_pEntity->SetRotation(camOrientation);

			if (m_currentViewMode == EViewMode::Spectator)
			{
				velocity = Vec3(travelAngle, travelSpeed, 0).normalized();
				velocity.z = 0;
				Matrix34 transformation = m_pEntity->GetWorldTM();
				transformation.AddTranslation(transformation.TransformVector(velocity * spectatorSpeed * frameTime));
				m_pEntity->SetWorldTM(transformation);
			}

			if (m_pCharacter && m_currentViewMode != EViewMode::Spectator)
			{
				UpdateCharacter(travelSpeed, travelAngle, rotationSpeed, frameTime);
			}

			m_mouseDeltaRotation = ZERO;

			CheckInteractables();
		}
		break;
		case Cry::Entity::EEvent::Reset:
		{
		}
		break;
	}
}

void CTempPlayerComponent::UpdateCharacter(float travelSpeed, float travelAngle, float rotationSpeed, float frameTime)
{
	Ang3 characterYPR = CCamera::CreateAnglesYPR(Matrix33(m_lookOrientation));
	characterYPR.x += m_mouseDeltaRotation.x * rotationSpeed;
	characterYPR.z = 0;
	characterYPR.y = 0;
	m_lookOrientation = Quat(CCamera::CreateOrientationYPR(characterYPR));

	m_pCharacter->UpdateRotation(m_lookOrientation);
	m_pCharacter->UpdateMovement(travelSpeed, travelAngle);
	m_pCharacter->UpdateLookOrientation(m_pEntity->GetForwardDir());

	const QuatT& entityOrientation = m_pCharacter->GetAnimComp()->GetCharacter()->GetISkeletonPose()->GetAbsJointByID(m_attachJointId);
	m_pEntity->SetPos(LERP(m_pEntity->GetPos(), entityOrientation.t + m_activePos, 5.0f * frameTime));
}

void CTempPlayerComponent::SetCharacter(CCharacterComponent* character)
{
	m_pCharacter = character;
	m_attachJointId = m_pCharacter->GetAnimComp()->GetCharacter()->GetIDefaultSkeleton().GetJointIDByName("mixamorig:head");
	m_activePos = m_thirdPersonPos;
	m_currentViewMode = EViewMode::ThirdPerson;
}

void CTempPlayerComponent::CheckInteractables()
{
	Vec3 origin = m_pEntity->GetWorldPos();
	Vec3 direction = m_pEntity->GetForwardDir();
	const unsigned int rayFlags = rwi_stop_at_pierceable | rwi_colltype_any;
	ray_hit hitInfo;
	if (gEnv->pPhysicalWorld->RayWorldIntersection(origin, direction * 3.0f, ent_all, rayFlags, &hitInfo, 1, m_pCharacter->GetEntity()->GetPhysicalEntity()))
	{
		if (IEntity* pHitEnt = gEnv->pEntitySystem->GetEntityFromPhysics(hitInfo.pCollider))
		{
			if (CInterfaceComponent* pInterfaceComp = pHitEnt->GetComponent<CInterfaceComponent>())
			{
				if (IInteractable* pInteractable = pInterfaceComp->GetInterface<IInteractable>())
				{
					if (m_pActiveInteractable != pInteractable)
					{
						SObjectData objData;
						pInteractable->Observe(m_pCharacter, objData);
						m_pActiveInteractable = pInteractable;

						string combinedString = objData.objectKeyword + " " + objData.objectName + " " + objData.objectBonus;
						m_pInteractableUI->CallFunction("UpdateText", SUIArguments::Create(combinedString));
						m_pInteractableUI->SetVisible(true);
						return;
					}
					return;
				}
			}
		}
	}

	m_pInteractableUI->SetVisible(false);
	m_pActiveInteractable = nullptr;
}

void CTempPlayerComponent::RegisterInputs()
{

	// Register an action, and the callback that will be sent when it's triggered
	m_pInputComponent->RegisterAction("player", "moveleft", [this](int activationMode, float value) { HandleInputFlagChange(EInputFlag::MoveLeft, (EActionActivationMode)activationMode); });
	// Bind the 'A' key the "moveleft" action
	m_pInputComponent->BindAction("player", "moveleft", eAID_KeyboardMouse, EKeyId::eKI_A);

	m_pInputComponent->RegisterAction("player", "moveright", [this](int activationMode, float value) { HandleInputFlagChange(EInputFlag::MoveRight, (EActionActivationMode)activationMode); });
	m_pInputComponent->BindAction("player", "moveright", eAID_KeyboardMouse, EKeyId::eKI_D);

	m_pInputComponent->RegisterAction("player", "moveforward", [this](int activationMode, float value) { HandleInputFlagChange(EInputFlag::MoveForward, (EActionActivationMode)activationMode); });
	m_pInputComponent->BindAction("player", "moveforward", eAID_KeyboardMouse, EKeyId::eKI_W);

	m_pInputComponent->RegisterAction("player", "moveback", [this](int activationMode, float value) { HandleInputFlagChange(EInputFlag::MoveBack, (EActionActivationMode)activationMode); });
	m_pInputComponent->BindAction("player", "moveback", eAID_KeyboardMouse, EKeyId::eKI_S);

	m_pInputComponent->RegisterAction("player", "mouse_rotateyaw", [this](int activationMode, float value) { m_mouseDeltaRotation.x -= value; });
	m_pInputComponent->BindAction("player", "mouse_rotateyaw", eAID_KeyboardMouse, EKeyId::eKI_MouseX);

	m_pInputComponent->RegisterAction("player", "mouse_rotatepitch", [this](int activationMode, float value) { m_mouseDeltaRotation.y -= value; });
	m_pInputComponent->BindAction("player", "mouse_rotatepitch", eAID_KeyboardMouse, EKeyId::eKI_MouseY);

	m_pInputComponent->RegisterAction("player", "jump", [this](int activationMode, float value)
	{
		if (m_pCharacter)
			m_pCharacter->ProcessJump();
	});
	m_pInputComponent->BindAction("player", "jump", eAID_KeyboardMouse, eKI_Space, true, false, false);

	m_pInputComponent->RegisterAction("player", "changeviewmode", [this](int activationMode, float value)
	{
		if (m_currentViewMode == EViewMode::Spectator)
			return;

		m_currentViewMode = m_currentViewMode == EViewMode::ThirdPerson ? EViewMode::FirstPerson : EViewMode::ThirdPerson;
		switch (m_currentViewMode)
		{
			case EViewMode::FirstPerson:
			{
				m_pCharacter->ChangeCharacter("Objects/Characters/exo_swat/exo_swat_1p.cdf", "FirstPersonCharacter");
				m_activePos = m_firstPersonPos;
				m_attachJointId = m_pCharacter->GetAnimComp()->GetCharacter()->GetIDefaultSkeleton().GetJointIDByName("mixamorig:head");
			}
			break;
			case EViewMode::ThirdPerson:
			{
				m_pCharacter->ChangeCharacter("Objects/Characters/exo_swat/exo_swat_3p.cdf", "ThirdPersonCharacter");
				m_activePos = m_thirdPersonPos;
				m_attachJointId = m_pCharacter->GetAnimComp()->GetCharacter()->GetIDefaultSkeleton().GetJointIDByName("mixamorig:head");
			}
			break;
		}
	});
	m_pInputComponent->BindAction("player", "changeviewmode", eAID_KeyboardMouse, eKI_F, true, false, false);

	m_pInputComponent->RegisterAction("player", "interact", [this](int activationMode, float value)
	{
		if (!m_pCharacter || m_currentViewMode == EViewMode::Spectator)
			return;

		if (m_pActiveInteractable)
		{
			m_pActiveInteractable->Interact(m_pCharacter);
		}
	});
	m_pInputComponent->BindAction("player", "interact", eAID_KeyboardMouse, eKI_E, true, false, false);

	m_pInputComponent->RegisterAction("player", "firebutton", [this](int activationMode, float value)
	{
		if (m_currentViewMode == EViewMode::Spectator)
			return;

		if (m_pCharacter)
		{
			switch (activationMode)
			{
				case EActionActivationMode::eAAM_OnPress:
					m_pCharacter->ProcessFire(true);
					break;
				case EActionActivationMode::eAAM_OnRelease:
					m_pCharacter->ProcessFire(false);
					break;
				default:
					break;
			}
		}
	});
	m_pInputComponent->BindAction("player", "fireButton", eAID_KeyboardMouse, eKI_Mouse1, true, true, false);

	m_pInputComponent->RegisterAction("player", "reloadweapon", [this](int activationMode, float value)
	{
		if (m_currentViewMode == EViewMode::Spectator)
			return;

		if (m_pCharacter)
		{
			m_pCharacter->ProcessReload();
		}
	});
	m_pInputComponent->BindAction("player", "reloadweapon", eAID_KeyboardMouse, eKI_R, true, false, false);

	m_pInputComponent->RegisterAction("player", "switchfiremode", [this](int activationMode, float value)
	{
		if (m_currentViewMode == EViewMode::Spectator)
			return;

		if (m_pCharacter)
		{
			m_pCharacter->SwitchFireMode();
		}
	});
	m_pInputComponent->BindAction("player", "switchfiremode", eAID_KeyboardMouse, eKI_X, true, false, false);

	m_pInputComponent->RegisterAction("player", "sprint", [this](int activationMode, float value)
	{
		if (m_currentViewMode == EViewMode::Spectator)
			return;

		switch (activationMode)
		{
			case EActionActivationMode::eAAM_OnPress:
				if (m_pCharacter)
				{
					m_pCharacter->ProcessSprinting(true);
				}
				break;
			case EActionActivationMode::eAAM_OnRelease:
				if (m_pCharacter)
				{
					m_pCharacter->ProcessSprinting(false);
				}
				break;
			default:
				break;
		}
	});
	m_pInputComponent->BindAction("player", "sprint", eAID_KeyboardMouse, eKI_LShift, true, true, false);

	m_pInputComponent->RegisterAction("player", "spectate", [this](int activationMode, float value)
	{
		if (m_currentViewMode == EViewMode::Spectator)
		{
			if (m_pCharacter)
			{
				m_pCharacter->GetEntity()->AttachChild(m_pEntity);
				m_pEntity->SetLocalTM(Matrix34::Create(Vec3(1.0f), IDENTITY, Vec3(0.5f, -1.5f, 1.75f)));
				m_currentViewMode = EViewMode::ThirdPerson;
			}
		}
		else
		{
			if (m_pCharacter)
				m_pEntity->DetachThis();
			m_currentViewMode = EViewMode::Spectator;
		}
	});
	m_pInputComponent->BindAction("player", "spectate", eAID_KeyboardMouse, eKI_F1, true, false, false);
}

void CTempPlayerComponent::HandleInputFlagChange(const CEnumFlags<EInputFlag> flags, const CEnumFlags<EActionActivationMode> activationMode, const EInputFlagType type)
{
	switch (type)
	{
		case EInputFlagType::Hold:
		{
			if (activationMode == eAAM_OnRelease)
			{
				m_inputFlags &= ~flags;
			}
			else
			{
				m_inputFlags |= flags;
			}
		}
		break;
		case EInputFlagType::Toggle:
		{
			if (activationMode == eAAM_OnRelease)
			{
				// Toggle the bit(s)
				m_inputFlags ^= flags;
			}
		}
		break;
	}
}