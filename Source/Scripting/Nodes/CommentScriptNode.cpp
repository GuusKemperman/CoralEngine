#include "Precomp.h"
#include "Scripting/Nodes/CommentScriptNode.h"

#include "GSON/GSONBinary.h"

CE::CommentScriptNode::CommentScriptNode(ScriptFunc& scriptFunc, const std::string& comment) :
	ScriptNode(ScriptNodeType::Comment, scriptFunc),
	mComment(comment)
{
}

void CE::CommentScriptNode::SerializeTo(BinaryGSONObject& to, const ScriptFunc& scriptFunc) const
{
	ScriptNode::SerializeTo(to, scriptFunc);

	to.AddGSONMember("comment") << mComment;
	to.AddGSONMember("size") << mSize;
	to.AddGSONMember("col") << mColour;
}

bool CE::CommentScriptNode::DeserializeVirtual(const BinaryGSONObject& from)
{
	if (!ScriptNode::DeserializeVirtual(from))
	{
		UNLIKELY;
		return false;
	}

	const BinaryGSONMember* comment = from.TryGetGSONMember("comment");
	const BinaryGSONMember* size = from.TryGetGSONMember("size");

	if (comment == nullptr
		|| size == nullptr)
	{
		UNLIKELY;
		LOG(LogAssets, Warning, "Failed to deserialize commentNode, no data serialized");
		return false;
	}

	*comment >> mComment;
	*size >> mSize;

	const BinaryGSONMember* colour = from.TryGetGSONMember("col");

	if (colour != nullptr)
	{
		*colour >> mColour;
	}

	return true;
}
