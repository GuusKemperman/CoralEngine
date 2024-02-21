#pragma once
#include "Scripting/ScriptNode.h"

namespace Engine
{
	class CommentScriptNode final :
		public ScriptNode
	{
		// ScriptNode constructs an instance of this 
		// object during serialization
		friend class ScriptNode;

		// This constructor is used only when deserializing
		CommentScriptNode() :
			ScriptNode(ScriptNodeType::Comment) {};
	public:
		CommentScriptNode(ScriptFunc& scriptFunc, const std::string& comment);

		const std::string& GetComment() const { return mComment; }
		void SetComment(const std::string& comment) { mComment = comment; }

		std::string GetTitle() const override { return mComment; }

		void SerializeTo(BinaryGSONObject& to, const ScriptFunc& scriptFunc) const override;

		glm::vec2 GetSize() const { return mSize; }
		void SetSize(glm::vec2 size) { mSize = size; }

	private:
		bool DeserializeVirtual(const BinaryGSONObject& from) override;

		std::string mComment{};
		glm::vec2 mSize{};
	};
}

