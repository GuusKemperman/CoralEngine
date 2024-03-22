#pragma once
#include "Scripting/Nodes/FunctionLikeScriptNode.h"

namespace Engine
{
	class MetaField;

	// The common parts between the Setter and Getter node
	class NodeInvolvingMetaMember :
		public FunctionLikeNode
	{
	protected:
		// ScriptNode constructs an instance of this 
		// object during serialization
		friend class ScriptNode;

		// This constructor is used only when deserializing
		NodeInvolvingMetaMember(const ScriptNodeType type) :
			FunctionLikeNode(type) {};

		NodeInvolvingMetaMember(ScriptNodeType type,
		                        ScriptFunc& scriptFunc,
		                        const MetaField& field);

	public:
		void SerializeTo(BinaryGSONObject& to, const ScriptFunc& scriptFunc) const override;

		const MetaField* TryGetOriginalMemberData() const { return mCachedField; }

		std::string GetSubTitle() const override { return mTypeName; }
		glm::vec4 GetHeaderColor(const ScriptFunc&) const override { return glm::vec4{ .5f, .25f, .25f, 1.0f }; }

		void PostDeclarationRefresh(ScriptFunc& scriptFunc) final;

	protected:
		bool DeserializeVirtual(const BinaryGSONObject& from) override;

		const MetaField* mCachedField{};

		// The type that holds the field
		std::string mTypeName{};
		std::string mMemberName{};
	};

	class SetterScriptNode final :
		public NodeInvolvingMetaMember
	{
		// ScriptNode constructs an instance of this 
		// object during serialization
		friend class ScriptNode;

		// This constructor is used only when deserializing
		SetterScriptNode() :
			NodeInvolvingMetaMember(ScriptNodeType::Setter) {};
	public:
		SetterScriptNode(ScriptFunc& scriptFunc, const MetaField& field);

		static std::string GetTitle(std::string_view memberName)
		{
			return Format("Set {}", memberName);
		}

		std::string GetTitle() const override { return GetTitle(mMemberName); }

	private:
		std::optional<InputsOutputs> GetExpectedInputsOutputs(const ScriptFunc&) const override;
	};

	class GetterScriptNode final :
		public NodeInvolvingMetaMember
	{
		// ScriptNode constructs an instance of this 
		// object during serialization
		friend class ScriptNode;

		// This constructor is used only when deserializing
		GetterScriptNode() :
			NodeInvolvingMetaMember(ScriptNodeType::Getter) {};
	public:
		GetterScriptNode(ScriptFunc& scriptFunc, const MetaField& field, bool returnsCopy);

		static std::string GetTitle(std::string_view memberName, bool returnsCopy);

		std::string GetTitle() const override { return GetTitle(mMemberName, mReturnsCopy); }

		bool DoesNodeReturnCopy() const { return mReturnsCopy; }

		void SerializeTo(BinaryGSONObject& to, const ScriptFunc& scriptFunc) const override;

	private:
		bool DeserializeVirtual(const BinaryGSONObject& from) override;

		std::optional<InputsOutputs> GetExpectedInputsOutputs(const ScriptFunc&) const override;

		// Whether this gets a copy
		// or a reference
		bool mReturnsCopy{};
	};
}
