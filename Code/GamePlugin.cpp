#include "StdAfx.h"
#include "GamePlugin.h"

#include "Components/PlayerComponent.h"
#include "Components/SpawnPoint.h"

#include <CrySchematyc/Env/IEnvRegistry.h>
#include <CrySchematyc/Env/EnvPackage.h>
#include <CrySchematyc/Utils/SharedString.h>

#include <IGameObjectSystem.h>
#include <IGameObject.h>

// Included only once per DLL module.
#include <CryCore/Platform/platform_impl.inl>

CGamePlugin::~CGamePlugin()
{
	gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);

	if (gEnv->pSchematyc)
	{
		gEnv->pSchematyc->GetEnvRegistry().DeregisterPackage(CGamePlugin::GetCID());
	}
}

bool CGamePlugin::Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
{
	// Register for engine system events, in our case we need ESYSTEM_EVENT_GAME_POST_INIT to load the map
	gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this, "CGamePlugin");

	return true;
}

void CGamePlugin::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
	switch (event)
	{
		// Called when the game framework has initialized and we are ready for game logic to start
		case ESYSTEM_EVENT_GAME_POST_INIT:
		{

			// Don't need to load the map in editor
			if (!gEnv->IsEditor())
			{
				// Load the example map in client server mode
				gEnv->pConsole->ExecuteString("map example s", false, true);
			}
		}
		break;

		case ESYSTEM_EVENT_LEVEL_GAMEPLAY_START:
		{
			CryLog("Called");
			SEntitySpawnParams charParams;
			charParams.pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("schematyc::entities::characters::playercharacter");
			charParams.sName = "PlayerCharacter";

			if (IEntity* pCharacterEntity = gEnv->pEntitySystem->SpawnEntity(charParams))
			{
				CCharacterComponent* pCharacterComponent = pCharacterEntity->GetComponent<CCharacterComponent>();

				SEntitySpawnParams playerParams;
				playerParams.pClass = gEnv->pEntitySystem->GetClassRegistry()->GetDefaultClass();
				playerParams.id = LOCAL_PLAYER_ENTITY_ID;
				playerParams.nFlags |= ENTITY_FLAG_LOCAL_PLAYER;
				playerParams.sName = "Drake";

				if (IEntity* pPlayerEntity = gEnv->pEntitySystem->SpawnEntity(playerParams))
				{
					pCharacterComponent->SetOwner(pPlayerEntity);

					CPlayerComponent* pPlayerComponent = pPlayerEntity->GetOrCreateComponent<CPlayerComponent>();
					pPlayerComponent->SetCharacter(pCharacterComponent);
					pCharacterEntity->AttachChild(pPlayerEntity);
					pPlayerComponent->SetPosOnAttach();

					const Matrix34 spawnTransform = CSpawnPointComponent::GetFirstSpawnPointTransform();
					pCharacterEntity->SetWorldTM(spawnTransform);
				}
			}
		}
		break;

		case ESYSTEM_EVENT_REGISTER_SCHEMATYC_ENV:
		{
			// Register all components that belong to this plug-in
			auto staticAutoRegisterLambda = [](Schematyc::IEnvRegistrar& registrar)
			{
				// Call all static callback registered with the CRY_STATIC_AUTO_REGISTER_WITH_PARAM
				Detail::CStaticAutoRegistrar<Schematyc::IEnvRegistrar&>::InvokeStaticCallbacks(registrar);
			};

			if (gEnv->pSchematyc)
			{
				gEnv->pSchematyc->GetEnvRegistry().RegisterPackage(
					stl::make_unique<Schematyc::CEnvPackage>(
					CGamePlugin::GetCID(),
					"EntityComponents",
					"Crytek GmbH",
					"Components",
					staticAutoRegisterLambda
				)
				);
			}
		}
		break;

		case ESYSTEM_EVENT_LEVEL_UNLOAD:
		{
		}
		break;
	}
}

CRYREGISTER_SINGLETON_CLASS(CGamePlugin)