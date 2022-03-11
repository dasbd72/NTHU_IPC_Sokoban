#include <omp.h>
#include <pthread.h>

#include <boost/functional/hash.hpp>
#include <fstream>
#include <iostream>
#include <queue>
#include <unordered_set>
#include <utility>
#include <vector>
using namespace std;

// g++ -std=c++17 -O3 -pthread -fopenmp -Wall -Wextra -fsanitize=address -g hw1.cc -o hw1; ./hw1 test.txt
// g++ -std=c++17 -O3 -pthread -fopenmp -Wall -Wextra -g hw1.cc -o hw1; ./hw1 test.txt

// =================Definition================
int Rows, Cols, Targets;
string initMap;

typedef unordered_set<pair<int, int>, boost::hash<pair<int, int>>> Pos_Set;
typedef pair<int, int> Position;
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
namespace boost {
template <class K, class C, class A>
std::size_t hash_value(const std::unordered_set<K, C, A>& v) {
    return boost::hash_range(v.begin(), v.end());
}
}  // namespace boost
Direction operator++(Direction& d);
pair<int, int> operator+(pair<int, int> const& lhs, Direction const& rhs);
pair<int, int> operator-(pair<int, int> const& lhs, pair<int, int> const& rhs);
Direction operator/(pair<int, int> const& rhs, pair<int, int> const& lhs);
ostream& operator<<(ostream& os, const pair<int, int>& rhs);

char getBlk(Position pos);
char getBlk(int r, int c);
class Sokoban {
   public:
    class State {
       private:
        Position pos;
        Pos_Set boxes;
        string moveSequence;
        int filled;

