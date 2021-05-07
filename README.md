# Archetypes - header-only archetype-based ECS for C++17.

Designed to be simple, lightweight, easy-to-use, and performant.

Find a guided tour in `example_main.cpp`

Or, to get started, simply drop all the files into your include directory and
    `#include "archetypes.hpp"`

basic functionality:

- World can contain entities composed of components which are any trivially copyable type
- Entities are referred to only by an id
- memory is kept in chunks which are cache-friendly and performant
- all entities with the same set of components (known as an Archetype) are stored in the same chunk for a compact memory layout
- You can run jobs on the World that affect all entities with a certain Archetype
- You can query an entity's Archetype, add and remove components, and insert and delete entities all at runtime
- changes happen only when World::update() is called, in order to preserve iterators during job execution

