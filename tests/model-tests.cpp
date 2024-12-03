#include <cmath>
#include <vector>
#include <catch2/catch_test_macros.hpp>

#include "../src/model/model.h"

using namespace std::literals;
using namespace  model;
using namespace  loot_gen;

using DogRetire = boost::signals2::signal<void(model::Dog::Id dog, const model::Map::Id& map)>;

SCENARIO("Loot spawn") {
    GIVEN("Game session with 1 map with 1 road") {
        Map map(Map::Id{"id"s}, "name"s);
        map.SetDogSpeed(1).SetDogBagCapacity(3);
        map.AddLootWorth(1);
        Road road(Road::HORIZONTAL, {0 ,0}, 10);
        map.AddRoad(road);
        DogRetire test;
        GameSession session(&map, 0, false, {5s, 1.0},100000, test);
        WHEN("Players not joined") {
            session.OnTick(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<size_t, std::milli>(50)));
            THEN("Loot object must not be spawned") {
                REQUIRE(session.GetLootObjects().size() == 0);
            }
        }        
        AND_WHEN("Players joined") {
            auto dog = session.NewDog("dog"s);
            session.OnTick(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<size_t, std::milli>(50)));
            THEN("Loot object must be spawned") {
                REQUIRE(session.GetLootObjects().size() == 1);
            }
            auto dog2 = session.NewDog("dog2"s);
            session.OnTick(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<size_t, std::milli>(50)));            
            THEN("Loot object must be spawned") {
                REQUIRE(session.GetLootObjects().size() == 2);
            }             
        }
    }
    AND_GIVEN("Game session with 2 map with 1 road"){
        Map map1(Map::Id{"map1"s}, "Map1"s);
        Road road1(Road::HORIZONTAL, {0 ,0}, 10);
        map1.SetDogSpeed(1).SetDogBagCapacity(3);
        map1.AddLootWorth(1);
        map1.AddRoad(road1);

        Map map2(Map::Id{"map2"s}, "Map2"s);
        Road road2(Road::VERTICAL, {0 ,0}, 10);
        map2.SetDogSpeed(1).SetDogBagCapacity(3);
        map2.AddLootWorth(1);
        map2.AddRoad(road2);
        
        DogRetire test;
        std::vector<GameSession> sessions;
        sessions.emplace_back(GameSession{&map1, 0, false, {5s, 1.0}, 100000, test});
        sessions.emplace_back(GameSession{&map1, 0, false, {5s, 1.0}, 100000, test});
        sessions.emplace_back(GameSession{&map2, 0, false, {5s, 1.0}, 100000, test});
        sessions.emplace_back(GameSession{&map2, 0, false, {5s, 1.0}, 100000, test});

        WHEN("Players not joined") {
            auto dog = sessions.at(2).NewDog("dog"s);

            for (auto&& it : sessions){
                it.OnTick(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<size_t, std::milli>(5)));
            }

            THEN("Loot object must be spawned session3") {
                for (auto session = 0 ; session < sessions.size(); session++){
                    if (session == 2)
                        REQUIRE(sessions[session].GetLootObjects().size() == 1);
                    else
                        REQUIRE(sessions[session].GetLootObjects().size() == 0);
                }
            }
        }  
    }
}