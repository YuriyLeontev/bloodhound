#pragma once

#include "player.h"

namespace service {

class SaveScores {
public:
    virtual service::RetiredPlayerRepository& PlayerRepository() = 0;
    virtual void Commit() = 0;
    virtual ~SaveScores() = default;
};


}