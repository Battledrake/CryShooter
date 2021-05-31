#pragma once

#include <CryEntitySystem/IEntitySystem.h>
#include <FlashUI/FlashUI.h>

struct IUIElement;
class CPlayerComponent;

class CUIComponent final : public IEntityComponent
{
public:
	CUIComponent() = default;
	virtual ~CUIComponent() = default;

	// Reflect type to set a unique identifier for this component
	// and provide additional information to expose it in the sandbox
	static void ReflectType(Schematyc::CTypeDesc<CUIComponent>& desc)
	{
		desc.SetGUID("{566E9E80-9868-4619-966D-BF033AE3F9AD}"_cry_guid);
		desc.SetComponentFlags({ IEntityComponent::EFlags::Singleton, IEntityComponent::EFlags::HiddenFromUser });
	}

protected:
	virtual void Initialize() override;
	virtual Cry::Entity::EventFlags GetEventMask() const override;
	virtual void ProcessEvent(const SEntityEvent& event) override;

private:
	void RegisterUIEvents(CCharacterComponent* pCharacter);

	CPlayerComponent* m_pPlayer;

	IUIElement* m_pCrosshairUI;
	IUIElement* m_pWeaponUI;
	IUIElement* m_pInteractableUI;
};