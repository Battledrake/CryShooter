#include "StdAfx.h"
#include "Player.h"
#include "SpawnPoint.h"
#include "GamePlugin.h"

#include <CryRenderer/IRenderAuxGeom.h>
#include <CrySchematyc/Env/Elements/EnvComponent.h>
#include <CryCore/StaticInstanceList.h>
#include <CryExtension/CryCreateClassInstance.h>
#include <Animation/PoseModifier/IKTorsoAim.h>
#include <CryNetwork/Rmi.h>
#include <FlashUI/FlashUI.h>

#define MOUSE_DELTA_TRESHOLD 0.0001f

namespace
{
	static void RegisterPlayerComponent(Schematyc::IEnvRegistrar& registrar)
	{
		Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
		{
			Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CPlayerComponent));
		}
	}
	CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterPlayerComponent);
}

void CPlayerComponent::Initialize()
{

	//  	// The character controller is responsible for maintaining player physics
	m_pCharacterController = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CCharacterControllerComponent>();
	//  	// Offset the default character controller up by one unit
	m_pCharacterController->SetTransformMatrix(Matrix34::Create(Vec3(1.f), IDENTITY, Vec3(0, 0, 1.f)));
	m_pCharacterController->GetMovementParameters().m_airControlRatio = 1.0f;

	m_currentView = EPersonViewState::First;

	CryCreateClassInstanceForInterface(cryiidof<IAnimationPoseModifierTorsoAim>(), m_ikTorsoAim);

	// Create the advanced animation component, responsible for updating Mannequin and animating the player
	m_pAnimationComponent = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CAdvancedAnimationComponent>();

	// Set the player geometry, this also triggers physics proxy creation
	m_pAnimationComponent->SetMannequinAnimationDatabaseFile("Animations/Mannequin/ADB/Character.adb");
	if (IsLocalClient())
	{
		m_pAnimationComponent->SetCharacterFile("Objects/Characters/exo_swat/exo_swat_1p.cdf");
		m_pAnimationComponent->SetDefaultScopeContextName("FirstPersonCharacter");
	}
	else
	{
		m_pAnimationComponent->SetCharacterFile("Objects/Characters/exo_swat/exo_swat_3p.cdf");
		m_pAnimationComponent->SetDefaultScopeContextName("ThirdPersonCharacter");
	}

	m_pAnimationComponent->SetControllerDefinitionFile("Animations/Mannequin/ADB/CharacterControllerDefinition.xml");

	// Queue the idle fragment to start playing immediately on next update
	m_pAnimationComponent->SetDefaultFragmentName("Locomotion");

	// Disable movement coming from the animation (root joint offset), we control this entirely via physics
	m_pAnimationComponent->SetAnimationDrivenMotion(true);

	// Load the character and Mannequin data from file
	m_pAnimationComponent->LoadFromDisk();

	//TODO: move this into networking code when I get to it.
// 	if (!IsLocalClient())
// 	{
// 		if (IAttachmentManager* pAttachMgr = m_pAnimationComponent->GetCharacter()->GetIAttachmentManager())
// 		{
// 			if (IAttachmentObject* pAttachObj = pAttachMgr->GetInterfaceByIndex(0)->GetIAttachmentObject())
// 			{
// 				IMaterial* bulletMat = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial("Materials/m_exo_swat");
// 				IMaterial* newMat = gEnv->p3DEngine->GetMaterialManager()->CloneMaterial(bulletMat);
// 				Vec3 matColor(.1f);
// 				newMat->SetGetMaterialParamVec3("diffuse", matColor, false);
// 				pAttachObj->SetReplacementMaterial(newMat);
// 			}
// 		}
// 	}

	// Acquire fragment and tag identifiers to avoid doing so each update
	m_locomotionId = m_pAnimationComponent->GetFragmentId("Locomotion");
	m_rifleId = m_pAnimationComponent->GetTagId("Rifle");
	// 	m_rotateTagId = m_pAnimationComponent->GetTagId("Rotate");

		// Mark the entity to be replicated over the network
	m_pEntity->GetNetEntity()->BindToNetwork();

	// Register the RemoteReviveOnClient function as a Remote Method Invocation (RMI) that can be executed by the server on clients
	SRmi<RMI_WRAP(&CPlayerComponent::RemoteReviveOnClient)>::Register(this, eRAT_NoAttach, false, eNRT_ReliableOrdered);
	SRmi<RMI_WRAP(&CPlayerComponent::SvJump)>::Register(this, eRAT_NoAttach, false, eNRT_ReliableOrdered);
	SRmi<RMI_WRAP(&CPlayerComponent::ClJump)>::Register(this, eRAT_NoAttach, false, eNRT_ReliableOrdered);
}

