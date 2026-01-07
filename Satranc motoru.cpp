#include <iostream>
#include <string>
#include <array>
#include <vector>
#include <queue>
#include <map>
#include <cmath>
#include <iomanip>
#include <unordered_map>
#include <cstring>
#include <climits>
#include <sstream>
#include <fstream>
#include <chrono>
#include <thread>
#include <atomic>
#include <random>

using namespace std;

enum rock_rights
{
    K = 1 << 0,
    Q = 1 << 1,
    k = 1 << 2,
    q = 1 << 3
};

struct square
{
    int line, column;

    bool operator<(const square& other) const
    {
        if (line != other.line) return line < other.line;
        else return column < other.column;
    }

    bool operator!=(const square& other) const
    {
        return line != other.line or column != other.column;
    }

    bool operator==(const square& other) const
    {
        return line == other.line and column == other.column;
    }
};

struct Move
{
    square from, to;
    char pawn_promotion = '-';

    bool operator<(const Move& other) const
    {
        if (from != other.from) return from < other.from;
        if (to != other.to) return to < other.to;
        return pawn_promotion < other.pawn_promotion;
    }

    bool operator==(const Move& other) const
    {
        return from == other.from and to == other.to and pawn_promotion == other.pawn_promotion;
    }

    bool operator!=(const Move& other) const
    {
        return from != other.from or to != other.to;
    }
};

struct Book_Move
{
    Move move;
    int weight;
};

struct timer
{
    chrono::steady_clock::time_point start;

    void reset()
    {
        start = chrono::steady_clock::now();
    }

    float elapsed_ms()
    {
        return (float)chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start).count();
    }
};

const int pawn_table[8][8] =
{
    0,   0,   0,   0,   0,   0,   0,   0,
   10,   5,   5, -10, -10,   5,   5,  10,
    5,   0,   0,  10,  10,   0,   0,   5,
    0,   0,   0,  20,  20,   0,   0,   0,
    0,   5,  10,  20,  20,  10,   5,   0,
   15,  20,  30,  30,  30,  30,  20,  15,
   50,  50,  50,  50,  50,  50,  50,  50,
    0,   0,   0,   0,   0,   0,   0,   0
};
const int knight_table[8][8] =
{
  -30, -20, -10, -10, -10, -10, -20, -30,
  -20, -10,   0,   5,   5,   0, -10, -20,
  -10,   0,  10,   5,   5,  10,   0, -10,
  -10,   0,  15,  20,  20,  15,   0, -10,
  -10,   0,  15,  20,  20,  15,   0, -10,
  -10,   0,  10,  15,  15,  10,   0, -10,
  -20, -10,   0,   0,   0,   0, -10, -20,
  -30, -20, -10, -10, -10, -10, -20, -30
};
const int bishop_table[8][8] =
{
  -20, -10, -10, -10, -10, -10, -10, -20,
  -10,  10,   0,   5,   5,   0,  10, -10,
  -10,   0,   5,  10,  10,   5,   0, -10,
  -10,   5,  15,  20,  20,  15,   5, -10,
  -10,   5,  10,  20,  20,  10,   5, -10,
  -10,  10,  10,  10,  10,  10,  10, -10,
  -10,   0,   0,   0,   0,   0,   0, -10,
  -20, -10, -10, -10, -10, -10, -10, -20
};
const int rook_table[8][8] =
{
    0,   0,   5,  10,  10,   5,   0,   0,
   -5,   0,   0,   0,   0,   0,   0,  -5,
   -5,   0,   0,   0,   0,   0,   0,  -5,
   -5,   0,   0,   0,   0,   0,   0,  -5,
   -5,   0,   0,   0,   0,   0,   0,  -5,
   -5,   0,   0,   0,   0,   0,   0,  -5,
    5,  10,  10,  10,  10,  10,  10,   5,
    0,   0,   0,   0,   0,   0,   0,   0
};
const int queen_table[8][8] =
{
  -20, -10,   5,  10,  10,   5, -10, -20,
  -10, -10,   0,   5,   5,   0, -10, -10,
  -10,   0,   0,   5,   5,   0,   0, -10,
   -5,   0,  10,  20,  20,  10,   0,  -5,
   -5,   0,  10,  20,  20,  10,   0,  -5,
  -10,   0,   5,  10,  10,   5,   0, -10,
  -10,   0,   0,   0,   0,   0,   0, -10,
  -20, -10,  -5,   0,   0,  -5, -10, -20
};
const int king_table[8][8] =
{
   20,  30,  20, -10,   0,  10,  30,  20,
   20,  20,   0, -20, -20,   0,  20,  20,
  -30, -30, -30, -30, -30, -30, -30, -30,
  -50, -50, -50, -50, -50, -50, -50, -50,
  -50, -50, -50, -50, -50, -50, -50, -50,
  -50, -50, -50, -50, -50, -50, -50, -50,
  -50, -50, -50, -50, -50, -50, -50, -50,
  -50, -50, -50, -50, -50, -50, -50, -50
};

array<array<char, 8>, 8> global_board;
bool global_whites_turn;
int played_moves = 0;
int global_rr;
square global_en_passant_sq;
unordered_map<string, int> position_count;
unordered_map<string, vector<Book_Move>> opening_book;
atomic<bool> stop_search;
bool book_finished = false;
Move nullmove = { {-1, -1}, {-1, -1} };
thread search_thread;
timer passed_time;
float visited_node_count = 0;
int in_depth;
int INF = INT_MAX;
float limit_time = (float)INF;
int checkmate = INF - 1;
int draw = 10;

bool is_same_team(char piece1, char piece2)
{
    if (piece1 == '#' or piece2 == '#') return false;
    return isupper(piece1) == isupper(piece2);
}

square notation_to_square(const string& notation)
{
    return { notation[1] - '1', notation[0] - 'a' };
}

string square_to_notation(square& sq)
{
    return string{ static_cast<char>(sq.column + 'a'), static_cast<char>(sq.line + '1') };
}

Move notation_to_move(string& notation)
{
    if (notation.size() == 4) return { notation_to_square(notation.substr(0, 2)), notation_to_square(notation.substr(2, 2)) };
    return { notation_to_square(notation.substr(0, 2)), notation_to_square(notation.substr(2, 2)), notation[4] };
}

string move_to_notation(Move& move)
{
    if (move.pawn_promotion == '-') return square_to_notation(move.from) + square_to_notation(move.to);
    return square_to_notation(move.from) + square_to_notation(move.to) + move.pawn_promotion;
}

