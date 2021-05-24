#pragma once

#include <CryEntitySystem/IEntitySystem.h>

class CPickupComponent final : public IEntityComponent {
public:
	CPickupComponent() = default;
	virtual ~CPickupComponent() = default;

	// Reflect type to set a unique identifier for this component
	// and provide additional information to expose it in the sandbox
	static void ReflectType(Schematyc::CTypeDesc<CPickupComponent>& desc) {
		desc.SetGUID("{5BDB3D82-2757-4210-8D7A-289172469E76}"_cry_guid);
		desc.SetEditorCategory("Custom");
		desc.SetLabel("Pickup");
		desc.SetDescription("Creates pickups");
	}
};