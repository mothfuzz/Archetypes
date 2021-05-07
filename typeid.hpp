#include <map>
#pragma once

namespace archetypes {

	unsigned long long base_type_id = 1;
	unsigned long long add_type() {
		unsigned long long id = base_type_id;
		base_type_id <<= 1;
		return id;
	}
	std::map<unsigned long long, std::size_t> type_sizes;
	template<typename T>
	unsigned long long get_type_id() {
		static unsigned long long type_id = add_type();
		type_sizes[type_id] = sizeof(T);
		return type_id;
	}

	template<typename T, typename... U>
	unsigned long long archetype() {
		if constexpr (sizeof...(U) == 0) {
			return get_type_id<T>();
		}
		else {
			return get_type_id<T>() | archetype<U...>();
		}
	}
}
