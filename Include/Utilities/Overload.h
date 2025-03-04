#pragma once

namespace CE
{
	template<class... Ts>
	struct Overload : Ts... { using Ts::operator()...; };
	
	template<class... Ts>
	Overload(Ts...) -> Overload<Ts...>;
}