#include <tuple>
#pragma once

namespace archetypes {

    template<typename... T>
    class Prefab {
    public:
	std::tuple<T...> items;
	Prefab(T... items): items(items...) {}

	std::tuple<T...> build() {
	    return items;
	};
	template<typename TT>
	TT& get() {
	    return std::get<TT>(items);
	}
    };
    //inherit from prefab for easier creation of stuff.
    //note: cannot automatically concat prefabs, but you can use
    //std::tuple_cat(prefab1.build(), prefab2.build())
    //if you want.
}