string create_FEN(array<array<char, 8>, 8>& board, bool whites_turn, int rr, square& en_passant_sq)
{
    string FEN = "";

    for (int line = 7; line > -1; line--)
    {
        char empty_count = '0';

        for (int column = 0; column < 8; column++)
        {
            if (board[line][column] == '#')
                empty_count++;
            else
            {
                if (empty_count != '0')
                {
                    FEN.push_back(empty_count);
                    empty_count = '0';
                }

                FEN.push_back(board[line][column]);
            }
        }

        if (empty_count != '0')
            FEN.push_back(empty_count);

        FEN.push_back('/');
    }
    FEN.pop_back();

    FEN.push_back(' ');
    FEN += whites_turn ? "w" : "b";

    FEN.push_back(' ');
    string castling;
    if (rr & K)
        castling.push_back('K');
    if (rr & Q)
        castling.push_back('Q');
    if (rr & k)
        castling.push_back('k');
    if (rr & q)
        castling.push_back('q');

    if (castling.size() == 0)
        FEN.push_back('-');
    else FEN += castling;

    FEN.push_back(' ');
    if (en_passant_sq != square{ -1, -1 })
        FEN += square_to_notation(en_passant_sq);
    else FEN.push_back('-');

    return FEN;
}

void load_book()
{
    ifstream file("Book.txt");

    string line;
    while (getline(file, line))
    {
        istringstream take(line);

        if (line.empty()) continue;
        if (line[0] == '#') continue;

        string fen, move;
        int weight;

        string p1, p2, p3, p4;
        take >> p1 >> p2 >> p3 >> p4;
        fen = p1 + " " + p2 + " " + p3 + " " + p4;

        take >> move;
        take >> weight;

        opening_book[fen].push_back({ notation_to_move(move), weight });
    }
}

void calculate_time(float time, float inc, array<array<char, 8>, 8>& board)
{
    float phase_factor = 0, security_factor = 0.7f, importance_factor, estimated_remaining_moves;

    for (int line = 0; line < 8; line++)
    {
        for (int column = 0; column < 8; column++)
        {
            if (board[line][column] == 'n' or board[line][column] == 'N' or board[line][column] == 'b' or board[line][column] == 'B')
                phase_factor += 1.f / 24.f;
            if (board[line][column] == 'r' or board[line][column] == 'R')
                phase_factor += 2.f / 24.f;
            if (board[line][column] == 'q' or board[line][column] == 'Q')
                phase_factor += 4.f / 24.f;
        }
    }

    if (phase_factor >= 0.7f)
        importance_factor = 0.6f, estimated_remaining_moves = 45;
    else if (phase_factor < 0.7f and phase_factor >= 0.4f)
        importance_factor = 1.3f, estimated_remaining_moves = 25;
    else
        importance_factor = 1.f, estimated_remaining_moves = 10;

    limit_time = (time + estimated_remaining_moves * inc) * security_factor * importance_factor / estimated_remaining_moves;
}

void make_move(Move& move, array<array<char, 8>, 8>& board)
{
    if (board[move.from.line][move.from.column] == 'K' and move.from.line == 0
        and move.from.column == 4 and move.to.line == 0 and move.to.column % 4 == 2)
    {
        if (move.to.column == 6)
        {
            board[0][7] = '#';
            board[0][6] = 'K';
            board[0][5] = 'R';
            board[0][4] = '#';
        }
        else
        {
            board[0][0] = '#';
            board[0][2] = 'K';
            board[0][3] = 'R';
            board[0][4] = '#';
        }
        return;
    }

    else if (board[move.from.line][move.from.column] == 'k' and move.from.line == 7
        and move.from.column == 4 and move.to.line == 7 and move.to.column % 4 == 2)
    {
        if (move.to.column == 6)
        {
            board[7][7] = '#';
            board[7][6] = 'k';
            board[7][5] = 'r';
            board[7][4] = '#';
        }
        else
        {
            board[7][0] = '#';
            board[7][2] = 'k';
            board[7][3] = 'r';
            board[7][4] = '#';
        }
        return;
    }

    if (board[move.to.line][move.to.column] == '#' and (board[move.from.line][move.from.column] == 'P'
        or board[move.from.line][move.from.column] == 'p') and
        (move.to.column == move.from.column + 1 or move.to.column == move.from.column - 1))
    {
        if (board[move.from.line][move.from.column] == 'P')
        {
            board[move.to.line - 1][move.to.column] = '#';
        }
        else
        {
            board[move.to.line + 1][move.to.column] = '#';
        }
    }

    if (board[move.from.line][move.from.column] == 'P' and move.to.line == 7)
    {
        if (move.pawn_promotion == 'q')
        {
            board[move.to.line][move.to.column] = 'Q';
        }
        else if (move.pawn_promotion == 'r')
        {
            board[move.to.line][move.to.column] = 'R';
        }
        else if (move.pawn_promotion == 'n')
        {
            board[move.to.line][move.to.column] = 'N';
        }
        else
        {
            board[move.to.line][move.to.column] = 'B';
        }
        board[move.from.line][move.from.column] = '#';
        return;
    }

    if (board[move.from.line][move.from.column] == 'p' and move.to.line == 0)
    {
        if (move.pawn_promotion == 'q')
        {
            board[move.to.line][move.to.column] = 'q';
        }
        else if (move.pawn_promotion == 'r')
        {
            board[move.to.line][move.to.column] = 'r';
        }
        else if (move.pawn_promotion == 'n')
        {
            board[move.to.line][move.to.column] = 'n';
        }
        else
        {
            board[move.to.line][move.to.column] = 'b';
        }
        board[move.from.line][move.from.column] = '#';
        return;
    }

    board[move.to.line][move.to.column] = board[move.from.line][move.from.column];
    board[move.from.line][move.from.column] = '#';
}

void debug_board(array<array<char, 8>, 8>& board, Move& move)
{
    make_move(move, board);
    cerr << "DEBUG Board (whites_turn=" << global_whites_turn << "):" << endl;
    for (int i = 7; i >= 0; i--) {
        cerr << i + 1 << " ";
        for (int j = 0; j < 8; j++) {
            if (board[i][j] == '#') cerr << ". ";
            else cerr << board[i][j] << " ";
        }
        cerr << endl;
    }
    cerr << "  a b c d e f g h" << endl;
}

