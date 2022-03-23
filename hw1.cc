#include <omp.h>
#include <pthread.h>

#include <boost/functional/hash.hpp>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "tbb/concurrent_priority_queue.h"
#include "tbb/concurrent_queue.h"
#include "tbb/concurrent_unordered_map.h"
#include "tbb/concurrent_vector.h"
using namespace std;
using namespace tbb;

// =================Definition================
typedef unordered_set<pair<int, int>, boost::hash<pair<int, int>>> Pos_Set;
typedef pair<int, int> Position;
typedef bitset<256> bs256;

int Rows, Cols;
vector<Position> Targets;
string initMap;
array<array<int, 256>, 256> to1DArray;
array<int, 256> zobrist;
map<pair<int, int>, vector<set<Position>>> deadPointsArray;
bs256 deadMap;
const vector<vector<string>> deadMasks = {
    {
        " ## ",
        "#..#",
        " xx ",
    },
    {
        " xx ",
        "#..#",
        " ## ",
    },
    {
        " # ",
        "#.x",
        "#.x",
        " # ",
    },
    {
        " # ",
        "x.#",
        "x.#",
        " # ",
    }};

enum MatrixCode : char {
    EMPTY = ' ',
    WALL = '#',
    TARGET = '.',
    FRAGILE = '@',
    BOX = 'x',
    BOXT = 'X',
    PLY = 'o',
    PLYT = 'O',
    PLYF = '!',
    NONE = '\0'
};
enum Direction : size_t {
    UP = 0,
    DOWN = 1,
    LEFT = 2,
    RIGHT = 3,
    STOP = 4,
};
char addBox(char c) {
    if (c == EMPTY)
        return BOX;
    else if (c == TARGET)
        return BOXT;
    else
        return NONE;
}
char rmBox(char c) {
    if (c == BOX)
        return EMPTY;
    else if (c == BOXT)
        return TARGET;
    else
        return NONE;
}
char addPly(char c) {
    if (c == EMPTY)
        return PLY;
    else if (c == TARGET)
        return PLYT;
    else if (c == FRAGILE)
        return PLYF;
    else
        return NONE;
}
char rmPly(char c) {
    if (c == PLY)
        return EMPTY;
    else if (c == PLYT)
        return TARGET;
    else if (c == PLYF)
        return FRAGILE;
    else
        return NONE;
}
size_t opposite(size_t dir) {
    switch (dir) {
        case UP:
            return DOWN;
        case DOWN:
            return UP;
        case LEFT:
            return RIGHT;
        default:
            return LEFT;
    }
}
char toKey(size_t dir) {
    switch (dir) {
        case UP:
            return 'W';
        case DOWN:
            return 'S';
        case LEFT:
            return 'A';
        default:
            return 'D';
    }
}
pair<int, int> operator+(pair<int, int> const& lhs, size_t const& rhs);
ostream& operator<<(ostream& os, const pair<int, int>& rhs);

char const& getBlk(Position const& pos);
char const& getBlk(int const& r, int const& c);
size_t to1D(Position const& pos);
size_t to1D(int const& r, int const& c);
class Sokoban {
   public:
    class State {
       private:
        Position pos;
        bs256 boxMap;
        bs256 wallMap;
        string moveSequence;
        int filled;
        bool dead;
        int hashValue;

       public:
        State() : filled(0), dead(false), hashValue(0) {}
        State(Position const& pos, bs256 const& boxMap);
        State(State const& obj);
        ~State() {}
        bool canMovePly(Position const& curpos, int const& dir) const;
        void movePly(int const& dir);
        vector<State*> nextStates() const;
        bool solved() const;
        bool isDead() const;
        int const& getFilled() const;
        bs256 getFloodedMap() const;
        bs256 const& getBoxMap() const;
        int const& getHashValue() const;
        string getMoveSequence() const;
        Position const& getPos() const;
        void print() const;
        State& operator=(const State& rhs);
    };
    class StateCmp {
       public:
        bool operator()(State const* const& lhs, State const* const& rhs) {
            if (lhs->getFilled() < rhs->getFilled())
                return true;
            else if (lhs->getFilled() > rhs->getFilled())
                return false;
            else {
                return false;
            }
        }
    };
    Sokoban() {}
    ~Sokoban() {}
    void getInput(char* file_path);
    string bfs();
    string parallel_bfs();

