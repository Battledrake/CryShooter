 #pragma once
 
 #include <numeric>
 
 #include <CryEntitySystem/IEntityComponent.h>
 #include <CryMath/Cry_Camera.h>
 
 #include <ICryMannequin.h>
 #include <CrySchematyc/Utils/EnumFlags.h>
 
 #include <DefaultComponents/Cameras/CameraComponent.h>
 #include <DefaultComponents/Input/InputComponent.h>
 #include <DefaultComponents/Audio/ListenerComponent.h>
 #include "Components/CharacterComponent.h"
 #include "Components/UIComponent.h"
 
 #include "Events/SimpleEvent.h"
 #include "Interfaces/IInteractable.h"
 
 struct IUIElement;
 
 class CPlayerComponent final : public IEntityComponent
 {
 	enum class EViewMode : uint8
 	{
 		Spectator,
 		FirstPerson,
 		ThirdPerson
 	};
 
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
 
 	//Events
 	SimpleEvent<> m_fireEvent;
 	SimpleEvent<const SObjectData&, bool> m_interactEvent;
 	//~Events
 
 	inline CCharacterComponent* GetCharacter() { return m_pCharacter; }
 	inline Vec3 GetFirstPersonPosition() const { return m_thirdPersonPos; }
 	inline Vec3 GetThirdPersonPosition() const { return m_thirdPersonPos; }
 
 	void AddRecoilEffect(Vec2 recoil) { m_mouseDeltaRotation += recoil; }
 	void FreeMovement(float travelSpeed, float travelAngle, float frameTime);
 	void SetPosOnAttach();
 	void SetCharacter(CCharacterComponent* character);
 	void CheckInteractables();
 
 	// Reflect type to set a unique identifier for this component
 	static void ReflectType(Schematyc::CTypeDesc<CPlayerComponent>& desc)
 	{
 		desc.SetGUID("{75A5C934-EA94-4149-A828-3BC3BE1620B6}"_cry_guid);
 	}
 
 protected:
 	virtual void Initialize() override;
 	virtual Cry::Entity::EventFlags GetEventMask() const override;
 	virtual void ProcessEvent(const SEntityEvent& event) override;
 
 private:
 	// Private Methods
 	Vec3 GetDirectionForIK();
 	void RegisterInputs();
 	void UpdateCharacter(float travelSpeed, float travelAngle, float rotationSpeed, float frameTime);
 	void HandleInputFlagChange(CEnumFlags<EInputFlag> flags, CEnumFlags<EActionActivationMode> activationMode, EInputFlagType type = EInputFlagType::Hold);
 
 	// Private Components
 	Cry::DefaultComponents::CCameraComponent* m_pCameraComponent = nullptr;
 	Cry::DefaultComponents::CInputComponent* m_pInputComponent = nullptr;
 	Cry::Audio::DefaultComponents::CListenerComponent* m_pAudioListenerComponent = nullptr;
 	CCharacterComponent* m_pCharacter = nullptr;
 	CUIComponent* m_pUIComponent = nullptr;
 
 	//Private variables
    IEntity* m_pInteractEntity;
 
 	CEnumFlags<EInputFlag> m_inputFlags;
 	Vec2 m_mouseDeltaRotation;
  	MovingAverage<Vec2, 5> m_mouseDeltaSmoothingFilter; //Increase for editor, decrease for game. 5 is an okay balance for swapping between the two.
 	Quat m_lookOrientation;
 
 	EViewMode m_currentViewMode;
 	EViewMode m_viewBeforeSpectate;
 	Vec3 m_activePos;
 	Vec3 m_firstPersonPos = Vec3(-0.1f, 0.0f, 0.1f);
 	Vec3 m_thirdPersonPos = Vec3(0.5f, -1.5f, 0.26f);
 	int m_attachJointId;
 };