bool is_attacked_by_white(const square& sq, array<array<char, 8>, 8>& board)
{
    int line = sq.line, column = sq.column;

    if (line > 0 and ((column < 7 and board[line - 1][column + 1] == 'P') or
        (column > 0 and board[line - 1][column - 1] == 'P')))
    {
        return true;
    }

    if (line < 7 and column < 6 and board[line + 1][column + 2] == 'N')
        return true;

    if (line > 0 and column < 6 and board[line - 1][column + 2] == 'N')
        return true;

    if (line < 6 and column < 7 and board[line + 2][column + 1] == 'N')
        return true;

    if (line < 6 and column > 0 and board[line + 2][column - 1] == 'N')
        return true;

    if (line < 7 and column > 1 and board[line + 1][column - 2] == 'N')
        return true;

    if (line > 0 and column > 1 and board[line - 1][column - 2] == 'N')
        return true;

    if (line > 1 and column < 7 and board[line - 2][column + 1] == 'N')
        return true;

    if (line > 1 and column > 0 and board[line - 2][column - 1] == 'N')
        return true;

    while (true)
    {
        line++;
        column++;
        if (line > 7 or column > 7)
            break;

        if (board[line][column] == 'B' or board[line][column] == 'Q')
            return true;

        if (board[line][column] != '#')
            break;
    }

    line = sq.line, column = sq.column;
    while (true)
    {
        line++;
        column--;
        if (line > 7 or column < 0)
            break;

        if (board[line][column] == 'B' or board[line][column] == 'Q')
            return true;

        if (board[line][column] != '#')
            break;
    }

    line = sq.line, column = sq.column;
    while (true)
    {
        line--;
        column++;
        if (line < 0 or column > 7)
            break;

        if (board[line][column] == 'B' or board[line][column] == 'Q')
            return true;

        if (board[line][column] != '#')
            break;
    }

    line = sq.line, column = sq.column;
    while (true)
    {
        line--;
        column--;
        if (line < 0 or column < 0)
            break;

        if (board[line][column] == 'B' or board[line][column] == 'Q')
            return true;

        if (board[line][column] != '#')
            break;
    }

    line = sq.line, column = sq.column;
    while (true)
    {
        column++;
        if (column > 7)
            break;

        if (board[line][column] == 'R' or board[line][column] == 'Q')
            return true;

        if (board[line][column] != '#')
            break;
    }

    column = sq.column;
    while (true)
    {
        column--;
        if (column < 0)
            break;

        if (board[line][column] == 'R' or board[line][column] == 'Q')
            return true;

        if (board[line][column] != '#')
            break;
    }

    column = sq.column;
    while (true)
    {
        line++;
        if (line > 7)
            break;

        if (board[line][column] == 'R' or board[line][column] == 'Q')
            return true;

        if (board[line][column] != '#')
            break;
    }

    line = sq.line;
    while (true)
    {
        line--;
        if (line < 0)
            break;

        if (board[line][column] == 'R' or board[line][column] == 'Q')
            return true;

        if (board[line][column] != '#')
            break;
    }

    line = sq.line, column = sq.column;
    if (column < 7 and board[line][column + 1] == 'K')
        return true;

    if (column > 0 and board[line][column - 1] == 'K')
        return true;

    if (line < 7 and board[line + 1][column] == 'K')
        return true;

    if (line > 0 and board[line - 1][column] == 'K')
        return true;

    if (line < 7 and column < 7 and board[line + 1][column + 1] == 'K')
        return true;

    if (line < 7 and column > 0 and board[line + 1][column - 1] == 'K')
        return true;

    if (line > 0 and column < 7 and board[line - 1][column + 1] == 'K')
        return true;

    if (line > 0 and column > 0 and board[line - 1][column - 1] == 'K')
        return true;

    return false;
}

bool is_attacked_by_black(const square& sq, array<array<char, 8>, 8>& board)
{
    int line = sq.line, column = sq.column;

    if (line < 7 and ((column < 7 and board[line + 1][column + 1] == 'p') or
        (column > 0 and board[line + 1][column - 1] == 'p')))
    {
        return true;
    }

    if (line < 7 and column < 6 and board[line + 1][column + 2] == 'n')
        return true;

    if (line > 0 and column < 6 and board[line - 1][column + 2] == 'n')
        return true;

    if (line < 6 and column < 7 and board[line + 2][column + 1] == 'n')
        return true;

    if (line < 6 and column > 0 and board[line + 2][column - 1] == 'n')
        return true;

    if (line < 7 and column > 1 and board[line + 1][column - 2] == 'n')
        return true;

    if (line > 0 and column > 1 and board[line - 1][column - 2] == 'n')
        return true;

    if (line > 1 and column < 7 and board[line - 2][column + 1] == 'n')
        return true;

    if (line > 1 and column > 0 and board[line - 2][column - 1] == 'n')
        return true;

    while (true)
    {
        line++;
        column++;
        if (line > 7 or column > 7)
            break;

        if (board[line][column] == 'b' or board[line][column] == 'q')
            return true;

        if (board[line][column] != '#')
            break;
    }

    line = sq.line, column = sq.column;
    while (true)
    {
        line++;
        column--;
        if (line > 7 or column < 0)
            break;

        if (board[line][column] == 'b' or board[line][column] == 'q')
            return true;

        if (board[line][column] != '#')
            break;
    }

    line = sq.line, column = sq.column;
    while (true)
    {
        line--;
        column++;
        if (line < 0 or column > 7)
            break;

        if (board[line][column] == 'b' or board[line][column] == 'q')
            return true;

        if (board[line][column] != '#')
            break;
    }

    line = sq.line, column = sq.column;
    while (true)
    {
        line--;
        column--;
        if (line < 0 or column < 0)
            break;

        if (board[line][column] == 'b' or board[line][column] == 'q')
            return true;

        if (board[line][column] != '#')
            break;
    }

    line = sq.line, column = sq.column;
    while (true)
    {
        column++;
        if (column > 7)
            break;

        if (board[line][column] == 'r' or board[line][column] == 'q')
            return true;

        if (board[line][column] != '#')
            break;
    }

    column = sq.column;
    while (true)
    {
        column--;
        if (column < 0)
            break;

        if (board[line][column] == 'r' or board[line][column] == 'q')
            return true;

        if (board[line][column] != '#')
            break;
    }

    column = sq.column;
    while (true)
    {
        line++;
        if (line > 7)
            break;

        if (board[line][column] == 'r' or board[line][column] == 'q')
            return true;

        if (board[line][column] != '#')
            break;
    }

    line = sq.line;
    while (true)
    {
        line--;
        if (line < 0)
            break;

        if (board[line][column] == 'r' or board[line][column] == 'q')
            return true;

        if (board[line][column] != '#')
            break;
    }

    line = sq.line, column = sq.column;
    if (column < 7 and board[line][column + 1] == 'k')
        return true;

    if (column > 0 and board[line][column - 1] == 'k')
        return true;

    if (line < 7 and board[line + 1][column] == 'k')
        return true;

    if (line > 0 and board[line - 1][column] == 'k')
        return true;

    if (line < 7 and column < 7 and board[line + 1][column + 1] == 'k')
        return true;

    if (line < 7 and column > 0 and board[line + 1][column - 1] == 'k')
        return true;

    if (line > 0 and column < 7 and board[line - 1][column + 1] == 'k')
        return true;

    if (line > 0 and column > 0 and board[line - 1][column - 1] == 'k')
        return true;

    return false;
}

