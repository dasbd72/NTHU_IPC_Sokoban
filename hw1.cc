#include <omp.h>
#include <pthread.h>

#include <boost/functional/hash.hpp>
#include <fstream>
#include <iostream>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
using namespace std;

// g++ -std=c++17 -O3 -pthread -fopenmp -Wall -Wextra -fsanitize=address -g hw1.cc -o hw1; time ./hw1 samples/16.txt
// g++ -std=c++17 -O3 -pthread -fopenmp -Wall -Wextra -fsanitize=address -g hw1.cc -o hw1; time ./hw1 samples/06.txt > hw1.txt
// g++ -std=c++17 -O3 -pthread -fopenmp -Wall -Wextra -g hw1.cc -o hw1; time ./hw1 test.txt

// =================Definition================
typedef unordered_set<pair<int, int>, boost::hash<pair<int, int>>> Pos_Set;
typedef pair<int, int> Position;
typedef bitset<256> bs256;

int Rows, Cols;
vector<Position> Targets;
string initMap;
array<int, 256> distScoreMap;

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
pair<int, int> operator-(pair<int, int> const& lhs, pair<int, int> const& rhs);
size_t operator/(pair<int, int> const& rhs, pair<int, int> const& lhs);
ostream& operator<<(ostream& os, const pair<int, int>& rhs);

char const& getBlk(Position const& pos);
char const& getBlk(int const& r, int const& c);
size_t to1D(Position const& pos);
size_t to1D(int const& r, int const& c);
class Sokoban {
   public:
    class State {
       private:
        // TODO : new variable
        Position pos;
        bs256 boxMap;
        bs256 wallMap;
        string moveSequence;
        int filled;
        int distScore;
        bool dead;

