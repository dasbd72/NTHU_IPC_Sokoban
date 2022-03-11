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

char getBlk(const string& data, pair<int, int> pos);
char getBlk(const string& data, int r, int c);
void setBlk(string& data, pair<int, int> pos, char c);
class Sokoban {
   public:
    class State {
       private:
        pair<int, int> pos;
        string data;
        string moveSequence;
        int filled;

       public:
        State() {}
        State(vector<string>& obj, pair<int, int>& pos);
        State(const State& obj);
        ~State() {}
        bool movePly(Direction dir);
        vector<State> nextStates();
        bool isdead() const;
        bool solved() const;
        int getFilled() const;
        string getFlooded() const;
        set<pair<int, int>> getFloodedPos() const;
        string getData() const;
        string getMoveSequence() const;
        pair<int, int> getPos() const;
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

char getBlk(const string& data, pair<int, int> pos) {
    return data[pos.first * Cols + pos.second];
}
char getBlk(const string& data, int r, int c) {
    return data[r * Cols + c];
}
void setBlk(string& data, pair<int, int> pos, char c) {
    data[pos.first * Cols + pos.second] = c;
}

Sokoban::State::State(vector<string>& obj, pair<int, int>& pos) {
    this->filled = 0;
    this->pos = pos;
    for (int r = 0; r < Rows; r++) {
        this->data.append(obj[r]);
        for (int c = 0; c < Cols; c++) {
            if (obj[r][c] == BOXT)
                this->filled++;
        }
    }
}
Sokoban::State::State(const State& obj) {
    this->data = obj.data;
    this->pos = obj.pos;
    this->moveSequence = obj.moveSequence;
    this->filled = obj.filled;
}
bool Sokoban::State::movePly(Direction dir) {
    bool res = false;
    switch (getBlk(this->data, this->pos + dir)) {
        case EMPTY:
        case TARGET:
        case FRAGILE:
            res = true;
            break;
        case BOX:
        case BOXT:
            if (addBox(getBlk(this->data, this->pos + dir + dir))) {
                if (getBlk(this->data, this->pos + dir) == BOXT)
                    this->filled--;
                setBlk(this->data, this->pos + dir, rmBox(getBlk(this->data, this->pos + dir)));
                setBlk(this->data, this->pos + dir + dir, addBox(getBlk(this->data, this->pos + dir + dir)));
                if (getBlk(this->data, this->pos + dir + dir) == BOXT)
                    this->filled++;
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
    queue<pair<pair<int, int>, string>> que;
    unordered_set<pair<int, int>, boost::hash<pair<int, int>>> posSet;
    State nxtState;
    que.emplace(pos, "");
    while (!que.empty()) {
        auto [curPos, curSeq] = que.front();
        que.pop();
        posSet.emplace(curPos);
        pair<int, int> nxtPos;
        string nxtSeq;
        for (Direction dir = UP; dir != NONEDIR; ++dir) {
            nxtPos = curPos + dir;
            nxtSeq = curSeq + (char)dir;
            if (posSet.find(nxtPos) != posSet.end())
                continue;
            if (getBlk(this->data, nxtPos) == BOX || getBlk(this->data, nxtPos) == BOXT) {
                nxtState = *this;
                nxtState.pos = curPos;
                nxtState.moveSequence.append(curSeq);
                if (nxtState.movePly(dir))
                    res.emplace_back(nxtState);
            }
            if (addPly(getBlk(this->data, nxtPos)))
                que.emplace(nxtPos, nxtSeq);
        }
    }
    return res;
}
bool Sokoban::State::isdead() const {
    pair<int, int> pos;
    for (pos.first = 0; pos.first < Rows; pos.first++) {
        for (pos.second = 0; pos.second < Cols; pos.second++) {
            if (getBlk(this->data, pos) == BOX) {
                if (getBlk(this->data, pos + UP) == WALL || getBlk(this->data, pos + DOWN) == WALL) {
                    if (getBlk(this->data, pos + LEFT) == WALL || getBlk(this->data, pos + RIGHT) == WALL)
                        return true;
                }
            }
        }
    }
    return false;
}
bool Sokoban::State::solved() const {
    return this->getFilled() == Targets;
}
int Sokoban::State::getFilled() const {
    return this->filled;
    // int res = 0;
    // for (int r = 0; r < Rows; r++) {
    //     for (int c = 0; c < Cols; c++) {
    //         if (getBlk(this->data, r, c) == BOXT)
    //             res++;
    //     }
    // }
    // return res;
}
string Sokoban::State::getFlooded() const {
    string res(this->data.size(), 0);
    queue<pair<int, int>> que;
    que.emplace(this->pos);
    while (!que.empty()) {
        pair<int, int> curPos = que.front();
        que.pop();
        setBlk(res, curPos, 'o');
        // cout << curPos << " ? \n";
        for (Direction dir = UP; dir != NONEDIR; ++dir) {
            pair<int, int> nxtPos = curPos + dir;
            if (getBlk(res, nxtPos))
                continue;
            if (addPly(getBlk(this->data, nxtPos)))
                que.emplace(nxtPos);
        }
    }
    return res;
}
set<pair<int, int>> Sokoban::State::getFloodedPos() const {
    set<pair<int, int>> res;
    queue<pair<int, int>> que;
    que.emplace(this->pos);
    while (!que.empty()) {
        pair<int, int> curPos = que.front();
        que.pop();
        res.emplace(curPos);
        for (Direction dir = UP; dir != NONEDIR; ++dir) {
            pair<int, int> nxtPos = curPos + dir;
            if (res.find(nxtPos) != res.end())
                continue;
            if (addPly(getBlk(this->data, nxtPos)))
                que.emplace(nxtPos);
        }
    }
    return res;
}

string Sokoban::State::getData() const {
    return this->data;
}
string Sokoban::State::getMoveSequence() const {
    return this->moveSequence;
}
pair<int, int> Sokoban::State::getPos() const {
    return this->pos;
}
void Sokoban::State::print() const {
    cout << "ply:" << pos << "\n";
    cout << "solved:" << (this->solved() ? "true" : "false") << "\n";
    for (int r = 0; r < Rows; r++) {
        for (int c = 0; c < Cols; c++)
            if (pos.first == r && pos.second == c)
                cout << addPly(getBlk(this->data, r, c));
            else
                cout << getBlk(this->data, r, c);
        cout << "\n";
    }
}
Sokoban::State& Sokoban::State::operator=(State const& rhs) {
    this->data = rhs.data;
    this->pos = rhs.pos;
    this->moveSequence = rhs.moveSequence;
    this->filled = rhs.filled;
    return *this;
}

void Sokoban::getInput(char* file_path) {
    ifstream input_file;
    string input_line;
    pair<int, int> ply;

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
            if (input[r][c] == TARGET || input[r][c] == BOXT)
                Targets++;
        }
    }
    // cout << "[DEBUG]Targets:" << Targets << "\n";

    initState = State(input, ply);
    return;
}
void Sokoban::bfs() {
    priority_queue<State, vector<State>, StateCmp> statesQue;
    map<string, set<pair<int, int>>> statesMap;
    statesQue.emplace(initState);

    // initState.print();

    while (!statesQue.empty()) {
        auto curState = statesQue.top();
        statesQue.pop();
        statesMap[curState.getData()] = curState.getFloodedPos();
        auto nextStates = curState.nextStates();
        // cout << "[DEBUG] After nextStates\n";
        for (auto nxtState : nextStates) {
            if (nxtState.isdead())
                continue;
            if (statesMap.find(nxtState.getData()) != statesMap.end() && statesMap[nxtState.getData()].find(nxtState.getPos()) != statesMap[nxtState.getData()].end())
                continue;

            if (nxtState.solved()) {
                // cout << "[DEBUG] Solving...\n";
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