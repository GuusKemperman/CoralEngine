#pragma once
#include "Scripting/Nodes/FunctionLikeScriptNode.h"

namespace Engine
{
	class MetaType;
	class MetaFunc;

	// Calls a meta function
	class MetaFuncScriptNode final :
		public FunctionLikeNode
	{
		// ScriptNode constructs an instance of this 
		// object during serialization
		friend class ScriptNode;

		// This constructor is used only when deserializing
		MetaFuncScriptNode() :
			FunctionLikeNode(ScriptNodeType::FunctionCall) {};
	public:
		MetaFuncScriptNode(ScriptFunc& scriptFunc, const MetaType& typeWhichThisFuncIsFrom, const MetaFunc& func);

		const std::string& GetTypeName() const { return mTypeName; }
		const std::variant<std::string, OperatorType>& GetFuncNameOrType() const { return mNameOrType; }

		std::string GetSubTitle() const override { return mTypeName; }
		std::string GetTitle() const override;

		void SerializeTo(BinaryGSONObject& to, const ScriptFunc& scriptFunc) const override;

		const MetaFunc* TryGetOriginalFunc() const { return mCachedFunc; }

		glm::vec4 GetHeaderColor(const ScriptFunc& scriptFunc) const override;

		void PostDeclarationRefresh(ScriptFunc& scriptFunc) override;

	private:
		bool DeserializeVirtual(const BinaryGSONObject& from) override;

		std::optional<InputsOutputs> GetExpectedInputsOutputs(const ScriptFunc&) const override;

		const MetaFunc* mCachedFunc{};

		// The type that holds the function
		std::string mTypeName{};
		std::variant<std::string, OperatorType> mNameOrType{};
	};
}