void CPlayerComponent::InitializeLocalPlayer()
{
	// Create the camera component, will automatically update the viewport every frame
	m_pCameraComponent = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CCameraComponent>();
	m_pCameraComponent->SetNearPlane(0.01f);

	// Create the audio listener component.
	m_pAudioListenerComponent = m_pEntity->GetOrCreateComponent<Cry::Audio::DefaultComponents::CListenerComponent>();

	// Get the input component, wraps access to action mapping so we can easily get callbacks when inputs are triggered
	m_pInputComponent = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CInputComponent>();

	m_pInputComponent->RegisterAction("player", "changeview", [this](int activationMode, float value)
	{
		if (m_currentView == EPersonViewState::First)
		{
			m_pAnimationComponent->SetCharacterFile("Objects/Characters/exo_swat/exo_swat_3p.cdf");
			RefreshIKJoints();
			m_pAnimationComponent->ActivateContext("ThirdPersonCharacter");
			m_currentView = EPersonViewState::Third;
			m_pAnimationComponent->SetTagWithId(m_rifleId, m_holdWeapon);
		}
		else
		{
			m_pAnimationComponent->SetCharacterFile("Objects/Characters/exo_swat/exo_swat_1p.cdf");
			RefreshIKJoints();
			m_pAnimationComponent->ActivateContext("FirstPersonCharacter");
			m_currentView = EPersonViewState::First;
			m_pAnimationComponent->SetTagWithId(m_rifleId, m_holdWeapon);
		}
		RefreshIKJoints();
	});
	m_pInputComponent->BindAction("player", "changeview", eAID_KeyboardMouse, eKI_F, true, false, false);

	m_pInputComponent->RegisterAction("player", "sprint", [this](int activationMode, float value)
	{
		switch (activationMode)
		{
			case EActionActivationMode::eAAM_OnPress:
				if (m_pCharacterController->IsWalking())
					m_isRunning = true;
				break;
			case EActionActivationMode::eAAM_OnRelease:
				m_isRunning = false;
				break;
			default:
				break;
		}
	});
	m_pInputComponent->BindAction("player", "sprint", eAID_KeyboardMouse, eKI_LShift);

	m_pInputComponent->RegisterAction("player", "jump", [this](int activationMode, float value)
	{
		SRmi<RMI_WRAP(&CPlayerComponent::SvJump)>::InvokeOnServer(this, RemoteJumpParams{ m_pEntity->GetId() });
	});
	m_pInputComponent->BindAction("player", "jump", eAID_KeyboardMouse, eKI_Space, true, false, false);

	m_pInputComponent->RegisterAction("player", "weapon", [this](int activationMode, float value)
	{
		m_holdWeapon = !m_holdWeapon;
		m_pAnimationComponent->SetTagWithId(m_rifleId, m_holdWeapon);
		m_pAnimationComponent->QueueFragmentWithId(m_activeFragmentId);
	});
	m_pInputComponent->BindAction("player", "weapon", eAID_KeyboardMouse, eKI_Q, true, false, false);

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

	// Register the shoot action
	m_pInputComponent->RegisterAction("player", "shoot", [this](int activationMode, float value)
	{
		// Only fire on press, not release
		if (activationMode == eAAM_OnPress)
		{
			SEntitySpawnParams spawnParams;
			spawnParams.pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("schematyc::entities::projectiles::projectile");
			spawnParams.vPosition = m_pCameraComponent->GetCamera().GetPosition();
			spawnParams.qRotation = m_lookOrientation;
			const float bulletScale = 0.05f;
			spawnParams.vScale = Vec3(bulletScale);

			if (IEntity* pEntity = gEnv->pEntitySystem->SpawnEntity(spawnParams))
			{
// 				m_pAnimationComponent->QueueFragment("Fire");
				m_pCrosshairUI->CallFunction("Fire");
			}
		}
	});

	// Bind the shoot action to left mouse click
	m_pInputComponent->BindAction("player", "shoot", eAID_KeyboardMouse, EKeyId::eKI_Mouse1);

	m_pCrosshairUI = gEnv->pFlashUI->GetUIElement("Crosshair");
	m_pCrosshairUI->SetVisible(true);
}

