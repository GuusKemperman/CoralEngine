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

		WeakAssetHandle<Prefab> receivedPrefab = DragDrop::PeekAsset<Prefab>();

		if (receivedPrefab != nullptr // Check if there is currently an asset being send
			&& receivedPrefab->GetMetaData().GetName() != currentPrefab.GetMetaData().GetName() // You can do some additional checks here, to filter out any assets you don't actually want to receive.
			&& DragDrop::AcceptAsset()) // Will only return true if the user released the mousebutton while hovering over the previous element.
		{
			// Congratulations! You just succesfully received an asset!
			currentPrefab = *receivedPrefab;
		}
		*/
		WeakAssetHandle<> PeekAsset(TypeId assetTypeId);

		template<typename T>
		static WeakAssetHandle<T> PeekAsset()
		{
			WeakAssetHandle<> asAsset = PeekAsset(MakeTypeId<T>());
			return StaticAssetHandleCast<T>(asAsset);
		}

		// Will only return true if the user released the mousebutton while hovering over the previous element.
		bool AcceptAsset();

		void SendEntities(const std::vector<entt::entity>& entity);
		std::optional<std::vector<entt::entity>> PeekEntities();
		bool AcceptEntities();
	}
}
