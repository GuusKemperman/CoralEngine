#pragma once

namespace CE
{
	class MetaType;

	uint32 GetClassVersion(const MetaType& type);
	void SetClassVersion(MetaType& type, uint32 version);
}