void CPlayerComponent::AttachToCharacter(CCharacterComponent* character)
{
	character->GetEntity()->AttachChild(m_pEntity);
	m_camJointId = character->GetAnimComp()->GetCharacter()->GetIDefaultSkeleton().GetJointIDByName("mixamorig:head");
}

void CPlayerComponent::RefreshIKJoints()
{
	int effectorJoint = m_pAnimationComponent->GetCharacter()->GetIDefaultSkeleton().GetJointIDByName("armature");
	m_aimJointId = m_pAnimationComponent->GetCharacter()->GetIDefaultSkeleton().GetJointIDByName("mixamorig:spine2");
	m_ikTorsoAim->SetJoints(effectorJoint, m_aimJointId);

	if (ICharacterInstance* pCharacter = m_pAnimationComponent->GetCharacter())
	{
		// Cache the camera joint id so that we don't need to look it up every frame in UpdateView
		m_cameraJointId = m_pAnimationComponent->GetCharacter()->GetIDefaultSkeleton().GetJointIDByName("mixamorig:head");
	}
}

Cry::Entity::EventFlags CPlayerComponent::GetEventMask() const
{
	return
		Cry::Entity::EEvent::BecomeLocalPlayer |
		Cry::Entity::EEvent::Update |
		Cry::Entity::EEvent::Reset;
}

void CPlayerComponent::ProcessEvent(const SEntityEvent& event)
{
	switch (event.event)
	{
		case Cry::Entity::EEvent::BecomeLocalPlayer:
		{
			InitializeLocalPlayer();
		}
		break;
		case Cry::Entity::EEvent::Update:
		{
			// Don't update the player if we haven't spawned yet
			if (!m_isAlive)
				return;

			const float frameTime = event.fParam[0];

			if (!m_pCharacterController->IsOnGround() && !m_hasJumped && !m_isFalling)
			{
				m_isFalling = true;
				m_pAnimationComponent->QueueFragment("Jump");
			}
			if (m_pCharacterController->IsOnGround() && (m_hasJumped || m_isFalling) && m_jumpFrame != gEnv->pRenderer->GetFrameID())
			{
				m_isFalling = false;
				m_hasJumped = false;
				m_activeFragmentId = m_locomotionId;
				m_pAnimationComponent->QueueFragmentWithId(m_activeFragmentId);
			}

			// Start by updating the movement request we want to send to the character controller
			// This results in the physical representation of the character moving
			UpdateMovementRequest(frameTime);

			// Process mouse input to update look orientation.
			UpdateLookDirectionRequest(frameTime);

			if (IsLocalClient())
			{
				// Update the camera component offset
				UpdateCamera(frameTime);
			}
		}
		break;
		case Cry::Entity::EEvent::Reset:
		{
			// Disable player when leaving game mode.
			m_isAlive = event.nParam[0] != 0;
		}
		break;
	}
}

