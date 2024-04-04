#pragma once
#include "Core/AssetManager.h"

#include "Meta/MetaTypeId.h"

namespace CE
{
	class MetaType;

	namespace DragDrop
	{
		void SendAsset(Name assetName);

		/* HOW TO RECEIVE ASSETS

		std::optional<WeakAsset<Prefab>> receivedPrefab = DragDrop::PeekAsset<Prefab>();

		if (receivedPrefab.has_value() // Check if there is currently an asset being send
			&& receivedPrefab->GetName() != currentPrefab.GetName() // You can do some additional checks here, to filter out any assets you don't actually want to receive.
			&& DragDrop::AcceptAsset()) // Will only return true if the user released the mousebutton while hovering over the previous element.
		{
			// Congratulations! You just succesfully received an asset!
			currentPrefab = *receivedPrefab;
		}
		*/
		std::optional<WeakAsset<Asset>> PeekAsset(TypeId typenameId);
		std::optional<WeakAsset<Asset>> PeekAsset(const MetaType& assetType);

		template<typename T>
		static std::optional<WeakAsset<T>> PeekAsset()
		{
			std::optional<WeakAsset<Asset>> asAsset = PeekAsset(MakeTypeId<T>());
			static_assert(std::is_base_of_v<Asset, T>);
			return reinterpret_cast<std::optional<WeakAsset<T>>&>(asAsset);
		}

		// Will only return true if the user released the mousebutton while hovering over the previous element.
		bool AcceptAsset();

		void SendEntities(const std::vector<entt::entity>& entity);
		std::optional<std::vector<entt::entity>> PeekEntities();
		bool AcceptEntities();
	}
}
