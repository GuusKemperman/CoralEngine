#pragma once
#include "Scripting/Nodes/FunctionLikeScriptNode.h"

namespace Engine
{
	class ScriptFunc;

	class BranchScriptNode final :
		public FunctionLikeNode
	{
		// ScriptNode constructs an instance of this 
		// object during serialization
		friend class ScriptNode;

		// This constructor is used only when deserializing
		BranchScriptNode() : 
			FunctionLikeNode(ScriptNodeType::Branch) {};

	public:
		BranchScriptNode(ScriptFunc& scriptFunc) :
			FunctionLikeNode(ScriptNodeType::Branch, scriptFunc)
		{
			ConstructExpectedPins(scriptFunc);
		}

		static constexpr std::string_view sBranchNodeName = "Branch";
		std::string GetTitle() const override { return std::string{ sBranchNodeName }; };

		static constexpr uint32 sIndexOfEntryPin = 0;
		static constexpr uint32 sIndexOfConditionPin = 1;

		static constexpr uint32 sIndexOfIfPin = 0;
		static constexpr uint32 sIndexOfElsePin = 1;

	private:
		std::optional<InputsOutputs> GetExpectedInputsOutputs(const ScriptFunc&) const override
		{
			return InputsOutputs{
				{ { ScriptPin::sFlow }, { MakeTypeTraits<const bool&>() }},
				{ { ScriptPin::sFlow, "If" }, { ScriptPin::sFlow, "Else" }}
			};
		};
	};

	class ForLoopScriptNode final :
		public FunctionLikeNode
	{
		// ScriptNode constructs an instance of this 
		// object during serialization
		friend class ScriptNode;

		// This constructor is used only when deserializing
		ForLoopScriptNode() :
			FunctionLikeNode(ScriptNodeType::ForLoop) {};

	public:
		ForLoopScriptNode(ScriptFunc& scriptFunc) :
			FunctionLikeNode(ScriptNodeType::ForLoop, scriptFunc)
		{
			ConstructExpectedPins(scriptFunc);
		}

		static constexpr std::string_view sForLoopNodeName = "For";

		std::string GetTitle() const override { return std::string{ sForLoopNodeName }; };

		static constexpr uint32 sIndexOfEntryPin = 0;
		static constexpr uint32 sIndexOfStartPin = 1;
		static constexpr uint32 sIndexOfEndPin = 2;
		static constexpr uint32 sIndexOfBreakPin = 3;

		static constexpr uint32 sIndexOfLoopPin = 0;
		static constexpr uint32 sIndexOfIndexPin = 1;
		static constexpr uint32 sIndexOfExitPin = 2;

	private:
		std::optional<InputsOutputs> GetExpectedInputsOutputs(const ScriptFunc&) const override
		{
			return InputsOutputs{
				{
					{ ScriptPin::sFlow,  "Entry" },
					{ MakeTypeTraits<int32>(), "Start" },
					{ MakeTypeTraits<const int32&>(), "End (Exclusive)" },
					{ ScriptPin::sFlow, "Break" }
				},
				{
					{ ScriptPin::sFlow, "Loop"},
					{ MakeTypeTraits<int32>(), "Index"},
					{ ScriptPin::sFlow, "Exit" }
				}
			};
		};
	};

	class WhileLoopScriptNode final :
		public FunctionLikeNode
	{
		// ScriptNode constructs an instance of this 
		// object during serialization
		friend class ScriptNode;

		// This constructor is used only when deserializing
		WhileLoopScriptNode() :
			FunctionLikeNode(ScriptNodeType::WhileLoop) {};

	public:
		WhileLoopScriptNode(ScriptFunc& scriptFunc) :
			FunctionLikeNode(ScriptNodeType::WhileLoop, scriptFunc)
		{
			ConstructExpectedPins(scriptFunc);
		}

		static constexpr std::string_view sWhileLoopNodeName = "While";

		std::string GetTitle() const override { return std::string{ sWhileLoopNodeName }; };
		static constexpr uint32 sIndexOfEntryPin = 0;
		static constexpr uint32 sIndexOfConditionPin = 1;
		static constexpr uint32 sIndexOfBreakPin = 2;

		static constexpr uint32 sIndexOfLoopPin = 0;
		static constexpr uint32 sIndexOfExitPin = 1;

	private:
		std::optional<InputsOutputs> GetExpectedInputsOutputs(const ScriptFunc&) const override
		{
			return InputsOutputs{
				{
					{ ScriptPin::sFlow, "Entry" },
					{ MakeTypeTraits<const bool&>(), "Condition" },
					{ ScriptPin::sFlow, "Break" }
				},
				{
					{ ScriptPin::sFlow, "Loop" },
					{ ScriptPin::sFlow, "Exit" }
				}
			};
		};
	};
}
