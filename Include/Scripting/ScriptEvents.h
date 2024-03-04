#pragma once
#include <tinygltf/tiny_gltf.h>

#include "Utilities/Events.h"

namespace Engine
{
	class Script;
	class ScriptFunc;
	class MetaFunc;
	class MetaType;

	class ScriptEvent
	{
	public:
		ScriptEvent(const EventBase& event, std::vector<MetaFuncNamedParam>&& params);

		std::reference_wrapper<const EventBase> mBasedOnEvent;
		std::vector<MetaFuncNamedParam> mParamsToShowToUser{};

		virtual MetaFunc& Declare(TypeTraits selfTraits, MetaType& toType) const = 0;
		virtual void Define(MetaFunc& declaredFunc, const ScriptFunc& scriptFunc, std::shared_ptr<const Script> script) const = 0;
	};

	class ScriptTickEvent final :
		public ScriptEvent
	{
	public:
		ScriptTickEvent();

		MetaFunc& Declare(TypeTraits selfTraits, MetaType& toType) const override;
		void Define(MetaFunc& declaredFunc, const ScriptFunc& scriptFunc, std::shared_ptr<const Script> script) const override;
	};

	class ScriptFixedTickEvent final :
		public ScriptEvent
	{
	public:
		ScriptFixedTickEvent();

		MetaFunc& Declare(TypeTraits selfTraits, MetaType& toType) const override;
		void Define(MetaFunc& declaredFunc, const ScriptFunc& scriptFunc, std::shared_ptr<const Script> script) const override;
	};

	static const ScriptTickEvent sOnTickScriptEvent{};
	static const ScriptFixedTickEvent sOnFixedTickScriptEvent{};

	static const std::array<std::reference_wrapper<const ScriptEvent>, 2> sAllScriptableEvents{ sOnTickScriptEvent, sOnFixedTickScriptEvent };
}