bool CPlayerComponent::NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags)
{
	if (aspect == InputAspect)
	{
		ser.BeginGroup("PlayerInput");

		const CEnumFlags<EInputFlag> prevInputFlags = m_inputFlags;

		ser.Value("m_inputFlags", m_inputFlags.UnderlyingValue(), 'ui8');

		if (ser.IsReading())
		{
			const CEnumFlags<EInputFlag> changedKeys = prevInputFlags ^ m_inputFlags;

			const CEnumFlags<EInputFlag> pressedKeys = changedKeys & prevInputFlags;
			if (!pressedKeys.IsEmpty())
			{
				HandleInputFlagChange(pressedKeys, eAAM_OnPress);
			}

			const CEnumFlags<EInputFlag> releasedKeys = changedKeys & prevInputFlags;
			if (!releasedKeys.IsEmpty())
			{
				HandleInputFlagChange(pressedKeys, eAAM_OnRelease);
			}
		}

		// Serialize the player look orientation
		ser.Value("m_lookOrientation", m_lookOrientation, 'ori3');

		ser.EndGroup();
	}

	return true;
}

void CPlayerComponent::UpdateMovementRequest(float frameTime)
{
	float travelSpeed = 0.0f;
	float travelAngle = 0.0f;

	const float moveSpeed = 3.0f;

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

	const float runMultiplier = m_isRunning && travelSpeed > 0 ? 2.0f : 1.0f;

	Vec3 velocity = Vec3(travelAngle, travelSpeed, 0.0f).normalized();
	velocity.z = 0.0f;

	Ang3 ypr = CCamera::CreateAnglesYPR(Matrix33(m_lookOrientation));
	ypr.z = 0;

	if (!m_pCharacter)
	{
		m_pCharacterController->SetVelocity(m_pEntity->GetWorldRotation() * velocity * moveSpeed * runMultiplier);

		m_pAnimationComponent->SetMotionParameter(eMotionParamID_TravelSpeed, travelSpeed * runMultiplier);
		m_pAnimationComponent->SetMotionParameter(eMotionParamID_TravelAngle, travelAngle * runMultiplier);

		ypr.y = 0;
	}
	else
	{
		m_pCharacter->UpdateMovement(travelSpeed, travelAngle);

		const Quat correctedOrientation = Quat(CCamera::CreateOrientationYPR(ypr));
		m_pCharacter->UpdateRotation(correctedOrientation);
	}
	// 	else
	// 	{
	// 		Matrix34 transform = m_pEntity->GetWorldTM();
	// 		transform.AddTranslation(transform.TransformVector(velocity));
	// 		Ang3 ypr = CCamera::CreateAnglesYPR(Matrix33(transform));
	// 		m_pEntity->SetWorldTM(transform);
	// 	}

	if (!m_pCharacter)
	{
		const Quat correctedOrientation = Quat(CCamera::CreateOrientationYPR(ypr));
		// Send updated transform to the entity, only orientation changes
		m_pEntity->SetPosRotScale(m_pEntity->GetWorldPos(), correctedOrientation, Vec3(1, 1, 1));
	}
}

