#include "StdAfx.h"
#include "HealthComponent.h"

#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <CrySchematyc/Env/Elements/EnvComponent.h>
#include <CryCore/StaticInstanceList.h>

namespace {
	static void RegisterHealthComponent(Schematyc::IEnvRegistrar& registrar) {
		Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID()); {
			Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CHealthComponent)); {
			}
		}
	}
	CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterHealthComponent)
}
