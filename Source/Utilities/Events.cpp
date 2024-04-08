#include "Precomp.h"
#include "Utilities/Events.h"

const CE::MetaFunc* CE::Internal::TryGetEvent(const MetaType& fromType, std::string_view eventName)
{
	const MetaFunc* func = fromType.TryGetFunc(eventName);

	if (func != nullptr
		&& func->GetProperties().Has(Internal::sIsEventProp))
	{
		return func;
	}
	return nullptr;
}

std::vector<CE::BoundEvent> CE::Internal::GetAllBoundEvents(std::string_view eventName)
{
	std::vector<BoundEvent> bound{};

	for (const MetaType& type : MetaManager::Get().EachType())
	{
		const MetaFunc* const boundFunc = Internal::TryGetEvent(type, eventName);

		if (boundFunc == nullptr)
		{
			continue;
		}

		bound.emplace_back(BoundEvent{ type, *boundFunc, boundFunc->GetProperties().Has(Props::sIsEventStaticTag) });
	}

	return bound;
}