void CPlayerComponent::UpdateLookDirectionRequest(float frameTime)
{
 	const float rotationSpeed = 0.002f;
	const float rotationLimitsMinPitch = -0.84f;
	const float rotationLimitsMaxPitch = 1.5f;

	// Apply smoothing filter to the mouse input
   	m_mouseDeltaRotation = m_mouseDeltaSmoothingFilter.Push(m_mouseDeltaRotation).Get();

	//   	if (m_mouseDeltaRotation.IsEquivalent(ZERO, MOUSE_DELTA_TRESHOLD))
	//   		return;

		// Start with updating look orientation from the latest input
	Ang3 ypr = CCamera::CreateAnglesYPR(Matrix33(m_lookOrientation));

	// Yaw
	ypr.x += m_mouseDeltaRotation.x * rotationSpeed;

	// Pitch
	// TODO: Perform soft clamp here instead of hard wall, should reduce rot speed in this direction when close to limit.
 	ypr.y = CLAMP(ypr.y + m_mouseDeltaRotation.y * rotationSpeed, rotationLimitsMinPitch, rotationLimitsMaxPitch);

	// Roll (skip)
	ypr.z = 0;

	m_lookOrientation = Quat(CCamera::CreateOrientationYPR(ypr));

	NetMarkAspectsDirty(InputAspect);

	if (ISkeletonAnim* pSkelAnim = m_pAnimationComponent->GetCharacter()->GetISkeletonAnim())
	{
		//TODO: This will work in single player, but networking will fail with camera grab. Also ugly. Look into later with fresh mind
		Quat localRot = !m_pEntity->GetWorldRotation();
		Vec3 camDirection = m_pCameraComponent->GetCamera().GetPosition() + m_pCameraComponent->GetCamera().GetViewdir() * 10.0f;
		Vec3 entPos = m_pEntity->GetPos() + Vec3(0.0f, 0.0f, 1.5f);
		Vec3 lookDirection = camDirection - entPos;
		m_ikTorsoAim->SetTargetDirection(localRot * lookDirection.normalized());
		m_ikTorsoAim->SetBlendWeight(1.0f);
		pSkelAnim->PushPoseModifier(0, m_ikTorsoAim);
	}

	// Reset the mouse delta accumulator every frame
	m_mouseDeltaRotation = ZERO;
}

void CPlayerComponent::UpdateCamera(float frameTime)
{
	// Start with updating look orientation from the latest input
	Ang3 ypr = CCamera::CreateAnglesYPR(Matrix33(m_lookOrientation));

	ypr.x += m_mouseDeltaRotation.x * m_rotationSpeed;

	const float rotationLimitsMinPitch = -0.84f;
	const float rotationLimitsMaxPitch = 1.5f;

	// TODO: Perform soft clamp here instead of hard wall, should reduce rot speed in this direction when close to limit.
	ypr.y = CLAMP(ypr.y + m_mouseDeltaRotation.y * m_rotationSpeed, rotationLimitsMinPitch, rotationLimitsMaxPitch);
	// Skip roll
	ypr.z = 0;

	m_lookOrientation = Quat(CCamera::CreateOrientationYPR(ypr));

	// Reset every frame
	m_mouseDeltaRotation = ZERO;

	// Ignore z-axis rotation, that's set by CPlayerAnimations
	ypr.x = 0;

	// Start with changing view rotation to the requested mouse look orientation
	Matrix34 localTransform = IDENTITY;
	localTransform.SetRotation33(CCamera::CreateOrientationYPR(ypr));

	if (ICharacterInstance* pCharacter = m_pAnimationComponent->GetCharacter())
	{
		// Get the local space orientation of the camera joint
		const QuatT& cameraOrientation = pCharacter->GetISkeletonPose()->GetAbsJointByID(m_cameraJointId);
		// Apply the offset to the camera
		if (m_currentView == EPersonViewState::First)
		{
			if (m_isRunning)
				localTransform.SetTranslation(cameraOrientation.t);
			else
				localTransform.SetTranslation(cameraOrientation.t + m_camPos_1p);
		}
		else
			localTransform.SetTranslation(cameraOrientation.t + m_camPos_3p);

		Vec3 camPos = m_pCameraComponent->GetTransformMatrix().GetTranslation();
		localTransform.SetTranslation(LERP(camPos, localTransform.GetTranslation(), 5.0f * frameTime));
	}

	m_pCameraComponent->SetTransformMatrix(localTransform);
	m_pAudioListenerComponent->SetOffset(localTransform.GetTranslation());
}

