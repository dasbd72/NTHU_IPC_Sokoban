#include <omp.h>
#include <pthread.h>

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
struct Position {
    int row;
    int col;
    Position() : row(0), col(0) {}
    Position(int r, int c) : row(r), col(c) {}
    ~Position() {}
    Position operator+(Direction& rhs) {
        if (rhs == UP)
            return Position(row - 1, col);
        else if (rhs == DOWN)
            return Position(row + 1, col);
        else if (rhs == LEFT)
            return Position(row, col - 1);
        else if (rhs == RIGHT)
            return Position(row, col + 1);
        else
            return *this;
    }
    Position go(Direction dir) {
        return *this + dir;
    }
    Position up() {
        return Position(row - 1, col);
    }
    Position down() {
        return Position(row + 1, col);
    }
    Position left() {
        return Position(row, col - 1);
    }
    Position right() {
        return Position(row, col + 1);
    }
};
ostream& operator<<(ostream& os, const Position& rhs);
class Sokoban {
   private:
    bool static isBox(char c);
    bool static isPly(char c);
    char static addBox(char c);
    char static rmBox(char c);
    char static addPly(char c);
    char static rmPly(char c);

    struct State {
       private:
        Position ply;
        string moveSequence;
        int filled;
        string data;

        bool moveBox(Position pos, Direction dir);
        void setBlk(Position pos, char c);

       public:
        State();
        State(const vector<string>& obj);
        State(const State& obj);
        ~State();
        char getBlk(Position pos) const;
        char getBlk(int r, int c) const;
        bool movePly(Direction dir);
        bool isdead() const;
        int getFilled() const;
        bool solved() const;
        void print() const;
        string getMoveSequence() const;
        string getData() const;
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
// Sokoban, Private
bool Sokoban::isBox(char c) {
    return c == BOX || c == BOXT;
}
bool Sokoban::isPly(char c) {
    return c == PLY || c == PLYT || c == PLYF;
}
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
bool Sokoban::State::moveBox(Position pos, Direction dir) {
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
void Sokoban::State::setBlk(Position pos, char c) {
    this->data[pos.row * Cols + pos.col] = c;
}
// Sokoban::State, Public
Sokoban::State::State() {
    this->filled = 0;
}
Sokoban::State::State(const vector<string>& obj) {
    State();
    for (int r = 0; r < Rows; r++) {
        this->data.append(obj[r]);
        for (int c = 0; c < Cols; c++) {
            if (obj[r][c] == PLY || obj[r][c] == PLYT || obj[r][c] == PLYF)
                ply = Position(r, c);
            if (obj[r][c] == BOXT)
                filled++;
        }
    }
}
Sokoban::State::State(const State& obj) {
    State();
    this->filled = obj.filled;
    this->data = obj.data;
    this->ply = obj.ply;
    this->moveSequence = obj.moveSequence;
}
Sokoban::State::~State() {}
bool Sokoban::State::movePly(Direction dir) {
    bool res = false;
    // cout << "movePly, curPly pos : " << ply << "\n";
    switch (getBlk(ply + dir)) {
        case EMPTY:
        case TARGET:
        case FRAGILE:
            setBlk(ply, rmPly(getBlk(ply)));
            setBlk((ply + dir), addPly(getBlk(ply + dir)));
            res = true;
            break;
        case BOX:
        case BOXT:
            // res = moveBox(ply + dir, dir);
            if (addBox(getBlk(ply + dir + dir))) {
                if (getBlk(ply + dir) == BOXT)
                    this->filled--;
                setBlk(ply + dir, rmBox(getBlk(ply + dir)));
                setBlk(ply + dir + dir, addBox(getBlk(ply + dir + dir)));
                if (getBlk(ply + dir + dir) == BOXT)
                    this->filled++;
                setBlk(ply, rmPly(getBlk(ply)));
                setBlk(ply + dir, addPly(getBlk(ply + dir)));
                res = true;
            }
            break;
        case WALL:
        default:
            break;
    }
    if (res) {
        ply = ply + dir;
        moveSequence.push_back(dir);
    }
    return res;
}
bool Sokoban::State::isdead() const {
    Position pos;
    for (pos.row = 0; pos.row < Rows; pos.row++) {
        for (pos.col = 0; pos.col < Cols; pos.col++) {
            if (this->getBlk(pos) == BOX) {
                if (this->getBlk(pos.up()) == WALL || this->getBlk(pos.down()) == WALL) {
                    if (this->getBlk(pos.left()) == WALL || this->getBlk(pos.right()) == WALL)
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
char Sokoban::State::getBlk(Position pos) const {
    return this->data[pos.row * Cols + pos.col];
}
char Sokoban::State::getBlk(int r, int c) const {
    return this->data[r * Cols + c];
}
bool Sokoban::State::solved() const {
    return this->getFilled() == Targets;
}
void Sokoban::State::print() const {
    cout << "ply : " << ply << "\n";
    cout << moveSequence << "\n";
    cout << "solved : " << (this->solved() ? "true" : "false") << "\n";
    for (int r = 0; r < Rows; r++) {
        for (int c = 0; c < Cols; c++)
            cout << this->getBlk(r, c);
        cout << "\n";
    }
}
string Sokoban::State::getMoveSequence() const {
    return moveSequence;
}
string Sokoban::State::getData() const {
    return data;
}
Sokoban::State& Sokoban::State::operator=(State const& rhs) {
    this->data = rhs.data;
    this->ply = rhs.ply;
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

    input_file.open(file_path);
    if (!input_file)
        cerr << "No file found.\n";

    while (getline(input_file, input_line))
        input.emplace_back(input_line);

    Rows = input.size();
    Cols = input[0].size();

    Targets = 0;
    for (int r = 0; r < Rows; r++)
        for (int c = 0; c < Cols; c++)
            if (input[r][c] == TARGET || input[r][c] == BOXT || input[r][c] == PLYT)
                Targets++;

    initState = State(input);
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
    unordered_set<string> statesSet;
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

ostream& operator<<(ostream& os, const Position& rhs) {
    os << "(" << rhs.row << " , " << rhs.col << ")";
    return os;
}

int main(int argc, char* argv[]) {
    Sokoban sokoban;
    sokoban.getInput(argv[1]);
    sokoban.bfs();
    return 0;
}