bool is_illegal(Move& move, array<array<char, 8>, 8> board, bool whites_turn)
{
    make_move(move, board);

    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            if (whites_turn and board[i][j] == 'K')
                return is_attacked_by_black({ i, j }, board);
            if (not whites_turn and board[i][j] == 'k')
                return is_attacked_by_white({ i, j }, board);
        }
    }
}

void fix_rock_rights(array<array<char, 8>, 8>& board, Move& move, int& rr)
{
    if (board[move.from.line][move.from.column] == 'K')
        rr &= ~(K | Q);
    if (board[move.from.line][move.from.column] == 'k')
        rr &= ~(k | q);
    if (board[move.from.line][move.from.column] == 'R' or
        board[move.to.line][move.to.column] == 'R')
    {
        if (move.from.line == 0 and move.from.column == 0)
            rr &= ~Q;
        if (move.from.line == 0 and move.from.column == 7)
            rr &= ~K;
        if (move.to.line == 0 and move.to.column == 0)
            rr &= ~Q;
        if (move.to.line == 0 and move.to.column == 7)
            rr &= ~K;
    }
    if (board[move.from.line][move.from.column] == 'r' or
        board[move.to.line][move.to.column] == 'r')
    {
        if (move.from.line == 7 and move.from.column == 0)
            rr &= ~q;
        if (move.from.line == 7 and move.from.column == 7)
            rr &= ~k;
        if (move.to.line == 7 and move.to.column == 0)
            rr &= ~q;
        if (move.to.line == 7 and move.to.column == 7)
            rr &= ~k;
    }
}

void fix_en_passant_sq(array<array<char, 8>, 8>& board, Move& move, square& en_passant_sq)
{
    if (board[move.from.line][move.from.column] == 'P' and move.from.line == 1 and move.to.line == 3
        and (move.to.column < 7 and board[move.to.line][move.to.column + 1] == 'p' or
            move.to.column > 0 and board[move.to.line][move.to.column - 1] == 'p'))
    {
        en_passant_sq = { 2, move.from.column };
    }
    else if (board[move.from.line][move.from.column] == 'p' and move.from.line == 6 and move.to.line == 4
        and (move.to.column < 7 and board[move.to.line][move.to.column + 1] == 'P' or
            move.to.column > 0 and board[move.to.line][move.to.column - 1] == 'P'))
    {
        en_passant_sq = { 5, move.from.column };
    }
    else
    {
        en_passant_sq = { -1, -1 };
    }
}

void add_possible_pawn_moves(const square& sq, square& en_passant_sq, array<array<char, 8>, 8>& board, vector<Move>& ret)
{
    int line = sq.line, column = sq.column;
    if (board[line][column] == 'P')
    {
        if (board[line + 1][column] == '#')
        {
            if (line + 1 == 7)
            {
                ret.push_back({ sq, {line + 1, column}, 'q' });
                ret.push_back({ sq, {line + 1, column}, 'r' });
                ret.push_back({ sq, {line + 1, column}, 'n' });
                ret.push_back({ sq, {line + 1, column}, 'b' });
            }
            else
            {
                ret.push_back({ sq, { line + 1, column } });
                if (board[line + 2][column] == '#' and line == 1)
                {
                    ret.push_back({ sq, { line + 2, column } });
                }
            }
        }

        if (column != 7 and (board[line + 1][column + 1] != '#' and
            not is_same_team(board[line][column], board[line + 1][column + 1]) or
            (line + 1 == en_passant_sq.line and column + 1 == en_passant_sq.column)))
        {
            if (line + 1 == 7)
            {
                ret.push_back({ sq, {line + 1, column + 1}, 'q' });
                ret.push_back({ sq, {line + 1, column + 1}, 'r' });
                ret.push_back({ sq, {line + 1, column + 1}, 'n' });
                ret.push_back({ sq, {line + 1, column + 1}, 'b' });
            }
            else
            {
                ret.push_back({ sq, { line + 1, column + 1 } });
            }
        }

        if (column != 0 and (board[line + 1][column - 1] != '#' and
            not is_same_team(board[line][column], board[line + 1][column - 1]) or
            (line + 1 == en_passant_sq.line and column - 1 == en_passant_sq.column)))
        {
            if (line + 1 == 7)
            {
                ret.push_back({ sq, {line + 1, column - 1}, 'q' });
                ret.push_back({ sq, {line + 1, column - 1}, 'r' });
                ret.push_back({ sq, {line + 1, column - 1}, 'n' });
                ret.push_back({ sq, {line + 1, column - 1}, 'b' });
            }
            else
            {
                ret.push_back({ sq, { line + 1, column - 1 } });
            }
        }
    }

    else if (board[line][column] == 'p')
    {
        if (board[line - 1][column] == '#')
        {
            if (line - 1 == 0)
            {
                ret.push_back({ sq, {line - 1, column}, 'q' });
                ret.push_back({ sq, {line - 1, column}, 'r' });
                ret.push_back({ sq, {line - 1, column}, 'n' });
                ret.push_back({ sq, {line - 1, column}, 'b' });
            }
            else
            {
                ret.push_back({ sq, { line - 1, column } });
                if (board[line - 2][column] == '#' and line == 6)
                {
                    ret.push_back({ sq, { line - 2, column } });
                }
            }
        }

        if (column != 7 and (board[line - 1][column + 1] != '#' and
            not is_same_team(board[line][column], board[line - 1][column + 1]) or
            (line - 1 == en_passant_sq.line and column + 1 == en_passant_sq.column)))
        {
            if (line - 1 == 0)
            {
                ret.push_back({ sq, {line - 1, column + 1}, 'q' });
                ret.push_back({ sq, {line - 1, column + 1}, 'r' });
                ret.push_back({ sq, {line - 1, column + 1}, 'n' });
                ret.push_back({ sq, {line - 1, column + 1}, 'b' });
            }
            else
            {
                ret.push_back({ sq, { line - 1, column + 1 } });
            }
        }

        if (column != 0 and (board[line - 1][column - 1] != '#' and
            not is_same_team(board[line][column], board[line - 1][column - 1]) or
            (line - 1 == en_passant_sq.line and column - 1 == en_passant_sq.column)))
        {
            if (line - 1 == 0)
            {
                ret.push_back({ sq, {line - 1, column - 1}, 'q' });
                ret.push_back({ sq, {line - 1, column - 1}, 'r' });
                ret.push_back({ sq, {line - 1, column - 1}, 'n' });
                ret.push_back({ sq, {line - 1, column - 1}, 'b' });
            }
            else
            {
                ret.push_back({ sq, { line - 1, column - 1 } });
            }
        }
    }
}

