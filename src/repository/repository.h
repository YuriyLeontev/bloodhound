#pragma once

#include "../service/save_scores.h"

namespace repository {

class SaveScoresFactory {
public:
    virtual std::unique_ptr<service::SaveScores> CreateSaveScores() = 0;
protected:
    ~SaveScoresFactory() = default;
};


class Database {
public:
    virtual SaveScoresFactory& GetSaveScoresFactory() = 0;
};

} // namespace repository