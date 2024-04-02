#pragma once
#include "Scripting/Nodes/FunctionLikeScriptNode.h"

namespace CE
{
	class Script;
	class ScriptFunc;

	namespace Internal
	{
		// The common parts between the Entry and Return node.
		class NodeInvolvingScriptFunc :
			public FunctionLikeNode
		{
		protected:
			// ScriptNode constructs an instance of this 
			// object during serialization
			friend class ScriptNode;

			// This constructor is used only when deserializing
			NodeInvolvingScriptFunc(const ScriptNodeType type) :
				FunctionLikeNode(type) {};

			NodeInvolvingScriptFunc(ScriptNodeType type,
				ScriptFunc& scriptFunc,
				const Script& scriptThisFuncIsFrom);

		public:
			void SerializeTo(BinaryGSONObject& to, const ScriptFunc& scriptFunc) const override;

#ifdef REMOVE_FROM_SCRIPTS_ENABLED
			void PostDeclarationRefresh(ScriptFunc& scriptFunc) override;
#endif // REMOVE_FROM_SCRIPTS_ENABLED

			std::string GetSubTitle() const override { return mNameOfFunction; }

			glm::vec4 GetHeaderColor(const ScriptFunc&) const override { return glm::vec4{ 0.318, 0.475, 0.576, 1.0f }; }

		protected:
			const ScriptFunc* TryGetOriginalFunc() const;

		private:
			bool DeserializeVirtual(const BinaryGSONObject& from) override;

		protected:
			std::string mNameOfScriptAsset{};
			std::string mNameOfFunction{};
		};
	}

	class FunctionEntryScriptNode final :
		public Internal::NodeInvolvingScriptFunc
	{
		// ScriptNode constructs an instance of this 
		// object during serialization
		friend class ScriptNode;

		// This constructor is used only when deserializing
		FunctionEntryScriptNode() :
			NodeInvolvingScriptFunc(ScriptNodeType::FunctionEntry) {};

	public:
		FunctionEntryScriptNode(ScriptFunc& scriptFunc,
			const Script& scriptThisFuncIsFrom) :
			NodeInvolvingScriptFunc(ScriptNodeType::FunctionEntry, scriptFunc, scriptThisFuncIsFrom)
		{
			ConstructExpectedPins(scriptFunc);
		}

		static constexpr std::string_view sEntryNodeName = "Entry";

		std::string GetTitle() const override { return std::string{ sEntryNodeName }; };

	private:
		std::optional<InputsOutputs> GetExpectedInputsOutputs(const ScriptFunc& scriptFunc) const override;
	};

	class FunctionReturnScriptNode final :
		public Internal::NodeInvolvingScriptFunc
	{
		// ScriptNode constructs an instance of this 
		// object during serialization
		friend class ScriptNode;

		// This constructor is used only when deserializing
		FunctionReturnScriptNode() :
			NodeInvolvingScriptFunc(ScriptNodeType::FunctionReturn) {};

	public:
		FunctionReturnScriptNode(ScriptFunc& scriptFunc,
			const Script& scriptThisFuncIsFrom) :
			NodeInvolvingScriptFunc(ScriptNodeType::FunctionReturn, scriptFunc, scriptThisFuncIsFrom)
		{
			ConstructExpectedPins(scriptFunc);
		}

		static constexpr std::string_view sReturnNodeName = "Return";

		std::string GetTitle() const override { return std::string{ sReturnNodeName }; };

	private:
		std::optional<InputsOutputs> GetExpectedInputsOutputs(const ScriptFunc& scriptFunc) const override;
	};
}

