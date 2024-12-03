#pragma once

#include "../model/model.h"
#include <boost/json.hpp>
#include <unordered_map>

namespace extra_data {

struct ExtraData {
    using MapIdToLootTypes = std::unordered_map<model::Map::Id, boost::json::array, util::TaggedHasher<model::Map::Id>>;
    MapIdToLootTypes map_id_to_loot_types;
};

}