void add_possible_knight_moves(const square& sq, array<array<char, 8>, 8>& board, vector<Move>& ret)
{
    int line = sq.line, column = sq.column;
    if (line < 7 and column < 6 and not is_same_team(board[line][column], board[line + 1][column + 2]))
    {
        ret.push_back({ sq, { line + 1, column + 2 } });
    }

    if (line > 0 and column < 6 and not is_same_team(board[line][column], board[line - 1][column + 2]))
    {
        ret.push_back({ sq, { line - 1, column + 2 } });
    }

    if (line < 6 and column < 7 and not is_same_team(board[line][column], board[line + 2][column + 1]))
    {
        ret.push_back({ sq, { line + 2, column + 1 } });
    }

    if (line < 6 and column > 0 and not is_same_team(board[line][column], board[line + 2][column - 1]))
    {
        ret.push_back({ sq, { line + 2, column - 1 } });
    }

    if (line < 7 and column > 1 and not is_same_team(board[line][column], board[line + 1][column - 2]))
    {
        ret.push_back({ sq, { line + 1, column - 2 } });
    }

    if (line > 0 and column > 1 and not is_same_team(board[line][column], board[line - 1][column - 2]))
    {
        ret.push_back({ sq, { line - 1, column - 2 } });
    }

    if (line > 1 and column < 7 and not is_same_team(board[line][column], board[line - 2][column + 1]))
    {
        ret.push_back({ sq, { line - 2, column + 1 } });
    }

    if (line > 1 and column > 0 and not is_same_team(board[line][column], board[line - 2][column - 1]))
    {
        ret.push_back({ sq, { line - 2, column - 1 } });
    }
}

void add_possible_bishop_moves(const square& sq, array<array<char, 8>, 8>& board, vector<Move>& ret)
{
    int line = sq.line, column = sq.column;
    while (true)
    {
        line++;
        column++;
        if (line > 7 or column > 7 or is_same_team(board[line][column], board[sq.line][sq.column])) break;
        ret.push_back({ sq, { line, column } });
        if (board[line][column] != '#') break;
    }

    line = sq.line, column = sq.column;
    while (true)
    {
        line++;
        column--;
        if (line > 7 or column < 0 or is_same_team(board[line][column], board[sq.line][sq.column])) break;
        ret.push_back({ sq, { line, column } });
        if (board[line][column] != '#') break;
    }

    line = sq.line, column = sq.column;
    while (true)
    {
        line--;
        column++;
        if (line < 0 or column > 7 or is_same_team(board[line][column], board[sq.line][sq.column])) break;
        ret.push_back({ sq, { line, column } });
        if (board[line][column] != '#') break;
    }

    line = sq.line, column = sq.column;
    while (true)
    {
        line--;
        column--;
        if (line < 0 or column < 0 or is_same_team(board[line][column], board[sq.line][sq.column])) break;
        ret.push_back({ sq, { line, column } });
        if (board[line][column] != '#') break;
    }
}

void add_possible_rook_moves(const square& sq, array<array<char, 8>, 8>& board, vector<Move>& ret)
{
    int line = sq.line, column = sq.column;
    while (true)
    {
        column++;
        if (column > 7 or is_same_team(board[line][column], board[line][sq.column])) break;
        ret.push_back({ sq, { line, column } });
        if (board[line][column] != '#') break;
    }

    column = sq.column;
    while (true)
    {
        column--;
        if (column < 0 or is_same_team(board[line][column], board[line][sq.column])) break;
        ret.push_back({ sq, { line, column } });
        if (board[line][column] != '#') break;
    }

    column = sq.column;
    while (true)
    {
        line++;
        if (line > 7 or is_same_team(board[line][column], board[sq.line][column])) break;
        ret.push_back({ sq, { line, column } });
        if (board[line][column] != '#') break;
    }

    line = sq.line;
    while (true)
    {
        line--;
        if (line < 0 or is_same_team(board[line][column], board[sq.line][column])) break;
        ret.push_back({ sq, { line, column } });
        if (board[line][column] != '#') break;
    }
}

void add_possible_queen_moves(const square& sq, array<array<char, 8>, 8>& board, vector<Move>& ret)
{
    int line = sq.line, column = sq.column;
    while (true)
    {
        line++;
        column++;
        if (line > 7 or column > 7 or is_same_team(board[line][column], board[sq.line][sq.column])) break;
        ret.push_back({ sq, { line, column } });
        if (board[line][column] != '#') break;
    }

    line = sq.line, column = sq.column;
    while (true)
    {
        line++;
        column--;
        if (line > 7 or column < 0 or is_same_team(board[line][column], board[sq.line][sq.column])) break;
        ret.push_back({ sq, { line, column } });
        if (board[line][column] != '#') break;
    }

    line = sq.line, column = sq.column;
    while (true)
    {
        line--;
        column++;
        if (line < 0 or column > 7 or is_same_team(board[line][column], board[sq.line][sq.column])) break;
        ret.push_back({ sq, { line, column } });
        if (board[line][column] != '#') break;
    }

    line = sq.line, column = sq.column;
    while (true)
    {
        line--;
        column--;
        if (line < 0 or column < 0 or is_same_team(board[line][column], board[sq.line][sq.column])) break;
        ret.push_back({ sq, { line, column } });
        if (board[line][column] != '#') break;
    }

    line = sq.line, column = sq.column;
    while (true)
    {
        column++;
        if (column > 7 or is_same_team(board[line][column], board[line][sq.column])) break;
        ret.push_back({ sq, { line, column } });
        if (board[line][column] != '#') break;
    }

    column = sq.column;
    while (true)
    {
        column--;
        if (column < 0 or is_same_team(board[line][column], board[line][sq.column])) break;
        ret.push_back({ sq, { line, column } });
        if (board[line][column] != '#') break;
    }

    column = sq.column;
    while (true)
    {
        line++;
        if (line > 7 or is_same_team(board[line][column], board[sq.line][column])) break;
        ret.push_back({ sq, { line, column } });
        if (board[line][column] != '#') break;
    }

    line = sq.line;
    while (true)
    {
        line--;
        if (line < 0 or is_same_team(board[line][column], board[sq.line][column])) break;
        ret.push_back({ sq, { line, column } });
        if (board[line][column] != '#') break;
    }
}

