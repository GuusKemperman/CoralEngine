#pragma once

namespace CE
{
	class SourceInfo
	{
	public:
		virtual ~SourceInfo() = default;

		virtual std::string ToString() const = 0;

#ifdef EDITOR
		virtual void NavigateToLocation() const = 0;
#endif
	};

	using IRSource = std::shared_ptr<SourceInfo>;
}