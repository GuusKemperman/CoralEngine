#include "Precomp.h"
#include "Meta/MetaField.h"

#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaAny.h"
#include "Meta/MetaProps.h"

Engine::MetaField::MetaField(const MetaType& outerType,
	const MetaType& type,
	const uint32 offset,
	std::string_view name) :
	mType(type),
	mOuterType(outerType),
	mOffset(offset),
	mName(name),
	mProperties(std::make_unique<MetaProps>())
{
}

Engine::MetaField::MetaField(MetaField&&) noexcept = default;
Engine::MetaField::~MetaField() = default;

bool Engine::MetaField::operator==(const MetaField& other) const
{
	return mName == other.mName && mOuterType.get() == other.mOuterType.get();
}

bool Engine::MetaField::operator!=(const MetaField& other) const
{
	return mName != other.mName || mOuterType.get() != other.mOuterType.get();
}

Engine::MetaAny Engine::MetaField::MakeRef(MetaAny& object) const
{
	ASSERT(mOuterType.get().IsBaseClassOf(object.GetTypeId()));

	void* data = object.GetData();

	if (data != nullptr)
	{
		data = reinterpret_cast<void*>(reinterpret_cast<uintptr>(data) + static_cast<uintptr>(mOffset));
	}

	return MetaAny{ GetType().GetTypeInfo(), data };
}

#ifdef ASSERTS_ENABLED
void Engine::MetaField::AssertThatOuterMatches(TypeId expectedTypeId) const
{
	ASSERT_LOG_FMT(GetOuterType().GetTypeId() == expectedTypeId, "Field {} does not belong to type {}", mName, GetOuterType().GetName());
}
#endif // ASSERTS_ENABLED
