#pragma once

namespace CE
{
	// Sometimes we save only the hash of a name
	// to a file. This map can be used to look up
	// the full name. This helps with missing asset
	// references, as we can then display the full
	// name to the user.
	// Note that sNameLookUp may be modified from
	// multiple threads. Use sNameLookUpMutex.
	inline std::unordered_map<Name::HashType, std::string> sNameLookUp{};
	inline std::mutex sNameLookUpMutex{};
}