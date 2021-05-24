#pragma once

#include <Interfaces/IInterfaceBase.h>

struct ITakeDamage : public IInterfaceBase
{
	virtual void PassDamage(float amount, int partId) = 0;
};