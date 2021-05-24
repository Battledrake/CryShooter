#pragma once

#include <CryEntitySystem/IEntitySystem.h>
#include <CryRenderer/IRenderAuxGeom.h>

//  struct IInterfaceTester : public ICryUnknown
//  {
//  	virtual void PrintTestMessage() = 0;
//  
//  	CRYINTERFACE_DECLARE_GUID(IInterfaceTester, "{1BDB8F1F-79E9-4965-A0F2-71EE47BF2B6E}"_cry_guid);
//  };

struct SBodyDamageMultiplier
{
	static void ReflectType(Schematyc::CTypeDesc<SBodyDamageMultiplier>& desc)
	{
		desc.SetGUID("{882B0F2C-6A0D-4CBF-9A20-CC50ACD37E21}"_cry_guid);
		desc.AddMember(&SBodyDamageMultiplier::m_bodyPart, 'part', "BodyPart", "Body Part Name", "Which joint is being checked", "");
		desc.AddMember(&SBodyDamageMultiplier::m_damageMultiplier, 'dam', "Damage", "Damage Multiplier", "Determines damage amount on body part", 1.0f);
	}

	inline bool Serialize(Serialization::IArchive& archive);

	inline bool operator==(const SBodyDamageMultiplier& rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }
	inline bool operator!=(const SBodyDamageMultiplier& rhs) const { return !(*this == rhs); }

	Schematyc::CSharedString m_bodyPart;
	float m_damageMultiplier;

};

class CDebugComponent final :
	public IEntityComponent,
	public IEntityComponentPreviewer
{
public:
	CDebugComponent() = default;
	virtual ~CDebugComponent() = default;

	virtual void Initialize() override;
	virtual Cry::Entity::EventFlags GetEventMask() const override;
	virtual void ProcessEvent(const SEntityEvent& event) override;

	virtual IEntityComponentPreviewer* GetPreviewer() final { return this; }
	virtual void SerializeProperties(Serialization::IArchive& archive) final {
	}
	virtual void Render(const IEntity& entity, const IEntityComponent& comp, SEntityPreviewContext& context) const final
	{
		ColorB gizmoColor = context.debugDrawInfo.color;
		Matrix34 transform = GetWorldTransformMatrix();
		Vec3 dir = transform.GetColumn1() * 1.4f;
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(transform.GetTranslation(), m_sphereRadius, gizmoColor);
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(transform.GetTranslation(), gizmoColor, transform.GetTranslation() + dir, gizmoColor, m_lineThickness);
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawCone(transform.GetTranslation() + dir, dir, m_coneRadius, m_coneHeight, gizmoColor);
	}

	// Reflect type to set a unique identifier for this component
	// and provide additional information to expose it in the sandbox
	static void ReflectType(Schematyc::CTypeDesc<CDebugComponent>& desc)
	{
		desc.SetGUID("{348D3558-8EDA-42E1-ACC5-6A6A0AC3B2F6}"_cry_guid);
		desc.SetEditorCategory("Debug");
		desc.SetLabel("DebugComponent");
		desc.SetDescription("Component used to quickly test and debug component related things");
		desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach });

		desc.AddMember(&CDebugComponent::m_damageParts, 'bdmp', "DamageParts", "Body Damage Parts", "Set up each body part and the modified damage when hit", Schematyc::CArray<SBodyDamageMultiplier>());
	}

private:
	Schematyc::CArray<SBodyDamageMultiplier> m_damageParts;
	std::vector<int> m_activeJointIds;
	
	float m_sphereRadius = 0.5f;
	float m_lineThickness = 1.0f;
	float m_coneRadius = 0.1f;
	float m_coneHeight = 0.4f;
};