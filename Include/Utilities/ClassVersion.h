#pragma once

namespace Engine
{
	class MetaType;

	uint32 GetClassVersion(const MetaType& type);
	void SetClassVersion(MetaType& type, uint32 version);
}