void add_possible_king_moves(const square& sq, array<array<char, 8>, 8>& board, vector<Move>& ret)
{
    int line = sq.line, column = sq.column;
    if (column < 7 and not is_same_team(board[line][column + 1], board[line][column]))
    {
        ret.push_back({ sq, { line, column + 1 } });
    }

    if (column > 0 and not is_same_team(board[line][column - 1], board[line][column]))
    {
        ret.push_back({ sq, { line, column - 1 } });
    }

    if (line < 7 and not is_same_team(board[line + 1][column], board[line][column]))
    {
        ret.push_back({ sq, { line + 1, column } });
    }

    if (line > 0 and not is_same_team(board[line - 1][column], board[line][column]))
    {
        ret.push_back({ sq, { line - 1, column } });
    }

    if (line < 7 and column < 7 and
        not is_same_team(board[line + 1][column + 1], board[line][column]))
    {
        ret.push_back({ sq, { line + 1, column + 1 } });
    }

    if (line < 7 and column > 0 and
        not is_same_team(board[line + 1][column - 1], board[line][column]))
    {
        ret.push_back({ sq, { line + 1, column - 1 } });
    }

    if (line > 0 and column < 7 and
        not is_same_team(board[line - 1][column + 1], board[line][column]))
    {
        ret.push_back({ sq, { line - 1, column + 1 } });
    }

    if (line > 0 and column > 0 and
        not is_same_team(board[line - 1][column - 1], board[line][column]))
    {
        ret.push_back({ sq, { line - 1, column - 1 } });
    }
}

void add_possible_rock_moves(int rr, array<array<char, 8>, 8>& board, bool whites_turn, vector<Move>& ret)
{
    if (whites_turn)
    {
        if (rr & K and not is_attacked_by_black({ 0, 6 }, board) and
            not is_attacked_by_black({ 0, 5 }, board) and
            not is_attacked_by_black({ 0, 4 }, board) and
            board[0][6] == '#' and board[0][5] == '#')
        {
            ret.push_back({ {0, 4}, {0, 6} });
        }
        if (rr & Q and not is_attacked_by_black({ 0, 2 }, board) and
            not is_attacked_by_black({ 0, 3 }, board) and
            not is_attacked_by_black({ 0, 4 }, board) and
            board[0][1] == '#' and board[0][2] == '#' and board[0][3] == '#')
        {
            ret.push_back({ {0, 4}, {0, 2} });
        }

    }
    else
    {
        if (rr & k and not is_attacked_by_white({ 7, 6 }, board) and
            not is_attacked_by_white({ 7, 5 }, board) and
            not is_attacked_by_white({ 7, 4 }, board) and
            board[7][6] == '#' and board[7][5] == '#')
        {
            ret.push_back({ {7, 4}, {7, 6} });
        }
        if (rr & q and not is_attacked_by_white({ 7, 2 }, board) and
            not is_attacked_by_white({ 7, 3 }, board) and
            not is_attacked_by_white({ 7, 4 }, board) and
            board[7][1] == '#' and board[7][2] == '#' and board[7][3] == '#')
        {
            ret.push_back({ {7, 4}, {7, 2} });
        }
    }
}

void add_possible_moves(int rr, array<array<char, 8>, 8>& board, bool whites_turn, square& en_passant_sq, vector<Move>& moves)
{
    add_possible_rock_moves(rr, board, whites_turn, moves);

    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            if (whites_turn)
            {
                if (board[i][j] == 'P')
                    add_possible_pawn_moves({ i, j }, en_passant_sq, board, moves);
                else if (board[i][j] == 'N')
                    add_possible_knight_moves({ i, j }, board, moves);
                else if (board[i][j] == 'B')
                    add_possible_bishop_moves({ i, j }, board, moves);
                else if (board[i][j] == 'R')
                    add_possible_rook_moves({ i, j }, board, moves);
                else if (board[i][j] == 'Q')
                    add_possible_queen_moves({ i, j }, board, moves);
                else if (board[i][j] == 'K')
                    add_possible_king_moves({ i, j }, board, moves);
            }
            else
            {
                if (board[i][j] == 'p')
                    add_possible_pawn_moves({ i, j }, en_passant_sq, board, moves);
                else if (board[i][j] == 'n')
                    add_possible_knight_moves({ i, j }, board, moves);
                else if (board[i][j] == 'b')
                    add_possible_bishop_moves({ i, j }, board, moves);
                else if (board[i][j] == 'r')
                    add_possible_rook_moves({ i, j }, board, moves);
                else if (board[i][j] == 'q')
                    add_possible_queen_moves({ i, j }, board, moves);
                else if (board[i][j] == 'k')
                    add_possible_king_moves({ i, j }, board, moves);
            }
        }
    }
}

void reset_board()
{
    global_board[0][0] = 'R'; global_board[7][0] = 'r';   //R N B Q K B N R
    global_board[0][7] = 'R'; global_board[7][7] = 'r';   //P P P P P P P P
    global_board[0][1] = 'N'; global_board[7][1] = 'n';   //# # # # # # # # 
    global_board[0][6] = 'N'; global_board[7][6] = 'n';   //# # # # # # # #
    global_board[0][2] = 'B'; global_board[7][2] = 'b';   //# # # # # # # # 
    global_board[0][5] = 'B'; global_board[7][5] = 'b';   //# # # # # # # #
    global_board[0][3] = 'Q'; global_board[7][3] = 'q';   //p p p p p p p p
    global_board[0][4] = 'K'; global_board[7][4] = 'k';   //r n b q k b n r

    for (int i = 0; i < 8; i++) { global_board[1][i] = 'P'; global_board[6][i] = 'p'; }
    for (int i = 2; i < 6; i++) { for (int j = 0; j < 8; j++) { global_board[i][j] = '#'; } }

    global_whites_turn = true;
    book_finished = false;
    global_en_passant_sq = { -1, -1 };
    position_count.clear();
    stop_search = false;
    global_rr = K | Q | k | q;
}

