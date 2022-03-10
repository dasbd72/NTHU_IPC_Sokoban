#include <omp.h>
#include <pthread.h>

#include <boost/functional/hash.hpp>
#include <fstream>
#include <iostream>
#include <queue>
#include <set>
#include <unordered_set>
#include <utility>
#include <vector>
using namespace std;

// g++ -std=c++17 -O3 -pthread -fopenmp -Wall -Wextra -fsanitize=address -g hw1.cc -o hw1; ./hw1 test.txt
// g++ -std=c++17 -O3 -pthread -fopenmp -Wall -Wextra -g hw1.cc -o hw1; ./hw1 test.txt

// =================Definition================
int Rows, Cols;
int Targets;
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
Direction operator++(Direction& d);
pair<int, int> operator+(pair<int, int> const& lhs, Direction const& rhs);
ostream& operator<<(ostream& os, const pair<int, int>& rhs);
class Sokoban {
   private:
    char static addBox(char c);
    char static rmBox(char c);
    char static addPly(char c);
    char static rmPly(char c);

    struct State {
       private:
        pair<int, int> plyPos;
        string moveSequence;
        int filled;
        string data;

        bool moveBox(pair<int, int> pos, Direction dir);
        void setBlk(pair<int, int> pos, char c);

       public:
        State();
        State(const vector<string>& obj, const pair<int, int>& plyPos);
        State(const State& obj);
        ~State();
        char getBlk(pair<int, int> pos) const;
        char getBlk(int r, int c) const;
        bool movePly(Direction dir);
        bool isdead() const;
        int getFilled() const;
        bool solved() const;
        void print() const;
        string getMoveSequence() const;
        pair<string, pair<int, int>> getData() const;
        State& operator=(const State& rhs);

        friend bool operator<(const State& lhs, const State& rhs);
    };
    vector<string> input;
    State initState;

