#include <vector>
#include <map>
#include <cstddef>
#include <algorithm>
#include <stdexcept>
#include "typeid.hpp"
#pragma once

namespace archetypes {

	//this is very bad but I'm a good person so it evens out
	struct Buffer {
		std::vector<std::byte> data;
		std::size_t item_size;
		Buffer(size_t item_size) : item_size(item_size) {}
		template<typename T>
		T& at(unsigned int index) {
			if (sizeof(T) != item_size) {
				throw std::runtime_error("wrong item type or wrong size");
			}
			return *reinterpret_cast<T*>(&data[index * item_size]);
		}
		template<typename T>
		void push_back(T item) {
			static_assert(std::is_trivially_copyable_v<T>, "type must be blittable!");
			if (sizeof(T) != item_size) {
				throw std::runtime_error("wrong item type or wrong size");
			}
			std::byte* begin = reinterpret_cast<std::byte*>(&item);
			std::byte* end = begin + sizeof(T);
			data.reserve(end - begin);
			data.insert(data.end(), begin, end);
		}
		void remove(unsigned int index) {
			auto begin = data.begin() + index * item_size;
			auto end = begin + item_size;
			data.erase(begin, end);
		}
		template<typename T>
		T& last() {
			return at<T>((data.size() - 1) / sizeof(T));
		}
	};

	//good ol' fashioned type-erased block o' bytes
	struct Chunk {
		unsigned long long archetype;
		std::vector<unsigned int> entities;
		std::map<unsigned long long, Buffer> data;

		Chunk(unsigned long long arch) : archetype(arch) {
			int counter = 0;
			unsigned long long arch_iter = arch;
			while (arch_iter) {
				if (auto type_id = (arch_iter & 1) << counter) {
					data.insert({ type_id, Buffer(type_sizes.at(type_id)) });
				}
				arch_iter >>= 1;
				counter++;
			}
		}

		unsigned int find_entity_location(unsigned int e) {
			return std::find(entities.begin(), entities.end(), e) - entities.begin();
		}

		template<typename T>
		T& get(unsigned int e) {
			return data.at(get_type_id<T>()).template at<T>(find_entity_location(e));
		}

		template<typename T>
		void push_back_items(T head) {
			data.at(get_type_id<T>()).push_back(head);
		}
		template<typename T, typename... U>
		void push_back_items(T head, U... tail) {
			push_back_items(head);
			push_back_items(tail...);
		}
		template<typename... T>
		void push_back(unsigned int e, T... items) {
			entities.push_back(e);
			if constexpr (sizeof...(T) > 0) {
				push_back_items(items...);
			}
		}

		template<typename T>
		void insert(unsigned int e, T item) {
			unsigned int entity_index = find_entity_location(e);
			data.at(get_type_id<T>()).template at<T>(entity_index) = item;
		}

		void transfer_entity_to(Chunk& new_chunk, unsigned int e) {
			unsigned int entity_index = find_entity_location(e);
			new_chunk.entities.push_back(e);
			for (auto& [k, v] : data) {
				//skip over the types it doesn't have
				if (new_chunk.data.find(k) == new_chunk.data.end()) {
					continue;
				}
				std::vector<std::byte>& new_data = new_chunk.data.at(k).data;
				std::size_t type_size = type_sizes.at(k);
				auto begin = v.data.begin() + entity_index * type_size;
				auto end = begin + type_size;
				new_data.reserve(end - begin);
				new_data.insert(new_data.end(), begin, end);
			}
			remove_entity(e);
		}
		void remove_entity(unsigned int e) {
			unsigned int entity_index = find_entity_location(e);
			for (auto& [_, v] : data) {
				v.remove(entity_index);
			}
			entities.erase(entities.begin() + entity_index);
		}
	};

}
