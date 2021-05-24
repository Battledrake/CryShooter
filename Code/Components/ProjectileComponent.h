#pragma once

#include <CryEntitySystem/IEntitySystem.h>
#include <DefaultComponents/Physics/ParticleComponent.h>

class CProjectileComponent final : public IEntityComponent {
public:
	CProjectileComponent() = default;
	virtual ~CProjectileComponent() = default;

	static void ReflectType(Schematyc::CTypeDesc<CProjectileComponent>& desc)
	{
		desc.SetGUID("{4687B241-F3C7-421C-A98B-6B87448AAAB5}"_cry_guid);
		desc.SetEditorCategory("Custom");
		desc.SetLabel("Projectile");
		desc.SetDescription("Creates projectiles");

		desc.AddMember(&CProjectileComponent::m_moveSpeed, 'move', "MoveSpeed", "Move Speed", "Determines speed of projectile", 1000.0f);
	}
protected:
	virtual void Initialize() override;
	virtual Cry::Entity::EventFlags GetEventMask() const override;
	virtual void ProcessEvent(const SEntityEvent& event) override;

private:
	Cry::DefaultComponents::CPhysParticleComponent* m_pParticle;

	float m_moveSpeed;
};