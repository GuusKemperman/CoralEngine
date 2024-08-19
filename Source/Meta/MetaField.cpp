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

const CE::MetaFunc* CE::MetaField::GetSetter() const
{
	return mSetter;
}

void CE::MetaField::SetSetter(const MetaFunc* func)
{
	ASSERT(func->GetParameters().size() == 2);
	ASSERT(!MetaFunc::CanArgBePassedIntoParam(TypeTraits(GetOuterType().GetTypeId(), TypeForm::Ref), func->GetParameters()[0].mTypeTraits).has_value());
	ASSERT(!MetaFunc::CanArgBePassedIntoParam(TypeTraits(GetType().GetTypeId(), TypeForm::ConstRef), func->GetParameters()[1].mTypeTraits).has_value());
	mSetter = func;
}

const CE::MetaFunc* CE::MetaField::GetGetter() const
{
	return mGetter;
}

void CE::MetaField::SetGetter(const MetaFunc* func)
{
	ASSERT(func->GetParameters().size() == 1);
	ASSERT(!MetaFunc::CanArgBePassedIntoParam(TypeTraits(GetOuterType().GetTypeId(), TypeForm::ConstRef), func->GetParameters()[0].mTypeTraits).has_value());
	ASSERT(mType.get().IsBaseClassOf(func->GetReturnType().mTypeTraits.mStrippedTypeId));
	mGetter = func;
}

CE::MetaAny CE::MetaField::Get(const MetaAny& objectOfOuterType) const
{
	MetaAny any = CanGetConstRef() ? GetConstRef(objectOfOuterType) : std::move(mGetter->InvokeUncheckedUnpacked(objectOfOuterType).GetReturnValue());
	return std::move(GetType().Construct(any).GetReturnValue());
}

void CE::MetaField::Set(MetaAny& objectOfOuterType, const MetaAny& value) const
{
	if (mSetter != nullptr)
	{
		mSetter->InvokeUncheckedUnpacked(objectOfOuterType, value);
		return;
	}

	MetaAny ref{ GetType(), GetFieldAddress(objectOfOuterType), false };
	GetType().Assign(ref, value);
}

bool CE::MetaField::CanGetRef() const
{
	return mGetter == nullptr && mSetter == nullptr;
}

bool CE::MetaField::CanGetConstRef() const
{
	return mGetter == nullptr;
}

CE::MetaAny CE::MetaField::GetRef(MetaAny& objectOfOuterType) const
{
	ASSERT(CanGetRef());
	ASSERT(mOuterType.get().IsBaseClassOf(objectOfOuterType.GetTypeId())
		&& objectOfOuterType.GetData() != nullptr);

	void* data = objectOfOuterType.GetData();
	data = reinterpret_cast<void*>(reinterpret_cast<uintptr>(data) + static_cast<uintptr>(mOffset));
	return MetaAny{ GetType().GetTypeInfo(), data };
}

CE::MetaAny CE::MetaField::GetConstRef(const MetaAny& objectOfOuterType) const
{
	ASSERT(CanGetConstRef());
	ASSERT(mOuterType.get().IsBaseClassOf(objectOfOuterType.GetTypeId())
		&& objectOfOuterType.GetData() != nullptr);

	return MetaAny{ GetType().GetTypeInfo(), GetFieldAddress(const_cast<MetaAny&>(objectOfOuterType)) };
}

void* CE::MetaField::GetFieldAddress(MetaAny& objectOfOuterType) const
{
	void* data = objectOfOuterType.GetData();
	return reinterpret_cast<void*>(reinterpret_cast<uintptr>(data) + static_cast<uintptr>(mOffset));
}

const void* CE::MetaField::GetFieldAddress(const MetaAny& objectOfOuterType) const
{
	void* data = const_cast<void*>(objectOfOuterType.GetData());
	return reinterpret_cast<void*>(reinterpret_cast<uintptr>(data) + static_cast<uintptr>(mOffset));
}

#ifdef ASSERTS_ENABLED
void CE::MetaField::AssertThatOuterMatches(TypeId expectedTypeId) const
{
	ASSERT_LOG(GetOuterType().GetTypeId() == expectedTypeId, "Field {} does not belong to type {}", mName, GetOuterType().GetName());
}
#endif // ASSERTS_ENABLED