void CPlayerComponent::OnReadyForGameplayOnServer()
{
	CRY_ASSERT(gEnv->bServer, "This function should only be called on the server!");

	const Matrix34 newTransform = CSpawnPointComponent::GetFirstSpawnPointTransform();

	Revive(newTransform);

	// Invoke the RemoteReviveOnClient function on all remote clients, to ensure that Revive is called across the network
	SRmi<RMI_WRAP(&CPlayerComponent::RemoteReviveOnClient)>::InvokeOnOtherClients(this, RemoteReviveParams{ newTransform.GetTranslation(), Quat(newTransform) });

	// Go through all other players, and send the RemoteReviveOnClient on their instances to the new player that is ready for gameplay
	const int channelId = m_pEntity->GetNetEntity()->GetChannelId();
	CGamePlugin::GetInstance()->IterateOverPlayers([this, channelId](CPlayerComponent& player)
	{
		// Don't send the event for the player itself (handled in the RemoteReviveOnClient event above sent to all clients)
		if (player.GetEntityId() == GetEntityId())
			return;

		// Only send the Revive event to players that have already respawned on the server
		if (!player.m_isAlive)
			return;

		// Revive this player on the new player's machine, on the location the existing player was currently at
		const QuatT currentOrientation = QuatT(player.GetEntity()->GetWorldTM());
		SRmi<RMI_WRAP(&CPlayerComponent::RemoteReviveOnClient)>::InvokeOnClient(&player, RemoteReviveParams{ currentOrientation.t, currentOrientation.q }, channelId);
	});
}

bool CPlayerComponent::RemoteReviveOnClient(RemoteReviveParams&& params, INetChannel* pNetChannel)
{
	// Call the Revive function on this client
	Revive(Matrix34::Create(Vec3(1.f), params.rotation, params.position));

	return true;
}


bool CPlayerComponent::SvJump(RemoteJumpParams&& params, INetChannel* pNetChannel)
{
	if (IEntity* pEntity = gEnv->pEntitySystem->GetEntity(params.entityId))
	{
		if (CPlayerComponent* pPlayer = pEntity->GetComponent<CPlayerComponent>())
		{
			if (pPlayer->GetCharacterController()->IsOnGround())
			{
				int channelId = pEntity->GetNetEntity()->GetChannelId();
				SRmi<RMI_WRAP(&CPlayerComponent::ClJump)>::InvokeOnClient(pPlayer, RemoteJumpParams{}, channelId);

				CGamePlugin::GetInstance()->IterateOverPlayers([this, pPlayer](CPlayerComponent& player)
				{
					SRmi<RMI_WRAP(&CPlayerComponent::ClJump)>::InvokeOnClient(pPlayer, RemoteJumpParams{}, player.GetEntity()->GetNetEntity()->GetChannelId());
				});
			}
		}
	}
	return true;
}
bool CPlayerComponent::ClJump(RemoteJumpParams&& params, INetChannel* pNetChannel)
{
	m_pCharacterController->ChangeVelocity(Vec3(0.0f, 0.0f, 5.0f), Cry::DefaultComponents::CCharacterControllerComponent::EChangeVelocityMode::Jump);
	m_pAnimationComponent->QueueFragment("Jump");
	m_hasJumped = true;
	m_jumpFrame = gEnv->pRenderer->GetFrameID();
	return true;
}

void CPlayerComponent::Revive(const Matrix34& transform)
{
	m_isAlive = true;

	// Set the entity transformation, except if we are in the editor
	// In the editor case we always prefer to spawn where the viewport is
	if (!gEnv->IsEditor())
	{
		m_pEntity->SetWorldTM(transform);
	}

	// Apply the character to the entity and queue animations
	m_pAnimationComponent->ResetCharacter();
	m_pCharacterController->Physicalize();

	// Reset input now that the player respawned
	m_inputFlags.Clear();
	NetMarkAspectsDirty(InputAspect);

	m_mouseDeltaRotation = ZERO;
	m_lookOrientation = IDENTITY;

	m_mouseDeltaSmoothingFilter.Reset();

	m_activeFragmentId = m_locomotionId;

	RefreshIKJoints();
}

void CPlayerComponent::HandleInputFlagChange(const CEnumFlags<EInputFlag> flags, const CEnumFlags<EActionActivationMode> activationMode, const EInputFlagType type)
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

	if (IsLocalClient())
	{
		NetMarkAspectsDirty(InputAspect);
	}
}