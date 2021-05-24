#include "StdAfx.h"
#include "UIComponent.h"

#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <CrySchematyc/Env/Elements/EnvComponent.h>
#include <CryCore/StaticInstanceList.h>

namespace
{
	static void RegisterUIComponent(Schematyc::IEnvRegistrar& registrar)
	{
		Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
		{
			Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CUIComponent));
			{
			}
		}
	}
	CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterUIComponent)
}

void CUIComponent::Initialize()
{}

Cry::Entity::EventFlags CUIComponent::GetEventMask() const
{
	return
		Cry::Entity::EEvent::GameplayStarted |
		Cry::Entity::EEvent::Reset;
}

void CUIComponent::ProcessEvent(const SEntityEvent& event)
{
	switch (event.event)
	{
		case Cry::Entity::EEvent::GameplayStarted:
		{
			m_pCrosshairUI = gEnv->pFlashUI->GetUIElement("Crosshair");
			m_pCrosshairUI->SetVisible(true);
			m_pWeaponUI = gEnv->pFlashUI->GetUIElement("WeaponHud");
			m_pInteractableUI = gEnv->pFlashUI->GetUIElement("InteractableText");

			m_pPlayer = m_pEntity->GetComponent<CTempPlayerComponent>();
			if (CCharacterComponent* pCharacter = m_pPlayer->GetCharacter())
			{
				RegisterUIEvents(pCharacter);
			}
		}
		break;
		case Cry::Entity::EEvent::Reset:
		{
			m_pCrosshairUI->SetVisible(false);
			m_pWeaponUI->SetVisible(false);
		}
		break;
	}
}

void CUIComponent::RegisterUIEvents(CCharacterComponent* pCharacter)
{
	pCharacter->m_equipEvent.RegisterListener([this](const string weaponName, const string fireMode, const int clipCount, const int clipCapacity, const int totalAmmo)
	{
		SUIArguments uiArgs;
		uiArgs.AddArgument(weaponName);
		uiArgs.AddArgument(fireMode);
		uiArgs.AddArgument(clipCount);
		uiArgs.AddArgument(clipCapacity);
		uiArgs.AddArgument(totalAmmo);
		m_pWeaponUI->CallFunction("SetupWeapon", uiArgs);
		m_pWeaponUI->SetVisible(true);
	});

	pCharacter->m_wepFiredEvent.RegisterListener([this](const int clipCount, const int totalAmmo)
	{
		m_pWeaponUI->CallFunction("Fire", SUIArguments::Create(clipCount));
		m_pWeaponUI->CallFunction("SetTotalAmmo", SUIArguments::Create(totalAmmo));
		m_pCrosshairUI->CallFunction("Fire");
	});

	pCharacter->m_reloadEvent.RegisterListener([this](const int clipCount)
	{
		m_pWeaponUI->CallFunction("Reload", SUIArguments::Create(clipCount));
	});

	pCharacter->m_switchFireModeEvent.RegisterListener([this](const string fireMode)
	{
		m_pWeaponUI->CallFunction("SetFireMode", SUIArguments::Create(fireMode));
	});

	m_pPlayer->m_interactEvent.RegisterListener([this](const SObjectData& objData, const bool isShowing)
	{
		if (isShowing)
		{
			string combinedString = objData.objectKeyword + " " + objData.objectName + " " + objData.objectBonus;
			m_pInteractableUI->CallFunction("UpdateText", SUIArguments::Create(combinedString));
		}
		m_pInteractableUI->SetVisible(isShowing);

	});
}