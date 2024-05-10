#include "Precomp.h"
#include "World/Physics.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"

CE::Physics::Physics(World& world) :
	mWorld(world)
{
}

CE::MetaType CE::Physics::Reflect()
{
	MetaType type = MetaType{ MetaType::T<Physics>{}, "Physics" };
	type.GetProperties().Add(Props::sIsScriptableTag);

	return type;
}