void set_board(istringstream& iss)
{
    stop_search = false;
    book_finished = false;
    position_count.clear();
    string FEN;
    iss >> FEN;

    for (int i = 0; i < 8; i++) { for (int j = 0; j < 8; j++) { global_board[i][j] = '#'; } }

    int line = 7, column = 0;
    for (char i : FEN)
    {
        if (i == '/')
            line--;
        else if (i < '9')
            column += i - '0';
        else
            global_board[line][column] = i, column++;

        column %= 8;
    }

    string info;
    int iinfo;
    for (int i = 0; i < 5; i++)
    {
        if (i == 0)
        {
            iss >> info;
            if (info == "w")
                global_whites_turn = true;
            else
                global_whites_turn = false, played_moves = 1;
        }
        else if (i == 1)
        {
            iss >> info;
            for (char x : info)
            {
                if (x == 'K')
                    global_rr |= K;
                if (x == 'Q')
                    global_rr |= Q;
                if (x == 'k')
                    global_rr |= k;
                if (x == 'q')
                    global_rr |= q;
            }
        }
        else if (i == 2)
        {
            iss >> info;
            if (info.size() == 2)
                global_en_passant_sq = { notation_to_square(info) };
        }
        else if (i == 3)
        {
            iss >> iinfo;
        }
        else
        {
            iss >> iinfo;
            played_moves += 2 * (iinfo - 1);
        }
    }
}

int evaluate(array<array<char, 8>, 8>& board)
{
    int white_value = 0;
    int black_value = 0;
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            if (board[i][j] == 'P')
                white_value += 100 + pawn_table[i][j];
            else if (board[i][j] == 'N')
                white_value += 300 + knight_table[i][j];
            else if (board[i][j] == 'B')
                white_value += 350 + bishop_table[i][j];
            else if (board[i][j] == 'R')
                white_value += 500 + rook_table[i][j];
            else if (board[i][j] == 'Q')
                white_value += 900 + queen_table[i][j];
            else if (board[i][j] == 'K')
                white_value += king_table[i][j];
            else if (board[i][j] == 'p')
                black_value += 100 + pawn_table[7 - i][j];
            else if (board[i][j] == 'n')
                black_value += 300 + knight_table[7 - i][j];
            else if (board[i][j] == 'b')
                black_value += 350 + bishop_table[7 - i][j];
            else if (board[i][j] == 'r')
                black_value += 500 + rook_table[7 - i][j];
            else if (board[i][j] == 'q')
                black_value += 900 + queen_table[7 - i][j];
            else if (board[i][j] == 'k')
                black_value += king_table[7 - i][j];
        }
    }

    return white_value - black_value;
}

Move play_from_book(const string& position_FEN)
{
    if (opening_book[position_FEN].size() == 0)
    {
        book_finished = true;
        return nullmove;
    }

    random_device rd;
    mt19937 gen(rd());

    vector<float> weights;

    for (int i = 0; i < opening_book[position_FEN].size(); i++)
        weights.push_back(opening_book[position_FEN][i].weight);

    discrete_distribution<> dist(weights.begin(), weights.end());

    return opening_book[position_FEN][dist(gen)].move;
}

int dfs_search(int depth, int rr, square en_passant_sq, Move& move, bool whites_turn, array<array<char, 8>, 8> board, 
    int cur_parent_best_value)
{
    in_depth++;
    visited_node_count++;

    if (passed_time.elapsed_ms() > limit_time or stop_search)
    {
        stop_search = true;
        in_depth--;
        return 0;
    }

    bool king_is_attacked = is_illegal(move, board, not whites_turn);

    fix_rock_rights(board, move, rr);
    fix_en_passant_sq(board, move, en_passant_sq);
    make_move(move, board);
    whites_turn = not whites_turn;

    string position_FEN = create_FEN(board, whites_turn, rr, en_passant_sq);
    position_count[position_FEN]++;

    if (position_count[position_FEN] == 3)
    {
        in_depth--;
        position_count[position_FEN]--;
        return whites_turn ? -draw : draw;
    }

    if (depth == 0)
    {
        in_depth--;
        position_count[position_FEN]--;
        return evaluate(board);
    }

    vector<Move> moves;
    add_possible_moves(rr, board, whites_turn, en_passant_sq, moves);

    if (whites_turn)
    {
        int best_value = -INF;
        for (Move move : moves)
        {
            if (is_illegal(move, board, whites_turn))
            {
                visited_node_count++;
                best_value = max(best_value, -checkmate + in_depth);
                continue;
            }

            int move_value = dfs_search(depth - 1, rr, en_passant_sq, move, whites_turn, board, best_value);

            best_value = max(best_value, move_value);

            if (best_value >= cur_parent_best_value or stop_search)
                break;
        }

        in_depth--;
        position_count[position_FEN]--;
        if (best_value == -checkmate + in_depth + 1 and not king_is_attacked) return -draw;
        return best_value;
    }
    else
    {
        int best_value = INF;
        for (Move move : moves)
        {
            if (is_illegal(move, board, whites_turn))
            {
                visited_node_count++;
                best_value = min(best_value, checkmate - in_depth);
                continue;
            }

            int move_value = dfs_search(depth - 1, rr, en_passant_sq, move, whites_turn, board, best_value);

            best_value = min(best_value, move_value);

            if (best_value <= cur_parent_best_value or stop_search)
                break;
        }

        in_depth--;
        position_count[position_FEN]--;
        if (best_value == checkmate - in_depth - 1 and not king_is_attacked) return draw;
        return best_value;
    }
}

