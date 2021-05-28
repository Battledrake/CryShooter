#pragma once

#include "Interfaces/IInterfaceBase.h"

enum class EEquipmentType
{
	PrimaryWeapon,
	SecondaryWeapon,
	Grenade,
	Count
};

static void ReflectType(Schematyc::CTypeDesc<EEquipmentType>& desc)
{
	desc.SetGUID("{D5F09339-E774-405E-85D6-8A93873B2502}"_cry_guid);
	desc.SetLabel("Equipment Type");
	desc.SetDescription("Determines which equipment slot equippable belongs to");
	desc.SetDefaultValue(EEquipmentType::PrimaryWeapon);
	desc.AddConstant(EEquipmentType::PrimaryWeapon, "PrimaryWeapon", "Primary Weapon");
	desc.AddConstant(EEquipmentType::SecondaryWeapon, "SecondaryWeapon", "Secondary Weapon");
	desc.AddConstant(EEquipmentType::Grenade, "Grenade", "Grenade");
}

struct IEquippable : IInterfaceBase
{
	virtual const char* GetEquipmentName() const = 0;
	virtual EEquipmentType GetEquipmentType() const = 0;
};