       public:
        State() : filled(0), distScore(0), dead(false) {}  // TODO : new variable
        State(Position const& pos, bs256 const& boxMap);
        State(State const& obj);
        ~State() {}
        bool canMovePly(Position const& curpos, int const& dir) const;
        void movePly(int const& dir);
        vector<State> nextStates() const;
        bool solved() const;
        bool isDead() const;
        int const& getFilled() const;
        int const& getDistScore() const;
        bs256 getFloodedMap() const;
        bs256 const& getBoxMap() const;
        string const& getMoveSequence() const;
        Position const& getPos() const;
        void print() const;
        State& operator=(const State& rhs);
    };
    class StateCmp {
       public:
        bool operator()(State const& lhs, State const& rhs) {
            if (lhs.getFilled() < rhs.getFilled())
                return true;
            else if (lhs.getFilled() > rhs.getFilled())
                return false;
            else
                // return false;
                return lhs.getDistScore() < rhs.getDistScore();
        }
    };
    Sokoban() {}
    ~Sokoban() {}
    void getInput(char* file_path);
    void bfs();

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
pair<int, int> operator-(pair<int, int> const& lhs, pair<int, int> const& rhs) {
    pair<int, int> delt;
    delt.first = rhs.first - lhs.first;
    delt.second = rhs.second - lhs.second;
    return delt;
}
size_t operator/(pair<int, int> const& rhs, pair<int, int> const& lhs) {
    auto delt = rhs - lhs;
    if (delt == make_pair(-1, 0))
        return 0;
    else if (delt == make_pair(1, 0))
        return UP;
    else if (delt == make_pair(0, -1))
        return DOWN;
    else if (delt == make_pair(0, 1))
        return LEFT;
    else
        return RIGHT;
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
    return r * Cols + c;
}

Sokoban::State::State(Position const& pos, bs256 const& boxMap) {
    // TODO : new variable
    this->dead = false;
    this->distScore = 0;
    this->filled = 0;
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
                this->distScore += distScoreMap[to1D(r, c)];
            }
        }
    }
    for (int r = 0; r < Rows; r++) {
        for (int c = 0; c < Cols; c++) {
            if (getBlk(r, c) == WALL)
                this->wallMap[to1D(r, c)] = 1;
            else if (this->boxMap[to1D(r, c)]) {
                if (getBlk(Position(r, c) + UP) == WALL || getBlk(Position(r, c) + DOWN) == WALL) {
                    if (getBlk(Position(r, c) + LEFT) == WALL || getBlk(Position(r, c) + RIGHT) == WALL)
                        this->wallMap[to1D(r, c)] = 1;
                }
            }
        }
    }
}
Sokoban::State::State(State const& obj) {
    // TODO : new variable
    this->boxMap = obj.boxMap;
    this->wallMap = obj.wallMap;
    this->pos = obj.pos;
    this->moveSequence = obj.moveSequence;
    this->filled = obj.filled;
    this->dead = obj.dead;
    this->distScore = obj.distScore;
}
Sokoban::State& Sokoban::State::operator=(State const& rhs) {
    // TODO : new variable
    this->boxMap = rhs.boxMap;
    this->wallMap = rhs.wallMap;
    this->pos = rhs.pos;
    this->moveSequence = rhs.moveSequence;
    this->filled = rhs.filled;
    this->dead = rhs.dead;
    this->distScore = rhs.distScore;
    return *this;
}
bool Sokoban::State::canMovePly(Position const& curpos, int const& dir) const {
    Position nxtPos = curpos + dir, nnxtPos = curpos + dir + dir;
    bool res = false;
    if (!this->wallMap[to1D(nxtPos)])
        switch (getBlk(nxtPos)) {
            case EMPTY:
            case TARGET:
            case FRAGILE:
                if (this->boxMap[to1D(nxtPos)]) {
                    if (getBlk(nnxtPos) != FRAGILE /* && getBlk(nnxtPos) != WALL */ && !this->wallMap[to1D(nnxtPos)] && !this->boxMap[to1D(nnxtPos)])
                        res = true;
                } else {
                    res = true;
                }
                break;
            case WALL:
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

                if (getBlk(nnxtPos) == TARGET)
                    this->filled++;
                if (getBlk(nxtPos) == TARGET)
                    this->filled--;

                this->distScore += distScoreMap[to1D(nnxtPos)] - distScoreMap[to1D(nxtPos)];

                // if (!this->dead)
                for (size_t v = UP; v <= DOWN; v++) {
                    Position leftpos = nnxtPos;
                    Position rightpos = nnxtPos;
                    while (getBlk(leftpos) == EMPTY && this->wallMap[to1D(leftpos + v)]) leftpos = leftpos + LEFT;
                    while (getBlk(rightpos) == EMPTY && this->wallMap[to1D(rightpos + v)]) rightpos = rightpos + RIGHT;
                    if (this->wallMap[to1D(leftpos)] && this->wallMap[to1D(rightpos)])
                        this->dead = true;
                }
                // if (!this->dead)
                for (size_t h = LEFT; h <= RIGHT; h++) {
                    Position uppos = nnxtPos;
                    Position downpos = nnxtPos;
                    while (getBlk(uppos) == EMPTY && this->wallMap[to1D(uppos + h)]) uppos = uppos + UP;
                    while (getBlk(downpos) == EMPTY && this->wallMap[to1D(downpos + h)]) downpos = downpos + DOWN;
                    if (this->wallMap[to1D(uppos)] && this->wallMap[to1D(downpos)])
                        this->dead = true;
                }

                if (this->wallMap[to1D(nnxtPos + UP)] || this->wallMap[to1D(nnxtPos + DOWN)]) {
                    if (this->wallMap[to1D(nnxtPos + LEFT)] || this->wallMap[to1D(nnxtPos + RIGHT)])
                        this->wallMap[to1D(nnxtPos)] = 1;
                }

                for (size_t v = UP; v < LEFT; v++) {
                    for (size_t h = LEFT; h < STOP; h++) {
                        int tot = 0, tg = 0, bx = 0;
                        if (this->wallMap[to1D(nnxtPos)] || this->boxMap[to1D(nnxtPos)])
                            tot++, bx += this->boxMap[to1D(nnxtPos)];
                        if (this->wallMap[to1D(nnxtPos + v)] || this->boxMap[to1D(nnxtPos + v)])
                            tot++, bx += this->boxMap[to1D(nnxtPos + v)];
                        if (this->wallMap[to1D(nnxtPos + h)] || this->boxMap[to1D(nnxtPos + h)])
                            tot++, bx += this->boxMap[to1D(nnxtPos + h)];
                        if (this->wallMap[to1D(nnxtPos + v + h)] || this->boxMap[to1D(nnxtPos + v + h)])
                            tot++, bx += this->boxMap[to1D(nnxtPos + v + h)];
                        if (getBlk(nnxtPos) == TARGET)
                            tg++;
                        if (getBlk(nnxtPos + v) == TARGET)
                            tg++;
                        if (getBlk(nnxtPos + h) == TARGET)
                            tg++;
                        if (getBlk(nnxtPos + v + h) == TARGET)
                            tg++;
                        if (tot == 4 && tg != bx) {
                            this->wallMap[to1D(nnxtPos)] = true;
                            this->wallMap[to1D(nnxtPos + v)] = true;
                            this->wallMap[to1D(nnxtPos + h)] = true;
                            this->wallMap[to1D(nnxtPos + v + h)] = true;
                            this->dead = true;
                        }
                    }
                }
                if (getBlk(nnxtPos) != TARGET && this->wallMap[to1D(nnxtPos)])
                    this->dead = true;
            }
            break;
        case WALL:
        default:
            break;
    }
    this->pos = pos + dir;
    this->moveSequence.push_back(toKey(dir));
}
vector<Sokoban::State> Sokoban::State::nextStates() const {
    vector<State> res;
    queue<pair<Position, string>> que;
    bs256 went;
    que.emplace(pos, "");
    went[to1D(pos)] = 1;

    while (!que.empty()) {
        auto [curPos, curSeq] = que.front();
        que.pop();

        // #pragma omp parallel for schedule(static) num_threads(4)
        for (size_t dir = UP; dir < STOP; dir++) {
            Position nxtPos = curPos + dir;
            string nxtSeq = curSeq + toKey(dir);
            if (went[to1D(nxtPos)])
                continue;
            if (this->boxMap[to1D(nxtPos)]) {
                if (this->canMovePly(curPos, dir)) {
                    State nxtState = *this;
                    nxtState.pos = curPos;
                    nxtState.moveSequence += curSeq;
                    nxtState.movePly(dir);
                    if (!nxtState.isDead())
                        // #pragma omp critical
                        res.emplace_back(nxtState);
                }
            } else if (!this->wallMap[to1D(nxtPos)]) {
                // #pragma omp critical
                que.emplace(nxtPos, nxtSeq);
                // #pragma omp critical
                went[to1D(nxtPos)] = 1;
            }
        }
    }
    return res;
}
bool Sokoban::State::solved() const {
    return this->getFilled() == Targets.size();
}
bool Sokoban::State::isDead() const {
    return this->dead;
    if (this->dead)
        return true;

    int range_box_cnt = 0;
    int range_target_cnt = 0;

    queue<Position> que;
    bs256 went;
    que.emplace(this->pos);
    went[to1D(this->pos)] = 1;

    while (!que.empty()) {
        auto curpos = que.front();
        que.pop();
        if (this->boxMap[to1D(curpos)])
            range_box_cnt++;
        if (getBlk(curpos) == TARGET) {
            // cout << curpos << ".\n";
            range_target_cnt++;
        }
        for (size_t dir = UP; dir <= RIGHT; dir++) {
            if (went[to1D(curpos + dir)])
                continue;
            went[to1D(curpos + dir)] = 1;
            if (!this->wallMap[to1D(curpos + dir)]) {
                que.emplace(curpos + dir);
            }
        }
    }
    // if (range_box_cnt != range_target_cnt) {
    //     cout << range_box_cnt << " " << range_target_cnt << "\n";
    //     this->print();
    //     cout << "\n";
    // }

    return range_box_cnt != range_target_cnt;
}
int const& Sokoban::State::getFilled() const {
    return this->filled;
}
int const& Sokoban::State::getDistScore() const {
    return this->distScore;
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
            if (went[to1D(nxtPos)])
                continue;
            went[to1D(nxtPos)] = 1;
            if (!this->wallMap[to1D(nxtPos)] && !this->boxMap[to1D(nxtPos)])
                que.emplace(nxtPos);
        }
    }
    return went;
}
bs256 const& Sokoban::State::getBoxMap() const {
    return this->boxMap;
}
string const& Sokoban::State::getMoveSequence() const {
    return this->moveSequence;
}
Position const& Sokoban::State::getPos() const {
    return this->pos;
}
void Sokoban::State::print() const {
    cout << "ply:" << this->pos << "\n";
    cout << "solved:" << (this->solved() ? "true" : "false") << "\n";
    // cout << "filled:" << this->filled << "\n";
    // for (int r = 0; r < Rows; r++) {
    //     for (int c = 0; c < Cols; c++)
    //         if (this->wallMap[to1D(r, c)])
    //             cout << '#';
    //         else
    //             cout << ' ';
    //     cout << "\n";
    // }
    for (int r = 0; r < Rows; r++) {
        for (int c = 0; c < Cols; c++)
            if (this->pos.first == r && this->pos.second == c)
                cout << addPly(getBlk(r, c));
            else if (this->boxMap[to1D(r, c)])
                cout << addBox(getBlk(r, c));
            else
                cout << getBlk(r, c);
        cout << "\n";
    }
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
    // Build distance score
    for (int i = 0; i < Rows * Cols; i++) distScoreMap[i] = 1;
    for (int r = 0; r < Rows; r++) {
        for (int c = 0; c < Cols; c++) {
            if (getBlk(r, c) == TARGET) {
                queue<pair<Position, int>> que;
                bs256 went;
                que.emplace(Position(r, c), 0);
                went[to1D(r, c)] = 1;
                while (!que.empty()) {
                    auto [curpos, curdist] = que.front();
                    que.pop();
                    distScoreMap[to1D(curpos)] *= curdist;
                    for (size_t dir = UP; dir < STOP; dir++) {
                        if (went[to1D(curpos + dir)])
                            continue;
                        went[to1D(curpos + dir)] = 1;
                        if (getBlk(curpos + dir) == EMPTY)
                            que.emplace(curpos + dir, curdist + 1);
                    }
                }
            }
        }
    }

    // Create initial state
    initState = State(ply, boxMap);
    return;
}
void Sokoban::bfs() {
    priority_queue<State, vector<State>, StateCmp> statesQue;
    unordered_map<bs256, bs256> statesMap;
    statesQue.emplace(initState);
    statesMap[initState.getBoxMap()] |= initState.getFloodedMap();

    while (!statesQue.empty()) {
        auto curState = statesQue.top();
        statesQue.pop();
        for (auto nxtState : curState.nextStates()) {
            nxtState.print();
            cout << "\n";
            auto it = statesMap.find(nxtState.getBoxMap());
            if (it != statesMap.end() && it->second[to1D(nxtState.getPos())])
                continue;
            statesMap[nxtState.getBoxMap()] |= nxtState.getFloodedMap();

            if (nxtState.solved()) {
                cout << nxtState.getMoveSequence() << "\n";
                return;
            } else {
                statesQue.emplace(nxtState);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2)
        cerr << "Input Error...\n";
    Sokoban sokoban;
    sokoban.getInput(argv[1]);
    sokoban.bfs();
    return 0;
}