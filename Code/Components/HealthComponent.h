#pragma once

#include <CryEntitySystem/IEntitySystem.h>

class CHealthComponent final : public IEntityComponent {
public:
	CHealthComponent() = default;
	virtual ~CHealthComponent() = default;

	void ApplyDamage(float amount, int partId, Vec3 hitVelocity = ZERO);
	void Die(int partId, Vec3 hitVelocity);

	// Reflect type to set a unique identifier for this component
	// and provide additional information to expose it in the sandbox
	static void ReflectType(Schematyc::CTypeDesc<CHealthComponent>& desc) {
		desc.SetGUID("{40890E59-3B95-4801-B08C-08667AA12121}"_cry_guid);
		desc.SetEditorCategory("Custom");
		desc.SetLabel("Health");
		desc.SetDescription("Adds Health");

		desc.AddMember(&CHealthComponent::m_startingHealth, 'csh', "StartHealth", "Starting Health", "Health begins at", 100);
		desc.AddMember(&CHealthComponent::m_maxHealth, 'max', "MaxHealth", "Max Health", "Maximum health value", 100);
	}

protected:
	virtual void Initialize() override;
	virtual Cry::Entity::EventFlags GetEventMask() const override;
	virtual void ProcessEvent(const SEntityEvent& event) override;

private:
	int m_startingHealth;
	int m_currentHealth;
	int m_maxHealth = 100;
};