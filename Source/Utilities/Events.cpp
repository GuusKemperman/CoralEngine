#include "Precomp.h"
#include "Utilities/Events.h"

std::optional<CE::BoundEvent> CE::Internal::TryGetEvent(const MetaType& fromType, std::string_view eventName)
{
	const MetaFunc* func = fromType.TryGetFunc(eventName);

	if (func != nullptr
		&& func->GetProperties().Has(Internal::sIsEventProp))
	{
		return BoundEvent{ fromType, *func, func->GetProperties().Has(sIsEventStaticTag) };
	}
	return std::nullopt;
}

std::vector<CE::BoundEvent> CE::Internal::GetAllBoundEvents(std::string_view eventName)
{
	std::vector<BoundEvent> bound{};

	for (const MetaType& type : MetaManager::Get().EachType())
	{
		std::optional<BoundEvent> boundEvent = Internal::TryGetEvent(type, eventName);

		if (!boundEvent.has_value())
		{
			continue;
		}

		bound.emplace_back(std::move(*boundEvent));
	}

	return bound;
}
