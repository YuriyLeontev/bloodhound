CREATE TABLE IF NOT EXISTS retired_players (
    id UUID CONSTRAINT firstindex PRIMARY KEY,
    name varchar(100) NOT NULL,
    score INT NOT NULL,
    play_time_ms INT NOT NULL
);

CREATE UNIQUE INDEX IF NOT EXISTS score_play_time_idx ON retired_players (score DESC, play_time_ms, name);
