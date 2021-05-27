 #pragma once
 
 #include <array>
 #include <numeric>
 
 #include <CryEntitySystem/IEntityComponent.h>
 #include <CryMath/Cry_Camera.h>
 
 #include <ICryMannequin.h>
 #include <CrySchematyc/Utils/EnumFlags.h>
 
 #include <DefaultComponents/Cameras/CameraComponent.h>
 #include <DefaultComponents/Physics/CharacterControllerComponent.h>
 #include <DefaultComponents/Geometry/AdvancedAnimationComponent.h>
 #include <DefaultComponents/Input/InputComponent.h>
 #include <DefaultComponents/Audio/ListenerComponent.h>
 
 class CCharacterComponent;
 class CIKTorsoAim;
 struct IUIElement;
 
 enum class EPersonViewState : uint8
 {
 	First,
 	Third
 };
 
 
 ////////////////////////////////////////////////////////
 // Represents a player participating in gameplay
 ////////////////////////////////////////////////////////
 class CPlayerComponent final : public IEntityComponent
 {
 	enum class EInputFlagType
 	{
 		Hold = 0,
 		Toggle
 	};
 
 	enum class EInputFlag : uint8
 	{
 		MoveLeft = 1 << 0,
 		MoveRight = 1 << 1,
 		MoveForward = 1 << 2,
 		MoveBack = 1 << 3
 	};
 
 	static constexpr EEntityAspects InputAspect = eEA_GameClientD;
 
 	template<typename T, size_t SAMPLES_COUNT>
 	class MovingAverage
 	{
 		static_assert(SAMPLES_COUNT > 0, "SAMPLES_COUNT shall be larger than zero!");
 
 	public:
 
 		MovingAverage()
 			: m_values()
 			, m_cursor(SAMPLES_COUNT)
 			, m_accumulator()
 		{}
 
 		MovingAverage& Push(const T& value)
 		{
 			if (m_cursor == SAMPLES_COUNT)
 			{
 				m_values.fill(value);
 				m_cursor = 0;
 				m_accumulator = std::accumulate(m_values.begin(), m_values.end(), T(0));
 			}
 			else
 			{
 				m_accumulator -= m_values[m_cursor];
 				m_values[m_cursor] = value;
 				m_accumulator += m_values[m_cursor];
 				m_cursor = (m_cursor + 1) % SAMPLES_COUNT;
 			}
 
 			return *this;
 		}
 
 		T Get() const
 		{
 			return m_accumulator / T(SAMPLES_COUNT);
 		}
 
 		void Reset()
 		{
 			m_cursor = SAMPLES_COUNT;
 		}
 
 	private:
 
 		std::array<T, SAMPLES_COUNT> m_values;
 		size_t m_cursor;
 
 		T m_accumulator;
 	};
 
 public:
 	CPlayerComponent() = default;
 	virtual ~CPlayerComponent() = default;
 
 	// IEntityComponent
 	virtual void Initialize() override;
 
 	virtual Cry::Entity::EventFlags GetEventMask() const override;
 	virtual void ProcessEvent(const SEntityEvent& event) override;
 
 	virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags) override;
 	virtual NetworkAspectType GetNetSerializeAspectMask() const override { return InputAspect; }
 	// ~IEntityComponent
 
 	Cry::DefaultComponents::CCharacterControllerComponent* GetCharacterController() { return m_pCharacterController; }
 
 	void AddRecoilEffect(Vec2 recoil) { m_mouseDeltaRotation += recoil; }
 
 	//Networking
 	void OnReadyForGameplayOnServer();
 	bool IsLocalClient() const { return (m_pEntity->GetFlags() & ENTITY_FLAG_LOCAL_PLAYER) != 0; }
 	//~Networking
 
 	// Reflect type to set a unique identifier for this component
 	static void ReflectType(Schematyc::CTypeDesc<CPlayerComponent>& desc)
 	{
 		desc.SetGUID("{63F4C0C6-32AF-4ACB-8FB0-57D45DD14725}"_cry_guid);
 	}
 
 protected:
 	void Revive(const Matrix34& transform);
 
 	void UpdateMovementRequest(float frameTime);
 	void UpdateLookDirectionRequest(float frameTime);
 	void UpdateCamera(float frameTime);
 
 	void HandleInputFlagChange(CEnumFlags<EInputFlag> flags, CEnumFlags<EActionActivationMode> activationMode, EInputFlagType type = EInputFlagType::Hold);
 
 	// Called when this entity becomes the local player, to create client specific setup such as the Camera
 	void InitializeLocalPlayer();
 
 protected:
 	// Parameters to be passed to the RemoteReviveOnClient function
 	struct RemoteReviveParams
 	{
 		// Called once on the server to serialize data to the other clients
 		// Then called once on the other side to deserialize
 		void SerializeWith(TSerialize ser)
 		{
 			// Serialize the position with the 'wrld' compression policy
 			ser.Value("pos", position, 'wrld');
 			// Serialize the rotation with the 'ori0' compression policy
 			ser.Value("rot", rotation, 'ori0');
 		}
 
 		Vec3 position;
 		Quat rotation;
 	};
 
 	struct RemoteJumpParams
 	{
 		void SerializeWith(TSerialize ser)
 		{
 			ser.Value("Entity", entityId, 'eid');
 		}
 		EntityId entityId;
 	};
 	// Remote method intended to be called on all remote clients when a player spawns on the server
 	bool RemoteReviveOnClient(RemoteReviveParams&& params, INetChannel* pNetChannel);
 	bool SvJump(RemoteJumpParams&& params, INetChannel* pNetChannel);
 	bool ClJump(RemoteJumpParams&& params, INetChannel* pNetChannel);
 
 private:
 	Cry::DefaultComponents::CCameraComponent* m_pCameraComponent = nullptr;
 	Cry::DefaultComponents::CCharacterControllerComponent* m_pCharacterController = nullptr;
 	Cry::DefaultComponents::CAdvancedAnimationComponent* m_pAnimationComponent = nullptr;
 	Cry::DefaultComponents::CInputComponent* m_pInputComponent = nullptr;
 	Cry::Audio::DefaultComponents::CListenerComponent* m_pAudioListenerComponent = nullptr;
 
 	bool m_isAlive = false;
 
 	TagID m_rifleId;
 	bool m_holdWeapon;
 	TagID m_rotateTagId;
 	FragmentID m_locomotionId;
 	FragmentID m_activeFragmentId;
 
 
 	CEnumFlags<EInputFlag> m_inputFlags;
 	Vec2 m_mouseDeltaRotation;
 	MovingAverage<Vec2, 5> m_mouseDeltaSmoothingFilter;
 	const float m_rotationSpeed = 0.002f;
 	Quat m_lookOrientation;
 
 
 
 	int m_cameraJointId = -1;
 	int m_cameraJointId_3p = -1;
 	void RefreshIKJoints();
 
 	bool m_hasJumped;
 	bool m_isFalling;
 	int m_jumpFrame;
 	bool m_isRunning;
 
 
 	EPersonViewState m_currentView;
 	std::shared_ptr<CIKTorsoAim> m_ikTorsoAim;
 
 	IUIElement* m_pCrosshairUI;
 
 	/*
 	Going to use this space below for all code refactoring as I migrate to a character based system instead of player.
 	*/
 private:
 	void AttachToCharacter(CCharacterComponent* character);
 
 	CCharacterComponent* m_pCharacter;
 
 	int m_camJointId;
 	int m_aimJointId;
 
 	Vec3 m_camPos_3p = Vec3(0.5f, -1.5f, 0.26f);
 	Vec3 m_camPos_1p = Vec3(-0.1f, 0.0f, 0.1f);
 };
