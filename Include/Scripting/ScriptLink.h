#pragma once
#include "Scripting/ScriptIds.h"
#include "ScriptErrors.h"

namespace Engine
{
	class ScriptPin;
	class BinaryGSONObject;
	class ScriptFunc;

	class ScriptLink
	{
		ScriptLink() = default;
	public:
		ScriptLink(LinkId id, const ScriptPin& pinA, const ScriptPin& pinB);
			
		LinkId GetId() const { return mId; }

		// Use with care, dangerous.
		void SetId(LinkId id) { mId = id; }

		PinId GetInputPinId() const { return mInput; }
		PinId GetOutputPinId() const { return mOutput; }

		void SerializeTo(BinaryGSONObject& dest) const;
		static std::optional<ScriptLink> DeserializeFrom(const BinaryGSONObject& src);

		void CollectErrors(ScriptErrorInserter inserter, const ScriptFunc& scriptFunc) const;

		// This is used to remap the IDs when duplicating a selection of nodes.
		void GetReferencesToIds(ScriptIdInserter inserter);

	private:
		LinkId mId;
		PinId mInput;
		PinId mOutput;
	};
}