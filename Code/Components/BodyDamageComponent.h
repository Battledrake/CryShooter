#pragma once

#include <CryEntitySystem/IEntitySystem.h>
#include <CrySchematyc/Utils/SharedString.h>
#include <map>

class CHealthComponent;

class CBodyDamageComponent final : public IEntityComponent
{
	struct SBodyPartsDefinition
	{
		friend class CBodyDamageComponent;

		static void ReflectType(Schematyc::CTypeDesc<SBodyPartsDefinition>& desc)
		{
			desc.SetGUID("{3A9F88B4-760F-4A54-8032-1058DC7D06B8}"_cry_guid);
			desc.SetLabel("BodyPartsDefinition");
			desc.SetDescription("Struct for setting body part and multiplier for damage checks");
			desc.AddMember(&SBodyPartsDefinition::m_jointName, 'part', "PartName", "Body Joint Name", "Define name of body joint to check for", "");
			desc.AddMember(&SBodyPartsDefinition::m_damageMultiplier, 'dmgm', "DamageMultiplier", "Damage Multipler", "Determines amount damage is multipled by", 1.0f);
		}

		inline bool Serialize(Serialization::IArchive& ar);

		inline bool operator==(const SBodyPartsDefinition& rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }
		inline bool operator!=(const SBodyPartsDefinition& rhs) const { return !(*this == rhs); }

	private:
		Schematyc::CSharedString m_jointName;
		float m_damageMultiplier;
	};

public:
	CBodyDamageComponent() = default;
	virtual ~CBodyDamageComponent() = default;

	void CalculateDamage(float damage, int jointId);

	static void ReflectType(Schematyc::CTypeDesc<CBodyDamageComponent>& desc)
	{
		desc.SetGUID("{E30CB5B2-6E59-406C-9D97-E84D0D08EAEF}"_cry_guid);
		desc.SetEditorCategory("Custom");
		desc.SetLabel("Body Damage");
		desc.SetDescription("Allows damage modification based on body proxy");
		desc.AddMember(&CBodyDamageComponent::m_bodyPartsArray, 'bpa', "PartArray", "Body Parts Array", "Define body parts and damage multipliers", Schematyc::CArray<SBodyPartsDefinition>());
	}

protected:
	virtual Cry::Entity::EventFlags GetEventMask() const override;
	virtual void ProcessEvent(const SEntityEvent& event) override;

private:
	void FillJointMap();

	CHealthComponent* m_pHealthComp;

	Schematyc::CArray<SBodyPartsDefinition> m_bodyPartsArray;
	std::map<int, SBodyPartsDefinition> m_jointIdMap;
};