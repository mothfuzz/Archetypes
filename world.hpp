#include <stdexcept>
#include <functional>
#include <type_traits>
#include <any>
#include <optional>
#include "typeid.hpp"
#include "chunks.hpp"
#pragma once

namespace archetypes {

	using Archetype = unsigned long long;
	using Entity = unsigned int;
	class World {
		unsigned int base_entity = 0;
		std::map<Archetype, Chunk> chunks;
		//std::map<Entity, Archetype> owners; //possibly for quick entity->chunk lookup
		std::map<Archetype, Chunk> insert_queue; //insert, add_component, remove_component
		std::vector<Entity> remove_queue;
		//std::map<Archetype, std::function(job)> job_queue; //not really possible since jobs are strongly typed

	private:
		Chunk& find_chunk(Archetype arch, bool insert = false) {
			std::map<Archetype, Chunk>* all_chunks = &chunks;
			if (insert) {
				//use ptr instead of ref because of assignment passthrough issues
				all_chunks = &insert_queue;
			}
			auto map_key = all_chunks->find(arch);
			if (map_key == all_chunks->end()) {
				//create a new chunk
				all_chunks->insert({ arch, Chunk(arch) });
				return all_chunks->at(arch);
			}
			else {
				//return existing chunk
				return map_key->second;
			}
		}
		Chunk& find_insert_queue(Archetype arch) {
			return find_chunk(arch, true);
		}

		Chunk& find_owning_chunk(Entity e) {
			//return owners.at(e);
			for (auto& [_, v] : chunks) {
				if (std::find(v.entities.begin(), v.entities.end(), e) != v.entities.end()) {
					//in the case of an entity existing in both a real chunk and a queued chunk (as in the case of insert_component)
					//real chunk takes priority.
					return v;
				}
			}
			for (auto& [_, v] : insert_queue) {
				if (std::find(v.entities.begin(), v.entities.end(), e) != v.entities.end()) {
					return v;
				}
			}
			throw std::runtime_error("entity does not belong to any chunk");
		}

	public:
		Entity insert() {
			find_insert_queue(0).push_back(base_entity);
			return base_entity++;
		}
		template<typename... T>
		Entity insert(T... items) {
			find_insert_queue(archetype<T...>()).push_back(base_entity, items...);
			return base_entity++;
		}
		template<typename... T>
		Entity insert(std::tuple<T...> items) {
			return insert(std::get<T>(items)...);
		}
		void remove(Entity e) {
			remove_queue.push_back(e);
		}

		unsigned int entity_chunk_size(Entity e) {
			return find_owning_chunk(e).entities.size();
		}

		template<typename T>
		void insert_component(Entity e, T item) {
			Chunk& c_old = find_owning_chunk(e);
			//find or create chunk with expanded archetype
			Chunk& c_new = find_insert_queue(c_old.archetype | archetype<T>());
			c_old.transfer_entity_to(c_new, e);
			c_new.push_back_items<T>(item);
		}
		template<typename T>
		void remove_component(Entity e) {
			Chunk& c_old = find_owning_chunk(e);
			//find or create chunk with reduced archetype
			Chunk& c_new = find_insert_queue(c_old.archetype ^ archetype<T>());
			c_old.transfer_entity_to(c_new, e);
		}

		template<typename T>
		T* try_get(Entity e) {
			Chunk& c = find_owning_chunk(e);
			if ((c.archetype & get_type_id<T>()) == get_type_id<T>()) {
				return &c.get<T>(e);
			}
			else {
				return nullptr;
			}
		}

		template<typename T>
		T& get(Entity e) {
			Chunk& c = find_owning_chunk(e);
			return c.get<T>(e);
		}

		template<typename... T>
		std::vector<Entity> match_entities() {
			return match_entities(archetype<T...>());
		}
		std::vector<Entity> match_entities(Archetype arch) {
			std::vector<Entity> entities;
			for (auto& [carch, c] : chunks) {
				if ((carch & arch) == arch) {
					entities.insert(entities.end(), c.entities.begin(), c.entities.end());
				}
			}
			return entities;
		}
		bool entity_contains(Entity e, Archetype arch) {
			return (find_owning_chunk(e).archetype & arch) == arch;
		}

		Archetype entity_archetype(Entity e) {
			return find_owning_chunk(e).archetype;
		}

		/*
		template<typename... Args>
		std::vector<std::tuple<Args&...>> find() {
			std::vector<std::tuple<Args&...>> ret;
			run_job([&](World& w, Args&... args){
			ret.push_back(std::make_tuple(args));
			});
			return ret;
		}
		*/

		void update() {
			//run all queued jobs
			//

			//delete pending entities
			for (auto e : remove_queue) {
				find_owning_chunk(e).remove_entity(e);
			}
			remove_queue.clear();
			
			//insert new entities
			for (auto& [arch, chunk] : insert_queue) {
				std::vector entities = chunk.entities; //cloned
				for (auto e : entities) {
					chunk.transfer_entity_to(find_chunk(arch), e);
				}
			}
			insert_queue.clear();
		}

		template<typename F>
		void run_job(F func) {
			run_job(std::function(func));
		}

		template<typename... Args>
		void run_job(std::function<void(Args&...)> func) {
			for (auto& [k, v] : chunks) {
				//if chunk is compatible with archetype
				if ((archetype<Args...>() & k) == archetype<Args...>()) {
					//we could split into threads here since chunks are independent memory-wise
					for (Entity e : v.entities) {
						func(v.template get<Args>(e)...);
					}
					//
				}
			}
		}

		template<typename F>
		void run_job_with_entities(F func) {
			run_job_with_entities(std::function(func));
		}
		template<typename... Args>
		void run_job_with_entities(std::function<void(Entity, Args&...)> func) {
			for (auto& [k, v] : chunks) {
				//if chunk is compatible with archetype
				if ((archetype<Args...>() & k) == archetype<Args...>()) {
					//we could split into threads here since chunks are independent memory-wise
					for (Entity e : v.entities) {
						func(e, v.template get<Args>(e)...);
					}
					//
				}
			}
		}

		template<typename... Args>
		std::vector<std::tuple<Args&...>> find() {
			std::vector<std::tuple<Args&...>> ret;
			for (auto& [k, v] : chunks) {
				//if chunk is compatible with archetype
				if ((archetype<Args...>() & k) == archetype<Args...>()) {
					for (Entity e : v.entities) {
						ret.push_back({ v.template get<Args>(e)... });
					}
				}
			}
			return ret;
		}

		template<typename... Args>
		std::tuple<Args&...> find_one() {
			return find<Args...>()[0];
		}
	};

}