   public:
    Sokoban();
    ~Sokoban();
    void getInput(char* file_path);
    void test();
    void manual();
    void bfs();
    friend bool operator<(const State& lhs, const State& rhs);
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
ostream& operator<<(ostream& os, const pair<int, int>& rhs) {
    os << "(" << rhs.first << " , " << rhs.second << ")";
    return os;
}

// Sokoban, Private
char Sokoban::addBox(char c) {
    if (c == EMPTY)
        return BOX;
    else if (c == TARGET)
        return BOXT;
    else
        return NONE;
}
char Sokoban::rmBox(char c) {
    if (c == BOX)
        return EMPTY;
    else if (c == BOXT)
        return TARGET;
    else
        return NONE;
}
char Sokoban::addPly(char c) {
    if (c == EMPTY)
        return PLY;
    else if (c == TARGET)
        return PLYT;
    else if (c == FRAGILE)
        return PLYF;
    else
        return NONE;
}
char Sokoban::rmPly(char c) {
    if (c == PLY)
        return EMPTY;
    else if (c == PLYT)
        return TARGET;
    else if (c == PLYF)
        return FRAGILE;
    else
        return NONE;
}
// Sokoban::State, Private
bool Sokoban::State::moveBox(pair<int, int> pos, Direction dir) {
    if (addBox(getBlk(pos + dir))) {
        if (getBlk(pos) == BOXT)
            this->filled--;
        setBlk(pos, rmBox(getBlk(pos)));
        setBlk(pos + dir, addBox(getBlk(pos + dir)));
        if (getBlk(pos + dir) == BOXT)
            this->filled++;
        return true;
    }
    return false;
}
void Sokoban::State::setBlk(pair<int, int> pos, char c) {
    this->data[pos.first * Cols + pos.second] = c;
}
// Sokoban::State, Public
Sokoban::State::State() {
    this->filled = 0;
}
Sokoban::State::State(const vector<string>& obj, const pair<int, int>& plyPos) {
    State();
    this->plyPos = plyPos;
    for (int r = 0; r < Rows; r++) {
        this->data.append(obj[r]);
        for (int c = 0; c < Cols; c++) {
            if (obj[r][c] == BOXT)
                filled++;
        }
    }
}
Sokoban::State::State(const State& obj) {
    State();
    this->filled = obj.filled;
    this->data = obj.data;
    this->plyPos = obj.plyPos;
    this->moveSequence = obj.moveSequence;
}
Sokoban::State::~State() {}
bool Sokoban::State::movePly(Direction dir) {
    bool res = false;
    switch (getBlk(plyPos + dir)) {
        case EMPTY:
        case TARGET:
        case FRAGILE:
            res = true;
            break;
        case BOX:
        case BOXT:
            if (addBox(getBlk(plyPos + dir + dir))) {
                if (getBlk(plyPos + dir) == BOXT)
                    this->filled--;
                setBlk(plyPos + dir, rmBox(getBlk(plyPos + dir)));
                setBlk(plyPos + dir + dir, addBox(getBlk(plyPos + dir + dir)));
                if (getBlk(plyPos + dir + dir) == BOXT)
                    this->filled++;
                res = true;
            }
            break;
        case WALL:
        default:
            break;
    }
    if (res) {
        plyPos = plyPos + dir;
        moveSequence.push_back(dir);
    }
    return res;
}
bool Sokoban::State::isdead() const {
    pair<int, int> pos;
    for (pos.first = 0; pos.first < Rows; pos.first++) {
        for (pos.second = 0; pos.second < Cols; pos.second++) {
            if (this->getBlk(pos) == BOX) {
                if (this->getBlk(pos + UP) == WALL || this->getBlk(pos + DOWN) == WALL) {
                    if (this->getBlk(pos + LEFT) == WALL || this->getBlk(pos + RIGHT) == WALL)
                        return true;
                }
            }
        }
    }
    return false;
}
int Sokoban::State::getFilled() const {
    int res = 0;
    for (int r = 0; r < Rows; r++) {
        for (int c = 0; c < Cols; c++) {
            if (getBlk(r, c) == BOXT)
                res++;
        }
    }
    return res;
}
char Sokoban::State::getBlk(pair<int, int> pos) const {
    return this->data[pos.first * Cols + pos.second];
}
char Sokoban::State::getBlk(int r, int c) const {
    return this->data[r * Cols + c];
}
bool Sokoban::State::solved() const {
    return this->getFilled() == Targets;
}
void Sokoban::State::print() const {
    cout << "ply : " << plyPos << "\n";
    for (int r = 0; r < Rows; r++) {
        for (int c = 0; c < Cols; c++)
            if (plyPos.first == r && plyPos.second == c)
                cout << addPly(this->getBlk(r, c));
            else
                cout << this->getBlk(r, c);
        cout << "\n";
    }
    cout << moveSequence << "\n";
    cout << "solved : " << (this->solved() ? "true" : "false") << "\n";
}
string Sokoban::State::getMoveSequence() const {
    return moveSequence;
}
pair<string, pair<int, int>> Sokoban::State::getData() const {
    return make_pair(this->data, make_pair(this->plyPos.first, this->plyPos.second));
}
Sokoban::State& Sokoban::State::operator=(State const& rhs) {
    this->data = rhs.data;
    this->plyPos = rhs.plyPos;
    this->moveSequence = rhs.moveSequence;
    this->filled = rhs.filled;
    return *this;
}
bool operator<(const Sokoban::State& lhs, const Sokoban::State& rhs) {
    return lhs.data < rhs.data;
}

// Sokoban, Private
Sokoban::Sokoban() {}
Sokoban::~Sokoban() {}
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

    initState = State(input, ply);
    return;
}
void Sokoban::test() {
    State s(initState);
    set<State> sset;
    s = initState;
    sset.emplace(s);
    s.print();

    s.movePly(UP);
    sset.emplace(s);
    s.print();

    // s.movePly(DOWN);
    // sset.emplace(s);
    // s.print();
    // s.movePly(LEFT);
    // sset.emplace(s);
    // s.print();
    // s.movePly(LEFT);
    // sset.emplace(s);
    // s.print();

    // for (auto& ss : sset) {
    //     ss.print();
    // }
}
void Sokoban::manual() {
    initState.print();
    string inp;
    while (cin >> inp) {
        initState.movePly(Direction(inp[0]));
        initState.print();
    }
}
void Sokoban::bfs() {
    queue<State> statesQue;
    unordered_set<pair<string, pair<int, int>>, boost::hash<pair<string, pair<int, int>>>> statesSet;
    statesQue.emplace(initState);

    while (!statesQue.empty()) {
        State curState = statesQue.front();
        statesQue.pop();
        statesSet.emplace(curState.getData());
        for (Direction dir = UP; dir != NONEDIR; ++dir) {
            State nxtState = curState;
            if (!nxtState.movePly(dir))
                continue;

            if (nxtState.isdead() || statesSet.find(nxtState.getData()) != statesSet.end())
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

int main(int argc, char* argv[]) {
    Sokoban sokoban;
    sokoban.getInput(argv[1]);
    sokoban.bfs();
    return 0;
}