void move_generator(int depth, int rr, square en_passant_sq, bool whites_turn, array<array<char, 8>, 8> board)
{
    in_depth = 0;

    if (not book_finished)
    {
        Move bestmove = play_from_book(create_FEN(board, whites_turn, rr, en_passant_sq));

        if (bestmove != nullmove)
        {
            cout << "bestmove " << move_to_notation(bestmove) << endl;
            return;
        }
    }

    vector<Move> moves;
    add_possible_moves(rr, board, whites_turn, en_passant_sq, moves);

    if (depth != 0)
    {
        Move bestmove = nullmove;

        if (whites_turn)
        {
            int bestmove_value = -INF;

            for (Move move : moves)
            {
                if (is_illegal(move, board, whites_turn))
                {
                    if (bestmove_value < -checkmate)
                    {
                        bestmove_value = -checkmate;
                        bestmove = move;
                    }
                    continue;
                }

                int move_value = dfs_search(depth - 1, rr, en_passant_sq, move, whites_turn, board, bestmove_value);

                //cerr << square_to_notation(move.from) << " " << square_to_notation(move.to) << " " << move_value << endl;
                //debug_board(board, move);

                if (bestmove_value < move_value)
                {
                    bestmove_value = move_value;
                    bestmove = move;
                }
            }
        }
        else
        {
            int bestmove_value = INF;

            for (Move move : moves)
            {
                if (is_illegal(move, board, whites_turn))
                {
                    if (bestmove_value > checkmate)
                    {
                        bestmove_value = checkmate;
                        bestmove = move;
                    }
                    continue;
                }

                int move_value = dfs_search(depth - 1, rr, en_passant_sq, move, whites_turn, board, bestmove_value);

                //cerr << square_to_notation(move.from) << " " << square_to_notation(move.to) << " " << move_value << endl;
                //debug_board(board, move);

                if (bestmove_value > move_value)
                {
                    bestmove_value = move_value;
                    bestmove = move;
                }
            }
        }

        cout << "bestmove " << move_to_notation(bestmove) << endl;
    }

    else
    {
        passed_time.reset();
        Move last_bestmove = nullmove;
        int depth = 1;

        if (whites_turn)
        {
            while (passed_time.elapsed_ms() < limit_time and not stop_search)
            {
                visited_node_count = moves.size();
                timer depth_time;
                depth_time.reset();

                Move bestmove = nullmove;
                int bestmove_value = -INF;

                for (Move move : moves)
                {
                    if (is_illegal(move, board, whites_turn))
                    {
                        if (bestmove_value < -checkmate)
                        {
                            bestmove_value = -checkmate;
                            bestmove = move;
                        }
                        continue;
                    }

                    int move_value = dfs_search(depth - 1, rr, en_passant_sq, move, whites_turn, board, bestmove_value);

                    if (passed_time.elapsed_ms() > limit_time or stop_search)
                    {
                        stop_search = true;
                        break;
                    }

                    if (bestmove_value < move_value)
                    {
                        bestmove_value = move_value;
                        bestmove = move;
                    }
                }

                if (not stop_search)
                {
                    last_bestmove = bestmove;

                    cout << fixed << setprecision(0) << "info depth " << depth
                        << " time " << passed_time.elapsed_ms()
                        << " nodes " << visited_node_count
                        << " NPS " << visited_node_count * 1000 / depth_time.elapsed_ms()
                        << " score cp " << bestmove_value << endl;
                }
                depth++;
            }
        }
        else
        {
            while (passed_time.elapsed_ms() < limit_time and not stop_search)
            {
                visited_node_count = moves.size();
                timer depth_time;
                depth_time.reset();

                Move bestmove = nullmove;
                int bestmove_value = INF;

                for (Move move : moves)
                {
                    if (is_illegal(move, board, whites_turn))
                    {
                        if (bestmove_value > checkmate)
                        {
                            bestmove_value = checkmate;
                            bestmove = move;
                        }
                        continue;
                    }

                    int move_value = dfs_search(depth - 1, rr, en_passant_sq, move, whites_turn, board, bestmove_value);

                    if (passed_time.elapsed_ms() > limit_time or stop_search)
                    {
                        stop_search = true;
                        break;
                    }

                    if (bestmove_value > move_value)
                    {
                        bestmove_value = move_value;
                        bestmove = move;
                    }
                }

                if (not stop_search)
                {
                    last_bestmove = bestmove;

                    cout << fixed << setprecision(0) << "info depth " << depth
                        << " time " << passed_time.elapsed_ms()
                        << " nodes " << visited_node_count
                        << " NPS " << visited_node_count * 1000 / depth_time.elapsed_ms()
                        << " score cp " << bestmove_value << endl;
                }
                depth++;
            }
        }

        cout << "bestmove " << move_to_notation(last_bestmove) << endl;
    }
}

int main()
{
    load_book();
    string line;

    while (getline(cin, line)) {
        if (line.empty()) continue;
        istringstream iss(line);
        string cmd; iss >> cmd;

        if (cmd == "uci")
        {
            cout << "id name Bot23" << endl;
            cout << "id author Artifexis" << endl;
            cout << "uciok" << endl;
        }
        else if (cmd == "isready")
        {
            cout << "readyok" << endl;
        }
        else if (cmd == "position")
        {
            string type;
            iss >> type;

            if (type == "startpos")
            {
                reset_board();
            }
            else
            {
                set_board(iss);
            }

            string notation;
            iss >> notation;
            if (notation == "moves")
            {
                while (iss >> notation)
                {
                    Move move = notation_to_move(notation);
                    fix_rock_rights(global_board, move, global_rr);
                    fix_en_passant_sq(global_board, move, global_en_passant_sq);
                    make_move(move, global_board);
                    global_whites_turn = not global_whites_turn;
                    played_moves++;
                    position_count[create_FEN(global_board, global_whites_turn, global_rr, global_en_passant_sq)]++;
                }
            }


            //debug_board();
        }
        else if (cmd == "go")
        {
            if (search_thread.joinable())
                search_thread.join();

            string info;
            bool infinite = false;
            int depth = 0;
            float movetime = 0, wtime = -1, btime = -1, winc = 0, binc = 0;

            while (iss >> info)
            {
                if (info == "wtime") iss >> wtime;
                else if (info == "btime") iss >> btime;
                else if (info == "winc") iss >> winc;
                else if (info == "binc") iss >> binc;
                else if (info == "depth") iss >> depth;
                else if (info == "movetime") iss >> movetime;
                else if (info == "infinite") infinite = true;
            }

            if (movetime != 0)
                limit_time = (float)movetime;
            else if (infinite)
                limit_time = (float)INF;
            else if (depth == 0)
            {
                if (global_whites_turn)
                    calculate_time(wtime, winc, global_board);
                else
                    calculate_time(btime, binc, global_board);
            }

            search_thread = thread(move_generator, depth, global_rr, global_en_passant_sq, global_whites_turn, ref(global_board));
        }
        else if (cmd == "stop")
        {
            stop_search = true;
        }
        else if (cmd == "quit")
        {
            stop_search = true;

            if (search_thread.joinable())
                search_thread.join();

            break;
        }
    }
}