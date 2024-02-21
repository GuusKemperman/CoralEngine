#pragma once
#include "Scripting/ScriptNode.h"

namespace Engine
{
	class RerouteScriptNode final :
		public ScriptNode
	{
		// ScriptNode constructs an instance of this 
		// object during serialization
		friend class ScriptNode;

		// This constructor is used only when deserializing
		RerouteScriptNode() :
			ScriptNode(ScriptNodeType::Rerout) {};

	public:
		RerouteScriptNode(ScriptFunc& scriptFunc, const ScriptVariableTypeData& type) :
			ScriptNode(ScriptNodeType::Rerout, scriptFunc)
		{
			SetPins(scriptFunc, InputsOutputs{ { { type } }, { { type } } });
		}
	};
}
