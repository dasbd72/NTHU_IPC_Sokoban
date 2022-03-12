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
int Rows, Cols, Targets;
string initMap;

typedef unordered_set<pair<int, int>, boost::hash<pair<int, int>>> Pos_Set;
typedef pair<int, int> Position;
typedef bitset<256> bs256;
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
enum Direction : char {
    UP = 'W',
    DOWN = 'S',
    LEFT = 'A',
    RIGHT = 'D',
    NONEDIR = '\0'
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
Direction operator++(Direction& d);
pair<int, int> operator+(pair<int, int> const& lhs, Direction const& rhs);
pair<int, int> operator-(pair<int, int> const& lhs, pair<int, int> const& rhs);
Direction operator/(pair<int, int> const& rhs, pair<int, int> const& lhs);
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
        string moveSequence;
        int filled;
        bool dead;

       public:
        State() { dead = false; }
        State(Position const& pos, bs256 const& boxMap);
        State(State const& obj);
        ~State() {}
        bool movePly(Direction const& dir);
        vector<State> nextStates() const;
        bool solved() const;
        bool const& isDead() const;
        int const& getFilled() const;
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
            return lhs.getFilled() < rhs.getFilled();
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
Direction operator++(Direction& d) {
    switch (d) {
        case UP:
            d = RIGHT;
            break;
        case RIGHT:
            d = DOWN;
            break;
        case DOWN:
            d = LEFT;
            break;
        case LEFT:
            d = NONEDIR;
            break;
        default:
            d = NONEDIR;
            break;
    }
    return d;
}
pair<int, int> operator+(pair<int, int> const& lhs, Direction const& rhs) {
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
Direction operator/(pair<int, int> const& rhs, pair<int, int> const& lhs) {
    auto delt = rhs - lhs;
    if (delt == make_pair(-1, 0))
        return UP;
    else if (delt == make_pair(1, 0))
        return DOWN;
    else if (delt == make_pair(0, -1))
        return LEFT;
    else if (delt == make_pair(0, 1))
        return RIGHT;
    else
        return NONEDIR;
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

Sokoban::State::State(Position const& pos, bs256 const& boxMap) {
    State();
    this->filled = 0;
    this->pos = pos;
    this->boxMap = boxMap;
    for (int r = 0; r < Rows; r++) {
        for (int c = 0; c < Cols; c++) {
            if (this->boxMap[to1D(r, c)] && getBlk(r, c) == TARGET)
                this->filled++;
        }
    }
}
Sokoban::State::State(State const& obj) {
    State();
    this->boxMap = obj.boxMap;
    this->pos = obj.pos;
    this->moveSequence = obj.moveSequence;
    this->filled = obj.filled;
}
Sokoban::State& Sokoban::State::operator=(State const& rhs) {
    this->boxMap = rhs.boxMap;
    this->pos = rhs.pos;
    this->moveSequence = rhs.moveSequence;
    this->filled = rhs.filled;
    return *this;
}
bool Sokoban::State::movePly(Direction const& dir) {
    Position nxtPos = this->pos + dir;
    bool res = false;
    switch (getBlk(nxtPos)) {
        case EMPTY:
        case TARGET:
        case FRAGILE:
            if (this->boxMap[to1D(nxtPos)]) {
                if (getBlk(nxtPos + dir) != FRAGILE && getBlk(nxtPos + dir) != WALL && !this->boxMap[to1D(nxtPos + dir)]) {
                    this->boxMap[to1D(nxtPos)] = 0;
                    this->boxMap[to1D(nxtPos + dir)] = 1;
                    if (getBlk(nxtPos + dir) != TARGET) {
                        if (getBlk(nxtPos + dir + UP) == WALL || getBlk(nxtPos + dir + DOWN) == WALL) {
                            if (getBlk(nxtPos + dir + LEFT) == WALL || getBlk(nxtPos + dir + RIGHT) == WALL)
                                dead = true;
                        }
                    }
                    res = true;
                    if (getBlk(nxtPos + dir) == TARGET)
                        this->filled++;
                    if (getBlk(nxtPos) == TARGET)
                        this->filled--;
                }
            } else {
                res = true;
            }
            break;
        case WALL:
        default:
            break;
    }
    if (res) {
        this->pos = pos + dir;
        this->moveSequence.push_back((char)dir);
    }
    return res;
}
vector<Sokoban::State> Sokoban::State::nextStates() const {
    vector<State> res;
    queue<pair<Position, string>> que;
    bs256 went;
    que.emplace(pos, "");

    while (!que.empty()) {
        auto [curPos, curSeq] = que.front();
        went[to1D(curPos)] = 1;
        que.pop();
        Position nxtPos;
        string nxtSeq;
        for (Direction dir = UP; dir != NONEDIR; ++dir) {
            nxtPos = curPos + dir;
            nxtSeq = curSeq + (char)dir;
            if (went[to1D(nxtPos)])
                continue;
            if (this->boxMap[to1D(nxtPos)]) {
                State nxtState = *this;
                nxtState.pos = curPos;
                nxtState.moveSequence.append(curSeq);
                if (nxtState.movePly(dir))
                    res.emplace_back(nxtState);
            }
            if (addPly(getBlk(nxtPos)) && !this->boxMap[to1D(nxtPos)])
                que.emplace(nxtPos, nxtSeq);
        }
    }
    return res;
}
bool Sokoban::State::solved() const {
    return this->filled == Targets;
}
bool const& Sokoban::State::isDead() const {
    return this->dead;
}
int const& Sokoban::State::getFilled() const {
    return this->filled;
}
bs256 Sokoban::State::getFloodedMap() const {
    bs256 res;
    queue<Position> que;
    que.emplace(this->pos);
    while (!que.empty()) {
        Position curPos = que.front();
        que.pop();
        res[to1D(curPos)] = 1;
        for (Direction dir = UP; dir != NONEDIR; ++dir) {
            Position nxtPos = curPos + dir;
            if (res[to1D(nxtPos)])
                continue;
            if (addPly(getBlk(nxtPos)) && !this->boxMap[to1D(nxtPos)])
                que.emplace(nxtPos);
        }
    }
    return res;
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
    cout << "filled:" << this->filled << "\n";
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

    Targets = 0;
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
                Targets++;
        }
        initMap.append(input[r]);
    }
    initState = State(ply, boxMap);
    return;
}
void Sokoban::bfs() {
    priority_queue<State, vector<State>, StateCmp> statesQue;
    unordered_map<bs256, bs256> statesMap;
    statesQue.emplace(initState);

    while (!statesQue.empty()) {
        auto curState = statesQue.top();
        auto nextStates = curState.nextStates();
        statesQue.pop();
        statesMap[curState.getBoxMap()] = curState.getFloodedMap();
        // statesMap.emplace(curState.getBoxMap(), curState.getFloodedMap());  // Dont use this cuz it's SLOW
        for (auto nxtState : nextStates) {
            if (nxtState.isDead())
                continue;
            auto it = statesMap.find(nxtState.getBoxMap());
            if (it != statesMap.end() && it->second[to1D(nxtState.getPos())])
                continue;

            if (nxtState.solved()) {
                cout << nxtState.getMoveSequence() << "\n";
                return;
            } else {
                statesQue.emplace(nxtState);
            }
        }
    }
}
size_t to1D(Position const& pos) {
    return pos.first * Cols + pos.second;
}
size_t to1D(int const& r, int const& c) {
    return r * Cols + c;
}

int main(int argc, char* argv[]) {
    if (argc != 2)
        cerr << "Input Error...\n";
    Sokoban sokoban;
    sokoban.getInput(argv[1]);
    sokoban.bfs();
    return 0;
}