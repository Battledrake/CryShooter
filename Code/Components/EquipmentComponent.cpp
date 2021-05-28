#include "StdAfx.h"
#include "EquipmentComponent.h"

#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <CrySchematyc/Env/Elements/EnvComponent.h>
#include <CryCore/StaticInstanceList.h>

namespace
{
	static void RegisterEquipmentComponent(Schematyc::IEnvRegistrar& registrar)
	{
		Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
		{
			Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CEquipmentComponent));
		}
	}
	CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterEquipmentComponent)
}

void CEquipmentComponent::Initialize()
{}

void CEquipmentComponent::AddEquipment(IEquippable* pEquipment)
{
	m_equipmentSlots[(int)pEquipment->GetEquipmentType()] = pEquipment;
}

bool CEquipmentComponent::CheckIfHasEquipment(IEquippable* pEquippable) const
{
	for (int i = 0; i < m_equipmentSlots.capacity(); ++i)
	{
		//We check each slot to make sure it's not a nullptr, since array is set with a fixed size
		//to represent equipment slots based on EquipmentType enum. Slots are nullptr if not filled.
		if (m_equipmentSlots[i])
		{
			if (strcmp(m_equipmentSlots[i]->GetEquipmentName(), pEquippable->GetEquipmentName()) == 0)
				return true;
		}
	}
	return false;
}

Cry::Entity::EventFlags CEquipmentComponent::GetEventMask() const
{
	return Cry::Entity::EEvent::Reset;
}

void CEquipmentComponent::ProcessEvent(const SEntityEvent& event)
{
	switch (event.event)
	{
		case Cry::Entity::EEvent::Reset:
		{
			m_equipmentSlots.clear();
		}
		break;
	}
}
