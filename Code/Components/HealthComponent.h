#pragma once

#include <CryEntitySystem/IEntitySystem.h>

class CHealthComponent final : public IEntityComponent {
public:
	CHealthComponent() = default;
	virtual ~CHealthComponent() = default;

	// Reflect type to set a unique identifier for this component
	// and provide additional information to expose it in the sandbox
	static void ReflectType(Schematyc::CTypeDesc<CHealthComponent>& desc) {
		desc.SetGUID("{40890E59-3B95-4801-B08C-08667AA12121}"_cry_guid);
		desc.SetEditorCategory("Custom");
		desc.SetLabel("Health");
		desc.SetDescription("Adds Health");
	}
};