   private:
    vector<string> input;
    State initState;
};
// =================Implementation==============
pair<int, int> operator+(pair<int, int> const& lhs, size_t const& rhs) {
    if (rhs == UP)
        return make_pair(lhs.first - 1, lhs.second);
    else if (rhs == DOWN)
        return make_pair(lhs.first + 1, lhs.second);
    else if (rhs == LEFT)
        return make_pair(lhs.first, lhs.second - 1);
    else if (rhs == RIGHT)
        return make_pair(lhs.first, lhs.second + 1);
    else
        return lhs;
}
ostream& operator<<(ostream& os, const pair<int, int>& rhs) {
    os << "(" << rhs.first << " , " << rhs.second << ")";
    return os;
}

char const& getBlk(Position const& pos) {
    return initMap[pos.first * Cols + pos.second];
}
char const& getBlk(int const& r, int const& c) {
    return initMap[r * Cols + c];
}
size_t to1D(Position const& pos) {
    return to1D(pos.first, pos.second);
}
size_t to1D(int const& r, int const& c) {
    return to1DArray[r][c];
}

Sokoban::State::State(Position const& pos, bs256 const& boxMap) : State() {
    this->pos = pos;
    this->boxMap = boxMap;
    for (int r = 0; r < Rows; r++) {
        for (int c = 0; c < Cols; c++) {
            if (this->boxMap[to1D(r, c)] && getBlk(r, c) == TARGET) {
                this->filled++;
            }
        }
    }

    for (int r = 0; r < Rows; r++) {
        for (int c = 0; c < Cols; c++) {
            if (this->boxMap[to1D(r, c)]) {
                this->hashValue ^= zobrist[to1D(r, c)];
            }
        }
    }
    for (int r = 0; r < Rows; r++) {
        for (int c = 0; c < Cols; c++) {
            if (getBlk(r, c) == WALL)
                this->wallMap[to1D(r, c)] = 1;
        }
    }
}
Sokoban::State::State(State const& obj) {
    this->boxMap = obj.boxMap;
    this->wallMap = obj.wallMap;
    this->hashValue = obj.hashValue;
    this->pos = obj.pos;
    this->moveSequence = obj.moveSequence;
    this->filled = obj.filled;
    this->dead = obj.dead;
}
Sokoban::State& Sokoban::State::operator=(State const& rhs) {
    this->boxMap = rhs.boxMap;
    this->wallMap = rhs.wallMap;
    this->hashValue = rhs.hashValue;
    this->pos = rhs.pos;
    this->moveSequence = rhs.moveSequence;
    this->filled = rhs.filled;
    this->dead = rhs.dead;
    return *this;
}
bool Sokoban::State::canMovePly(Position const& curpos, int const& dir) const {
    Position nxtPos = curpos + dir, nnxtPos = curpos + dir + dir;
    bool res = false;
    if (getBlk(nxtPos) != WALL)
        switch (getBlk(nxtPos)) {
            case EMPTY:
            case TARGET:
            case FRAGILE:
                if (this->boxMap[to1D(nxtPos)]) {
                    if (getBlk(nnxtPos) != FRAGILE && getBlk(nnxtPos) != WALL && !this->boxMap[to1D(nnxtPos)])
                        res = true;
                } else {
                    res = true;
                }
                break;
            default:
                break;
        }
    return res;
}
void Sokoban::State::movePly(int const& dir) {
    Position nxtPos = this->pos + dir, nnxtPos = this->pos + dir + dir;
    switch (getBlk(nxtPos)) {
        case EMPTY:
        case TARGET:
        case FRAGILE:
            if (this->boxMap[to1D(nxtPos)]) {
                this->boxMap[to1D(nxtPos)] = 0;
                this->boxMap[to1D(nnxtPos)] = 1;

                if (getBlk(nxtPos) == TARGET)
                    this->filled--;
                if (getBlk(nnxtPos) == TARGET)
                    this->filled++;
                this->hashValue ^= zobrist[to1D(nxtPos)];
                this->hashValue ^= zobrist[to1D(nnxtPos)];

                if (getBlk(nnxtPos + UP) == WALL || getBlk(nnxtPos + DOWN) == WALL) {
                    if (getBlk(nnxtPos + LEFT) == WALL || getBlk(nnxtPos + RIGHT) == WALL)
                        this->wallMap[to1D(nnxtPos)] = true;
                }

                if (!this->dead)
                    this->dead = deadMap[to1D(nnxtPos)];
                if (!this->dead)
                    for (size_t v = UP; v < LEFT; v++) {
                        for (size_t h = LEFT; h < STOP; h++) {
                            int tot = 0;
                            if (getBlk(nnxtPos) == WALL || this->boxMap[to1D(nnxtPos)])
                                tot++;
                            if (getBlk(nnxtPos + v) == WALL || this->boxMap[to1D(nnxtPos + v)])
                                tot++;
                            if (getBlk(nnxtPos + h) == WALL || this->boxMap[to1D(nnxtPos + h)])
                                tot++;
                            if (getBlk(nnxtPos + v + h) == WALL || this->boxMap[to1D(nnxtPos + v + h)])
                                tot++;
                            if (tot == 4) {
                                this->wallMap[to1D(nnxtPos)] = 1;
                                this->wallMap[to1D(nnxtPos + v)] = 1;
                                this->wallMap[to1D(nnxtPos + h)] = 1;
                                this->wallMap[to1D(nnxtPos + v + h)] = 1;
                                if (!this->dead)
                                    if (getBlk(nnxtPos) != TARGET)
                                        this->dead = true;
                                if (!this->dead)
                                    if (this->boxMap[to1D(nnxtPos + v)] && getBlk(nnxtPos + v) != TARGET)
                                        this->dead = true;
                                if (!this->dead)
                                    if (this->boxMap[to1D(nnxtPos + h)] && getBlk(nnxtPos + h) != TARGET)
                                        this->dead = true;
                                if (!this->dead)
                                    if (this->boxMap[to1D(nnxtPos + v + h)] && getBlk(nnxtPos + v + h) != TARGET)
                                        this->dead = true;
                            }
                        }
                    }
                if (!this->dead)
                    if (deadPointsArray.find(nnxtPos) != deadPointsArray.end()) {
                        for (auto ptsSet : deadPointsArray[nnxtPos]) {
                            bool flag = true;
                            for (auto pt : ptsSet) {
                                if (!this->boxMap[to1D(pt)])
                                    flag = false;
                            }
                            if (flag)
                                this->dead = true;
                        }
                    }
            }
            break;
        case WALL:
        default:
            break;
    }
    this->pos = pos + dir;
    this->moveSequence.push_back(toKey(dir));
}
vector<Sokoban::State*> Sokoban::State::nextStates() const {
    vector<State*> res;
    queue<pair<Position, string>> que;
    bs256 went;
    que.emplace(pos, "");
    went[to1D(pos)] = 1;

    while (!que.empty()) {
        auto [curPos, curSeq] = que.front();
        que.pop();

        for (size_t dir = UP; dir < STOP; dir++) {
            Position nxtPos = curPos + dir;
            string nxtSeq = curSeq + toKey(dir);
            if (!went[to1D(nxtPos)]) {
                if (this->boxMap[to1D(nxtPos)]) {
                    if (this->canMovePly(curPos, dir)) {
                        State* nxtState = new State(*this);
                        nxtState->pos = curPos;
                        nxtState->moveSequence += curSeq;
                        nxtState->movePly(dir);
                        if (!nxtState->isDead())
                            res.emplace_back(nxtState);
                        else
                            delete nxtState;
                    }
                } else if (/* getBlk(nxtPos) != WALL */ !this->wallMap[to1D(nxtPos)]) {
                    went[to1D(nxtPos)] = 1;
                    que.emplace(nxtPos, nxtSeq);
                }
            }
        }
    }
    return res;
}
bool Sokoban::State::solved() const {
    return this->getFilled() == (int)Targets.size();
}
bool Sokoban::State::isDead() const {
    return this->dead;
}
int const& Sokoban::State::getFilled() const {
    return this->filled;
}
bs256 Sokoban::State::getFloodedMap() const {
    bs256 went;
    queue<Position> que;
    que.emplace(this->pos);
    went[to1D(this->pos)] = 1;
    while (!que.empty()) {
        Position curPos = que.front();
        que.pop();
        for (size_t dir = UP; dir < STOP; dir++) {
            Position nxtPos = curPos + dir;
            if (!went[to1D(nxtPos)]) {
                if (/* getBlk(nxtPos) != WALL */ !this->wallMap[to1D(nxtPos)] && !this->boxMap[to1D(nxtPos)]) {
                    que.emplace(nxtPos);
                    went[to1D(nxtPos)] = 1;
                }
            }
        }
    }
    return went;
}
bs256 const& Sokoban::State::getBoxMap() const {
    return this->boxMap;
}
int const& Sokoban::State::getHashValue() const {
    return this->hashValue;
}
string Sokoban::State::getMoveSequence() const {
    return this->moveSequence;
}
Position const& Sokoban::State::getPos() const {
    return this->pos;
}
void Sokoban::getInput(char* file_path) {
    ifstream input_file;
    string input_line;
    Position ply;
    bs256 boxMap;

    input_file.open(file_path);
    if (!input_file)
        cerr << "No file found.\n";

    while (getline(input_file, input_line))
        input.emplace_back(input_line);

    Rows = input.size();
    Cols = input[0].size();

    for (int r = 0; r < Rows; r++) {
        for (int c = 0; c < Cols; c++) {
            to1DArray[r][c] = r * Cols + c;
        }
    }

    for (int r = 0; r < Rows; r++) {
        for (int c = 0; c < Cols; c++) {
            zobrist[to1D(r, c)] = rand();
        }
    }

    // Build initial map
    for (int r = 0; r < Rows; r++) {
        for (int c = 0; c < Cols; c++) {
            if (input[r][c] == PLY || input[r][c] == PLYT || input[r][c] == PLYF) {
                ply = make_pair(r, c);
                input[r][c] = rmPly(input[r][c]);
            }
            if (input[r][c] == BOX || input[r][c] == BOXT) {
                boxMap[to1D(r, c)] = 1;
                input[r][c] = rmBox(input[r][c]);
            }
            if (input[r][c] == TARGET)
                Targets.emplace_back(r, c);
        }
        initMap.append(input[r]);
    }
    // Build dead points array
    for (auto deadMask : deadMasks) {
        for (int row = 0; row < Rows - deadMask.size() + 1; row++) {
            for (int col = 0; col < Cols - deadMask[0].size() + 1; col++) {
                bool flag = true;
                int tg = 0, wl = 0;
                set<Position> pts;
                for (int r = 0; flag && r < deadMask.size(); r++) {
                    for (int c = 0; flag && c < deadMask[0].size(); c++) {
                        if (deadMask[r][c] == '#') {
                            if (getBlk(row + r, col + c) == WALL)
                                wl++;
                        } else if (deadMask[r][c] == 'x' || deadMask[r][c] == '.') {
                            if (getBlk(row + r, col + c) == TARGET)
                                tg++;
                            if (deadMask[r][c] == 'x')
                                pts.emplace(row + r, col + c);
                        }
                    }
                }
                if (tg < 2 && wl >= 4) {
                    for (auto pt1 : pts) {
                        deadPointsArray[pt1].emplace_back(pts);
                    }
                }
            }
        }
    }
    // Build dead map
    for (int row = 0; row < Rows; row++) {
        for (int col = 0; col < Cols; col++) {
            Position curpos(row, col);
            if (getBlk(curpos) == EMPTY) {
                if (!deadMap[to1D(curpos)])
                    for (size_t v = UP; v <= DOWN; v++) {
                        Position leftpos = curpos;
                        Position rightpos = curpos;
                        while (getBlk(leftpos) == EMPTY && getBlk(leftpos + v) == WALL) leftpos = leftpos + LEFT;
                        while (getBlk(rightpos) == EMPTY && getBlk(rightpos + v) == WALL) rightpos = rightpos + RIGHT;
                        if (getBlk(leftpos) == WALL && getBlk(rightpos) == WALL)
                            deadMap[to1D(curpos)] = true;
                    }
                if (!deadMap[to1D(curpos)])
                    for (size_t h = LEFT; h <= RIGHT; h++) {
                        Position uppos = curpos;
                        Position downpos = curpos;
                        while (getBlk(uppos) == EMPTY && getBlk(uppos + h) == WALL) uppos = uppos + UP;
                        while (getBlk(downpos) == EMPTY && getBlk(downpos + h) == WALL) downpos = downpos + DOWN;
                        if (getBlk(uppos) == WALL && getBlk(downpos) == WALL)
                            deadMap[to1D(curpos)] = true;
                    }
                if (!deadMap[to1D(curpos)])
                    if (getBlk(curpos + UP) == WALL || getBlk(curpos + DOWN) == WALL) {
                        if (getBlk(curpos + LEFT) == WALL || getBlk(curpos + RIGHT) == WALL)
                            deadMap[to1D(curpos)] = true;
                    }
            }
        }
    }

    // Create initial state
    initState = State(ply, boxMap);
    return;
}
string Sokoban::bfs() {
    string ans;
    bool solved = false;
    priority_queue<State*, vector<State*>, StateCmp> statesQue;
    unordered_map<int, bs256> statesMap;
    statesQue.emplace(new State(initState));
    statesMap[initState.getHashValue()] |= initState.getFloodedMap();

    while (!statesQue.empty() && !solved) {
        auto curState = statesQue.top();
        statesQue.pop();
        auto nextStates = curState->nextStates();
        for (auto nxtState : nextStates) {
            // nxtState->print();
            // cout << "\n";
            auto it = statesMap.find(nxtState->getHashValue());
            if (it == statesMap.end() || !it->second[to1D(nxtState->getPos())]) {
                if (nxtState->solved()) {
                    ans = nxtState->getMoveSequence();
                    solved = true;
                }
                statesQue.emplace(nxtState);
                statesMap[nxtState->getHashValue()] |= nxtState->getFloodedMap();
            } else {
                delete nxtState;
            }
        }
        delete curState;
    }
    while (!statesQue.empty()) {
        delete statesQue.top();
        statesQue.pop();
    }
    return ans;
}
string Sokoban::parallel_bfs() {
    string ans;
    bool solved = false;
    concurrent_priority_queue<State*, StateCmp> statesQue;
    concurrent_unordered_map<int, bs256> statesMap;
    statesQue.emplace(new State(initState));
    statesMap[initState.getHashValue()] |= initState.getFloodedMap();

    while (!statesQue.empty() && !solved) {
        int maxThreads = min((int)statesQue.size(), 6);
#pragma omp parallel for schedule(dynamic) num_threads(maxThreads)
        for (int i = 0; i < maxThreads; i++) {
            State* currState;
            statesQue.try_pop(currState);
            auto nextStates = currState->nextStates();
            for (auto nxtState : nextStates) {
                auto it = statesMap.find(nxtState->getHashValue());
                if (it == statesMap.end() || !it->second[to1D(nxtState->getPos())]) {
                    if (nxtState->solved()) {
#pragma omp critical
                        {
                            ans = nxtState->getMoveSequence();
                            solved = true;
                        }
                    }
                    statesQue.emplace(nxtState);
                    statesMap[nxtState->getHashValue()] |= nxtState->getFloodedMap();
                } else {
                    delete nxtState;
                }
            }
            delete currState;
        }
    }
    while (!statesQue.empty()) {
        State* currState;
        statesQue.try_pop(currState);
        delete currState;
    }
    return ans;
}

int main(int argc, char* argv[]) {
    if (argc != 2)
        cerr << "Input Error...\n";
    srand(time(NULL));
    Sokoban sokoban;
    sokoban.getInput(argv[1]);
    cout << sokoban.parallel_bfs() << "\n";
    return 0;
}
