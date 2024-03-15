#pragma once
#include "Scripting/ScriptNode.h"

namespace Engine
{
	class FunctionLikeNode :
		public ScriptNode
	{
	public:
		using ScriptNode::ScriptNode;

		// Construct pins based on the return value of the GetExpectedInputsOutputs function		
		void ConstructExpectedPins(ScriptFunc& scriptFunc);

#ifdef REMOVE_FROM_SCRIPTS_ENABLED
		void PostDeclarationRefresh(ScriptFunc& scriptFunc) override;
#endif // REMOVE_FROM_SCRIPTS_ENABLED

		void CollectErrors(ScriptErrorInserter inserter, const ScriptFunc& scriptFunc) const override;

	protected:
		// Return std::nullopt if the function no longer exists.
		virtual std::optional<InputsOutputs> GetExpectedInputsOutputs([[maybe_unused]] const ScriptFunc& scriptFunc) const = 0;
	};
}
