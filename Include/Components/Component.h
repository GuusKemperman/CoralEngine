#pragma once


namespace Engine
{
	/*
	Some component require extra steps during serialization. This can be done by adding the following function:

	FUNCTION()
	void OnSerialize(BinaryGSONObject& serializeTo, const entt::entity owner, const World& world) const;

	TryGetComponentOnSerialize returns a pointer to this function, if it exists.
	*/
	constexpr Name sComponentCustomSerializeFuncName = "OnSerialize"_Name;

	/*
	Some component require extra steps during deserialization. This can be done by adding the following function:

	FUNCTION()
	void OnSerialize(BinaryGSONObject& serializeTo, const entt::entity owner, const World& world) const;

	TryGetComponentOnDeserialize returns a pointer to this function, if it exists.
	*/
	constexpr Name sComponentCustomDeserializeFuncName = "OnDeserialize"_Name;

	/*
	Some components may have a custom OnInspect function.

	FUNCTION()
	static void OnInspect(Registry& registry, const std::vector<entt::entity>& entities);
	*/
	constexpr Name sComponentCustomOnInspectFuncName = "OnInspect"_Name;
}


