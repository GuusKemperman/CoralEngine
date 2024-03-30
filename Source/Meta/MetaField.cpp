#include "Precomp.h"
#include "Meta/MetaField.h"

#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaAny.h"
#include "Meta/MetaProps.h"

CE::MetaField::MetaField(const MetaType& outerType,
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

CE::MetaField::MetaField(MetaField&&) noexcept = default;
CE::MetaField::~MetaField() = default;

bool CE::MetaField::operator==(const MetaField& other) const
{
	return mName == other.mName && mOuterType.get() == other.mOuterType.get();
}

bool CE::MetaField::operator!=(const MetaField& other) const
{
	return mName != other.mName || mOuterType.get() != other.mOuterType.get();
}

CE::MetaAny CE::MetaField::MakeRef(MetaAny& object) const
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
void CE::MetaField::AssertThatOuterMatches(TypeId expectedTypeId) const
{
	ASSERT_LOG(GetOuterType().GetTypeId() == expectedTypeId, "Field {} does not belong to type {}", mName, GetOuterType().GetName());
}
#endif // ASSERTS_ENABLED
