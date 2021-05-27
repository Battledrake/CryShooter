#pragma once

#include <CryEntitySystem/IEntitySystem.h>

class CProjectileComponent final : public IEntityComponent {
public:
	CProjectileComponent() = default;
	virtual ~CProjectileComponent() = default;

	CCharacterComponent* GetProjectileOwner() const { return m_pOwner; }

	void InitializeProjectile(CCharacterComponent* pOwner);

	static void ReflectType(Schematyc::CTypeDesc<CProjectileComponent>& desc)
	{
		desc.SetGUID("{4687B241-F3C7-421C-A98B-6B87448AAAB5}"_cry_guid);
		desc.SetEditorCategory("Custom");
		desc.SetLabel("Projectile");
		desc.SetDescription("Creates projectiles");

		desc.AddMember(&CProjectileComponent::m_moveSpeed, 'move', "MoveSpeed", "Move Speed", "Determines speed of projectile", 1000.0f);
	}
protected:
	virtual Cry::Entity::EventFlags GetEventMask() const override;
	virtual void ProcessEvent(const SEntityEvent& event) override;

private:
	CCharacterComponent* m_pOwner;
	float m_moveSpeed;
};