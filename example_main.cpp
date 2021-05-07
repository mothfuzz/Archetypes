#include "archetypes.hpp"
#include <iostream>
#include <cmath>
#include <random>

using namespace archetypes;

struct Position {
    float x;
    float y;
};
struct Velocity {
    float x;
    float y;
};
struct Player {};
struct Invincible {};
struct Enemy {};

void do_physics(Position& p, Velocity& v) {
    //update positions based on velocities
    p.x += v.x;
    p.y += v.y;
}

float distance(Position& a, Position& b) {
    return sqrt((b.x - a.x)*(b.x - a.x) + (b.y - a.y)*(b.y - a.y));
}

int main(int argc, char *argv[]) {

    World w;

    //all at once
    Entity player = w.insert(Player{}, Position{64.0, 64.0}, Velocity{0.0, 0.0});

    //or one at a time
    Entity enemy = w.insert();
    w.insert_component(enemy, Enemy{});
    w.insert_component(enemy, Position{100.0, 64.0});
    w.insert_component(enemy, Velocity{-1.0, 0.0});

    //can query archetypes and check if entities contain a subset
    std::cout << "Player's archetype: " << w.entity_archetype(player) << std::endl;
    std::cout << "Expected archetype: " << archetype<Player, Position, Velocity>() << std::endl;
    if(w.entity_contains(player, archetype<Position>())) {
        std::cout << "ready to go." << std::endl;
    }

    //can add or remove components at runtime
    std::random_device rd;
    std::default_random_engine rng(rd());
    std::uniform_int_distribution<int> zero_or_one(0, 1);
    if(zero_or_one(rng) == 1) {
        //player has the chance to be invincible.
        w.insert_component(player, Invincible{});
    }
    //player doesn't move
    w.remove_component<Velocity>(player);


    std::cout << "beginning update loop..." << std::endl;

    //update loop, demonstrates job system
    do {
        //jobs can be functions
        w.run_job(do_physics);

        //or closures
        w.run_job([](Player& p, Position& pos) {
            //print pertinent info
            std::cout << "Player at\t(" << pos.x << ", " << pos.y << ")" << std::endl;
        });
        w.run_job([](Enemy& e, Position& pos) {
            //print pertinent info
            std::cout << "Enemy at\t(" << pos.x << ", " << pos.y << ")" << std::endl;
        });

        //kill player if touched by an enemy and not invincible
        w.run_job_with_entities([&](Entity pid, Player& p, Position& player_pos){
            w.run_job([&](Enemy& e, Position& enemy_pos) {
                if(distance(player_pos, enemy_pos) <= 1.0) {
                    //can use try_get to get either the component or nullptr
                    if(w.template try_get<Invincible>(pid)) {
                        std::cout << "Player may be invincible but we have to have an exit condition!" << std::endl;
                    } else {
                        std::cout << "Player got killed by an enemy!" << std::endl;
                    }
                    w.remove(pid);
                }
            });
        });

        //remember to call this to persist changes
        w.update();
    }
    while(w.find<Player>().size() > 0);

    std::cout << "done" << std::endl;
    return 0;
}
