#include "StdAfx.h"
#include "PickupComponent.h"

#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <CrySchematyc/Env/Elements/EnvComponent.h>
#include <CryCore/StaticInstanceList.h>

namespace {
	static void RegisterPickupComponent(Schematyc::IEnvRegistrar& registrar) {
		Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID()); {
			Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CPickupComponent)); {
			}
		}
	}
	CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterPickupComponent)
}