       public:
        State() {}
        State(Position& pos, Pos_Set& boxes);
        State(const State& obj);
        ~State() {}
        bool movePly(Direction dir);
        vector<State> nextStates();
        bool isdead() const;
        bool solved() const;
        int getFilled() const;
        Pos_Set getFloodedPos() const;
        Pos_Set getBoxes() const;
        string getMoveSequence() const;
        Position getPos() const;
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

char getBlk(Position pos) {
    return initMap[pos.first * Cols + pos.second];
}
char getBlk(int r, int c) {
    return initMap[r * Cols + c];
}

Sokoban::State::State(Position& pos, Pos_Set& boxes) {
    this->filled = 0;
    this->pos = pos;
    this->boxes = boxes;
    for (auto boxPos : this->boxes) {
        if (getBlk(boxPos) == TARGET)
            this->filled++;
    }
}
Sokoban::State::State(const State& obj) {
    this->boxes = obj.boxes;
    this->pos = obj.pos;
    this->moveSequence = obj.moveSequence;
    this->filled = obj.filled;
}
Sokoban::State& Sokoban::State::operator=(State const& rhs) {
    this->boxes = rhs.boxes;
    this->pos = rhs.pos;
    this->moveSequence = rhs.moveSequence;
    this->filled = rhs.filled;
    return *this;
}
bool Sokoban::State::movePly(Direction dir) {
    Position nxtPos = this->pos + dir;
    bool res = false;
    switch (getBlk(nxtPos)) {
        case EMPTY:
        case TARGET:
        case FRAGILE:
            if (this->boxes.find(nxtPos) != this->boxes.end()) {
                if (getBlk(nxtPos + dir) != FRAGILE && getBlk(nxtPos + dir) != WALL && this->boxes.find(nxtPos + dir) == this->boxes.end()) {
                    this->boxes.erase(nxtPos);
                    this->boxes.emplace(nxtPos + dir);
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
vector<Sokoban::State> Sokoban::State::nextStates() {
    vector<State> res;
    queue<pair<Position, string>> que;
    Pos_Set posSet;
    State nxtState;
    que.emplace(pos, "");
    while (!que.empty()) {
        auto [curPos, curSeq] = que.front();
        que.pop();
        posSet.emplace(curPos);
        Position nxtPos;
        string nxtSeq;
        for (Direction dir = UP; dir != NONEDIR; ++dir) {
            nxtPos = curPos + dir;
            nxtSeq = curSeq + (char)dir;
            if (posSet.find(nxtPos) != posSet.end())
                continue;
            if (this->boxes.find(nxtPos) != this->boxes.end()) {
                nxtState = *this;
                nxtState.pos = curPos;
                nxtState.moveSequence.append(curSeq);

                if (nxtState.movePly(dir)) {
                    res.emplace_back(nxtState);
                }
            }
            if (addPly(getBlk(nxtPos)) && this->boxes.find(nxtPos) == this->boxes.end())
                que.emplace(nxtPos, nxtSeq);
        }
    }
    return res;
}
bool Sokoban::State::isdead() const {
    for (auto boxPos : this->boxes) {
        if (getBlk(boxPos) == TARGET)
            continue;
        if (getBlk(boxPos + UP) == WALL || getBlk(boxPos + DOWN) == WALL) {
            if (getBlk(boxPos + LEFT) == WALL || getBlk(boxPos + RIGHT) == WALL)
                return true;
        }
    }
    return false;
}
bool Sokoban::State::solved() const {
    return this->getFilled() == Targets;
}
int Sokoban::State::getFilled() const {
    return this->filled;
}
Pos_Set Sokoban::State::getFloodedPos() const {
    Pos_Set res;
    queue<Position> que;
    que.emplace(this->pos);
    while (!que.empty()) {
        Position curPos = que.front();
        que.pop();
        res.emplace(curPos);
        for (Direction dir = UP; dir != NONEDIR; ++dir) {
            Position nxtPos = curPos + dir;
            if (res.find(nxtPos) != res.end())
                continue;
            if (addPly(getBlk(nxtPos)) && this->boxes.find(nxtPos) == this->boxes.end())
                que.emplace(nxtPos);
        }
    }
    return res;
}
Pos_Set Sokoban::State::getBoxes() const {
    return this->boxes;
}
string Sokoban::State::getMoveSequence() const {
    return this->moveSequence;
}
Position Sokoban::State::getPos() const {
    return this->pos;
}
void Sokoban::State::print() const {
    cout << "ply:" << pos << "\n";
    cout << "solved:" << (this->solved() ? "true" : "false") << "\n";
    cout << "filled:" << this->filled << "\n";
    for (auto boxPos : this->boxes) cout << boxPos << " ";
    cout << "\n";
    for (int r = 0; r < Rows; r++) {
        for (int c = 0; c < Cols; c++)
            if (pos.first == r && pos.second == c)
                cout << addPly(getBlk(r, c));
            else if (boxes.find(make_pair(r, c)) != boxes.end())
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
    Pos_Set boxes;

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
                boxes.emplace(r, c);
                input[r][c] = rmBox(input[r][c]);
            }
            if (input[r][c] == TARGET)
                Targets++;
        }
        initMap.append(input[r]);
    }
    initState = State(ply, boxes);
    return;
}
void Sokoban::bfs() {
    priority_queue<State, vector<State>, StateCmp> statesQue;
    unordered_map<Pos_Set, Pos_Set, boost::hash<Pos_Set>> statesMap;
    statesQue.emplace(initState);

    while (!statesQue.empty()) {
        auto curState = statesQue.top();
        statesQue.pop();
        statesMap.emplace(curState.getBoxes(), curState.getFloodedPos());
        auto nextStates = curState.nextStates();
        for (auto nxtState : nextStates) {
            if (nxtState.isdead())
                continue;
            if (statesMap.find(nxtState.getBoxes()) != statesMap.end() && statesMap[nxtState.getBoxes()].find(nxtState.getPos()) != statesMap[nxtState.getBoxes()].end()) {
                continue;
            }
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
    Sokoban sokoban;
    sokoban.getInput(argv[1]);
    sokoban.bfs();
    return 0;
}