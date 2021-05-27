 #pragma once
 
 #include "Interfaces/IInterfaceBase.h"
 
 struct SObjectData
 {
 	string objectKeyword;
 	string objectName;
 	string objectBonus;
 };
 
 class CCharacterComponent;
 
 struct IInteractable : IInterfaceBase
 {
 	virtual void Observe(CCharacterComponent* pObserver, SObjectData& objectData) = 0;
 	virtual void Interact(CCharacterComponent* pInteractor) = 0;
 };