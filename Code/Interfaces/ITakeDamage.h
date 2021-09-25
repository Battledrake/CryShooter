#pragma once

#include <Interfaces/IInterfaceBase.h>

struct ITakeDamage : public IInterfaceBase
{
	virtual void PassDamage(float amount, int partId, Vec3 hitVelocity) = 0;
};