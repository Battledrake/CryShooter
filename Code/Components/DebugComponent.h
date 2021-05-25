#pragma once

#include <CryEntitySystem/IEntitySystem.h>
#include <CryRenderer/IRenderAuxGeom.h>

class CDebugComponent final :
	public IEntityComponent,
	public IEntityComponentPreviewer
{
public:
	CDebugComponent() = default;
	virtual ~CDebugComponent() = default;

	virtual void Initialize() override;
	virtual ComponentEventPriority GetEventPriority() const override;
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

		desc.AddMember(&CDebugComponent::m_collisionType, 'col', "CollisionType", "Collision Type", "Determines collision type", ECollisionType::Enemy);

	}

private:
	ECollisionType m_collisionType;
	
	float m_sphereRadius = 0.5f;
	float m_lineThickness = 1.0f;
	float m_coneRadius = 0.1f;
	float m_coneHeight = 0.4f;
};