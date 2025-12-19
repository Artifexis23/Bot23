#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <cstring>
#include <climits>
#include <sstream>
#include <chrono>

using namespace std;
using namespace std::chrono;

enum rock_rights
{
    K = 1 << 0,
    Q = 1 << 1,
    k = 1 << 2,
    q = 1 << 3
};

char board[8][8];
bool whites_turn;
int played_moves = 0;
int INF = INT_MAX;
int checkmate = -INF + 1;
int rr;

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
};

struct Move
{
    square from, to;

    bool operator<(const Move& other) const
    {
        if (from != other.from) return from < other.from;
        else return to < other.to;
    }
};

struct possible_move
{
    Move move;
    char local_board[8][8];
    bool whites_turn;
    int rr;
};

void debug_board() {
    cerr << "DEBUG Board (whites_turn=" << whites_turn << "):" << endl;
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

bool is_same_team(char piece1, char piece2) 
{
    if (piece1 == '#' or piece2 == '#') return false;
    return (isupper(piece1) and isupper(piece2)) or (islower(piece1) and islower(piece2));
}

square notation_to_square(string notation)  
{
    return { notation[1] - '1', notation[0] - 'a' };
}  

string square_to_notation(square sq) 
{
    return string{ static_cast<char>(sq.column + 'a'), static_cast<char>(sq.line + '1') };
}

void make_move(Move move, char (&local_board)[8][8])
{
    if (local_board[move.from.line][move.from.column] == 'K' and move.from.line == 0 
                                                             and move.from.column == 4) 
    {
        if (move.to.line == 0 and move.to.column == 6) 
        {
            local_board[0][6] = 'K';
            local_board[0][5] = 'R';
        }
        else if (move.to.line == 0 and move.to.column == 2) 
        {
            local_board[0][2] = 'K';
            local_board[0][3] = 'R';
        }
        return;
    }
    else if (local_board[move.from.line][move.from.column] == 'k' and move.from.line == 7
                                                                  and move.from.column == 4)
    {
        if (move.to.line == 7 and move.to.column == 6)
        {
            local_board[7][6] = 'k';
            local_board[7][5] = 'r';
        }
        else if (move.to.line == 7 and move.to.column == 2)
        {
            local_board[7][2] = 'k';
            local_board[7][3] = 'r';
        }
        return;
    }
    local_board[move.to.line][move.to.column] = local_board[move.from.line][move.from.column];
    local_board[move.from.line][move.from.column] = '#';
}

bool is_attacked(square sq, char board[8][8], bool whites_turn)
{
    int line = sq.line, column = sq.column;

    if (whites_turn) 
    {
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
            if (line > 7 or column > 7 or is_same_team(board[line][column], 
                                                       board[sq.line][sq.column]))
                break;

            if (board[line][column] == 'b' or board[line][column] == 'q') 
                return true;
        }

        line = sq.line, column = sq.column;
        while (true)
        {
            line++;
            column--;
            if (line > 7 or column < 0 or is_same_team(board[line][column],
                                                       board[sq.line][sq.column]))
                break;

            if (board[line][column] == 'b' or board[line][column] == 'q')
                return true;
        }

        line = sq.line, column = sq.column;
        while (true)
        {
            line--;
            column++;
            if (line < 0 or column > 7 or is_same_team(board[line][column],
                                                       board[sq.line][sq.column]))
                break;

            if (board[line][column] == 'b' or board[line][column] == 'q')
                return true;
        }

        line = sq.line, column = sq.column;
        while (true)
        {
            line--;
            column--;
            if (line < 0 or column < 0 or is_same_team(board[line][column],
                                                       board[sq.line][sq.column]))
                break;

            if (board[line][column] == 'b' or board[line][column] == 'q')
                return true;
        }

        line = sq.line, column = sq.column;
        while (true)
        {
            column++;
            if (column > 7 or is_same_team(board[line][column], board[sq.line][sq.column]))
                break;

            if (board[line][column] == 'r' or board[line][column] == 'q')
                return true;
        }

        column = sq.column;
        while (true)
        {
            column--;
            if (column < 0 or is_same_team(board[line][column], board[sq.line][sq.column]))
                break;

            if (board[line][column] == 'r' or board[line][column] == 'q')
                return true;
        }

        column = sq.column;
        while (true)
        {
            line++;
            if (line > 7 or is_same_team(board[line][column], board[sq.line][sq.column]))
                break;

            if (board[line][column] == 'r' or board[line][column] == 'q')
                return true;
        }

        line = sq.line;
        while (true)
        {
            line--;
            if (line < 0 or is_same_team(board[line][column], board[sq.line][sq.column]))
                break;

            if (board[line][column] == 'r' or board[line][column] == 'q')
                return true;
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
    }
    else 
    {
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
            if (line > 7 or column > 7 or is_same_team(board[line][column],
                board[sq.line][sq.column]))
                break;

            if (board[line][column] == 'B' or board[line][column] == 'Q')
                return true;
        }

        line = sq.line, column = sq.column;
        while (true)
        {
            line++;
            column--;
            if (line > 7 or column < 0 or is_same_team(board[line][column],
                board[sq.line][sq.column]))
                break;

            if (board[line][column] == 'B' or board[line][column] == 'Q')
                return true;
        }

        line = sq.line, column = sq.column;
        while (true)
        {
            line--;
            column++;
            if (line < 0 or column > 7 or is_same_team(board[line][column],
                board[sq.line][sq.column]))
                break;

            if (board[line][column] == 'B' or board[line][column] == 'Q')
                return true;
        }

        line = sq.line, column = sq.column;
        while (true)
        {
            line--;
            column--;
            if (line < 0 or column < 0 or is_same_team(board[line][column],
                board[sq.line][sq.column]))
                break;

            if (board[line][column] == 'B' or board[line][column] == 'Q')
                return true;
        }

        line = sq.line, column = sq.column;
        while (true)
        {
            column++;
            if (column > 7 or is_same_team(board[line][column], board[sq.line][sq.column]))
                break;

            if (board[line][column] == 'R' or board[line][column] == 'Q')
                return true;
        }

        column = sq.column;
        while (true)
        {
            column--;
            if (column < 0 or is_same_team(board[line][column], board[sq.line][sq.column]))
                break;

            if (board[line][column] == 'R' or board[line][column] == 'Q')
                return true;
        }

        column = sq.column;
        while (true)
        {
            line++;
            if (line > 7 or is_same_team(board[line][column], board[sq.line][sq.column]))
                break;

            if (board[line][column] == 'R' or board[line][column] == 'Q')
                return true;
        }

        line = sq.line;
        while (true)
        {
            line--;
            if (line < 0 or is_same_team(board[line][column], board[sq.line][sq.column]))
                break;

            if (board[line][column] == 'R' or board[line][column] == 'Q')
                return true;
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
    }

    return false;
}

bool is_illegal(char board[8][8], bool whites_turn) 
{
    for (int i = 0; i < 8; i++) 
    {
        for (int j = 0; j < 8; j++) 
        {
            if (whites_turn and board[i][j] == 'K') 
                return is_attacked({ i, j }, board, whites_turn);
            if (not whites_turn and board[i][j] == 'k')
                return is_attacked({ i, j }, board, whites_turn);
        }
    }
    return false;
}

void add_possible_pawn_moves(square sq, char board[8][8], vector<Move>& ret)
{
    int line = sq.line, column = sq.column;
    if (board[line][column] == 'P')
    {
        if (board[line + 1][column] == '#') 
        {
            ret.push_back({ sq, { line + 1, column } });
            if (board[line + 2][column] == '#' and line == 1)
            {
                ret.push_back({ sq, { line + 2, column } });
            }
        }

        if (column != 7 and board[line + 1][column + 1] != '#' and
            not is_same_team(board[line][column], board[line + 1][column + 1]))
        {
            ret.push_back({ sq, { line + 1, column + 1 } });
        }

        if (column != 0 and board[line + 1][column - 1] != '#' and
            not is_same_team(board[line][column], board[line + 1][column - 1]))
        {
            ret.push_back({ sq, { line + 1, column - 1 } });
        }
    }

    

    if (board[line][column] == 'p')
    {
        if (board[line - 1][column] == '#') 
        {
            ret.push_back({ sq, { line - 1, column } });
            if (board[line - 2][column] == '#' and line == 6)
            {
                ret.push_back({ sq, { line - 2, column } });
            }
        }

        if (column != 7 and board[line - 1][column + 1] != '#' and
            not is_same_team(board[line][column], board[line - 1][column + 1]))
        {
            ret.push_back({ sq, { line - 1, column + 1 } });
        }

        if (column != 0 and board[line - 1][column - 1] != '#' and
            not is_same_team(board[line][column], board[line - 1][column - 1]))
        {
            ret.push_back({ sq, { line - 1, column - 1 } });
        }
    }
}

void add_possible_knight_moves(square sq, char board[8][8], vector<Move>& ret)   
{
    int line = sq.line, column = sq.column;
    if (line < 7 and column < 6 and (board[line + 1][column + 2] == '#'
        or not is_same_team(board[line][column], board[line + 1][column + 2])))
    {
        ret.push_back({ sq, { line + 1, column + 2 } });
    }

    if (line > 0 and column < 6 and (board[line - 1][column + 2] == '#'
        or not is_same_team(board[line][column], board[line - 1][column + 2])))
    {
        ret.push_back({ sq, { line - 1, column + 2 } });
    }

    if (line < 6 and column < 7 and (board[line + 2][column + 1] == '#'
        or not is_same_team(board[line][column], board[line + 2][column + 1])))
    {
        ret.push_back({ sq, { line + 2, column + 1 } });
    }

    if (line < 6 and column > 0 and (board[line + 2][column - 1] == '#'
        or not is_same_team(board[line][column], board[line + 2][column - 1])))
    {
        ret.push_back({ sq, { line + 2, column - 1 } });
    }

    if (line < 7 and column > 1 and (board[line + 1][column - 2] == '#'
        or not is_same_team(board[line][column], board[line + 1][column - 2])))
    {
        ret.push_back({ sq, { line + 1, column - 2 } });
    }

    if (line > 0 and column > 1 and (board[line - 1][column - 2] == '#'
        or not is_same_team(board[line][column], board[line - 1][column - 2])))
    {
        ret.push_back({ sq, { line - 1, column - 2 } });
    }

    if (line > 1 and column < 7 and (board[line - 2][column + 1] == '#'
        or not is_same_team(board[line][column], board[line - 2][column + 1])))
    {
        ret.push_back({ sq, { line - 2, column + 1 } });
    }

    if (line > 1 and column > 0 and (board[line - 2][column - 1] == '#'
        or not is_same_team(board[line][column], board[line - 2][column - 1])))
    {
        ret.push_back({ sq, { line - 2, column - 1 } });
    }
}

void add_possible_bishop_moves(square sq, char board[8][8], vector<Move>& ret)
{
    int line = sq.line, column = sq.column;
    while (true) 
    {
        line++;
        column++;
        if (line > 7 or column > 7 or
            is_same_team(board[line][column], board[sq.line][sq.column])) break;
        ret.push_back({ sq, { line, column } });
        if (board[line][column] != '#') break;
    }

    line = sq.line, column = sq.column;
    while (true)
    {
        line++;
        column--;
        if (line > 7 or column < 0 or
            is_same_team(board[line][column], board[sq.line][sq.column])) break;
        ret.push_back({ sq, { line, column } });
        if (board[line][column] != '#') break;
    }

    line = sq.line, column = sq.column;
    while (true)
    {
        line--;
        column++;
        if (line < 0 or column > 7 or
            is_same_team(board[line][column], board[sq.line][sq.column])) break;
        ret.push_back({ sq, { line, column } });
        if (board[line][column] != '#') break;
    }

    line = sq.line, column = sq.column;
    while (true)
    {
        line--;
        column--;
        if (line < 0 or column < 0 or
            is_same_team(board[line][column], board[sq.line][sq.column])) break;
        ret.push_back({ sq, { line, column } });
        if (board[line][column] != '#') break;
    }
}

void add_possible_rook_moves(square sq, char board[8][8], vector<Move>& ret)
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

void add_possible_queen_moves(square sq, char board[8][8], vector<Move>& ret)
{
    int line = sq.line, column = sq.column;
    while (true)
    {
        line++;
        column++;
        if (line > 7 or column > 7 or
            is_same_team(board[line][column], board[sq.line][sq.column])) break;
        ret.push_back({ sq, { line, column } });
        if (board[line][column] != '#') break;
    }

    line = sq.line, column = sq.column;
    while (true)
    {
        line++;
        column--;
        if (line > 7 or column < 0 or
            is_same_team(board[line][column], board[sq.line][sq.column])) break;
        ret.push_back({ sq, { line, column } });
        if (board[line][column] != '#') break;
    }

    line = sq.line, column = sq.column;
    while (true)
    {
        line--;
        column++;
        if (line < 0 or column > 7 or
            is_same_team(board[line][column], board[sq.line][sq.column])) break;
        ret.push_back({ sq, { line, column } });
        if (board[line][column] != '#') break;
    }

    line = sq.line, column = sq.column;
    while (true)
    {
        line--;
        column--;
        if (line < 0 or column < 0 or
            is_same_team(board[line][column], board[sq.line][sq.column])) break;
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

void add_possible_king_moves(square sq, char board[8][8], vector<Move>& ret)
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

void add_possible_rock_moves(int rr, char board[8][8], bool whites_turn, vector<Move>& ret)
{
    if (whites_turn) 
    {
        if (rr & K and not is_attacked({ 0, 7 }, board, whites_turn) and
                       not is_attacked({ 0, 6 }, board, whites_turn) and
                       not is_attacked({ 0, 5 }, board, whites_turn) and
                       not is_attacked({ 0, 4 }, board, whites_turn) and
                       board[0][6] == '#' and board[0][5] == '#') 
        {
            ret.push_back({ {0, 4}, {0, 6} });
        }
        if (rr & Q and not is_attacked({ 0, 0 }, board, whites_turn) and
                       not is_attacked({ 0, 1 }, board, whites_turn) and
                       not is_attacked({ 0, 2 }, board, whites_turn) and
                       not is_attacked({ 0, 3 }, board, whites_turn) and
                       not is_attacked({ 0, 4 }, board, whites_turn) and
                       board[0][1] == '#' and board[0][2] == '#' and board[0][3] == '#') 
        {
            ret.push_back({ {0, 4}, {0, 2} });
        }
        
    }
    else 
    {
        if (rr & k and not is_attacked({ 7, 7 }, board, whites_turn) and
                       not is_attacked({ 7, 6 }, board, whites_turn) and
                       not is_attacked({ 7, 5 }, board, whites_turn) and
                       not is_attacked({ 7, 4 }, board, whites_turn) and
                       board[7][6] == '#' and board[7][5] == '#')
        {
            ret.push_back({ {7, 4}, {7, 6} });
        }
        if (rr & q and not is_attacked({ 7, 0 }, board, whites_turn) and
                       not is_attacked({ 7, 1 }, board, whites_turn) and
                       not is_attacked({ 7, 2 }, board, whites_turn) and
                       not is_attacked({ 7, 3 }, board, whites_turn) and
                       not is_attacked({ 7, 4 }, board, whites_turn) and
                       board[7][1] == '#' and board[7][2] == '#' and board[7][3] == '#')
        {
            ret.push_back({ {7, 4}, {7, 2} });
        }
    }
}

int evaluate(char board[8][8]) 
{
    int white_value = 0;
    int black_value = 0;
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            if (board[i][j] == 'P')
                white_value += 100;
            else if (board[i][j] == 'N')
                white_value += 300;
            else if (board[i][j] == 'B')
                white_value += 350;
            else if (board[i][j] == 'R')
                white_value += 500;
            else if (board[i][j] == 'Q')
                white_value += 900;
            else if (board[i][j] == 'p')
                black_value += 100;
            else if (board[i][j] == 'n')
                black_value += 300;
            else if (board[i][j] == 'b')
                black_value += 350;
            else if (board[i][j] == 'r')
                black_value += 500;
            else if (board[i][j] == 'q')
                black_value += 900;
        }
    }
    if (whites_turn) return white_value - black_value;
    else return black_value - white_value;
}

pair<string, int> dfs_search(int depth, Move move, bool whites_turn, char (&board)[8][8])
{
    //debug_board();

    char local_board[8][8];
    memcpy(local_board, board, sizeof(local_board));
    make_move(move, local_board);

    if (depth == 0) 
    {
        return { square_to_notation(move.from) + square_to_notation(move.to),
                evaluate(local_board) };
    }

    vector<Move> moves;
    for (int i = 0; i < 8; i++) 
    {
        for (int j = 0; j < 8; j++) 
        {
            if (whites_turn) 
            {
                if (local_board[i][j] == 'P')
                    add_possible_pawn_moves({ i, j }, local_board, moves);
                else if (local_board[i][j] == 'N')
                    add_possible_knight_moves({ i, j }, local_board, moves);
                else if (local_board[i][j] == 'B')
                    add_possible_bishop_moves({ i, j }, local_board, moves);
                else if (local_board[i][j] == 'R')
                    add_possible_rook_moves({ i, j }, local_board, moves);
                else if (local_board[i][j] == 'Q')
                    add_possible_queen_moves({ i, j }, local_board, moves);
                else if (local_board[i][j] == 'K')
                    add_possible_king_moves({ i, j }, local_board, moves);
            }
            else 
            {
                if (local_board[i][j] == 'p')
                    add_possible_pawn_moves({ i, j }, local_board, moves);
                else if (local_board[i][j] == 'n')
                    add_possible_knight_moves({ i, j }, local_board, moves);
                else if (local_board[i][j] == 'b')
                    add_possible_bishop_moves({ i, j }, local_board, moves);
                else if (local_board[i][j] == 'r')
                    add_possible_rook_moves({ i, j }, local_board, moves);
                else if (local_board[i][j] == 'q')
                    add_possible_queen_moves({ i, j }, local_board, moves);
                else if (local_board[i][j] == 'k')
                    add_possible_king_moves({ i, j }, local_board, moves);
            }
        }
    }

    pair<string, int> bestmove_and_value;
    for (Move move : moves) 
    {
        pair<string, int> move_and_value = dfs_search(depth - 1, move, not whites_turn, local_board);
        if (bestmove_and_value.second < move_and_value.second)
            bestmove_and_value = move_and_value;
    }
    return bestmove_and_value;
}

string bfs_search(int time_limit)
{
    //debug_board();

    auto start = high_resolution_clock::now();
    vector<Move> moves;
    map<Move, pair<queue<possible_move>, int>> moves_value;

    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            if (whites_turn)
            {
                add_possible_rock_moves(rr, board, whites_turn, moves);
                if (board[i][j] == 'P')
                    add_possible_pawn_moves({ i, j }, board, moves);
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
                add_possible_rock_moves(rr, board, whites_turn, moves);
                if (board[i][j] == 'p')
                    add_possible_pawn_moves({ i, j }, board, moves);
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

    for (Move move : moves) 
    {
        //cerr << square_to_notation(move.from) << " " << square_to_notation(move.to) << endl;
        moves_value[move].second = INF;
        possible_move pm;
        pm.move = move;
        pm.rr = rr;
        pm.whites_turn = whites_turn;
        memcpy(pm.local_board, board, sizeof(pm.local_board));
        moves_value[move].first.push(pm);
    }

    bool stop = false;
    while (not stop) 
    {
        for (Move move : moves) 
        {
            auto now = high_resolution_clock::now();
            if (now - start > chrono::milliseconds(time_limit)) { stop = true; break; }

            moves_value[move].second = INF;
            int number_of_possible_moves = moves_value[move].first.size();
            if (number_of_possible_moves == 0) moves_value[move].second = checkmate;

            while (number_of_possible_moves--) 
            {
                possible_move pm = moves_value[move].first.front();
                moves_value[move].first.pop();

                if (pm.local_board[pm.move.from.line][pm.move.from.column] == 'K')
                    pm.rr &= ~(K | Q);
                if (pm.local_board[pm.move.from.line][pm.move.from.column] == 'k')
                    pm.rr &= ~(k | q);
                if (pm.local_board[pm.move.from.line][pm.move.from.column] == 'R' or
                    pm.local_board[pm.move.to.line][pm.move.to.column] == 'R')
                {
                    if (pm.move.from.line == 0 and pm.move.from.column == 0)
                        pm.rr &= ~Q;
                    if (pm.move.from.line == 0 and pm.move.from.column == 7)
                        pm.rr &= ~K;
                    if (pm.move.to.line == 0 and pm.move.to.column == 0)
                        pm.rr &= ~Q;
                    if (pm.move.to.line == 0 and pm.move.to.column == 7)
                        pm.rr &= ~K;
                } 
                if (pm.local_board[pm.move.from.line][pm.move.from.column] == 'r' or 
                    pm.local_board[pm.move.to.line][pm.move.to.column] == 'r')
                {
                    if (pm.move.from.line == 7 and pm.move.from.column == 0)
                        pm.rr &= ~q;
                    if (pm.move.from.line == 7 and pm.move.from.column == 7)
                        pm.rr &= ~k;
                    if (pm.move.to.line == 7 and pm.move.to.column == 0)
                        pm.rr &= ~q;
                    if (pm.move.to.line == 7 and pm.move.to.column == 7)
                        pm.rr &= ~k;
                }

                make_move(pm.move, pm.local_board);
                if (is_illegal(pm.local_board, pm.whites_turn)) continue;

                pm.whites_turn = not pm.whites_turn;
                moves_value[move].second = min(moves_value[move].second, evaluate(pm.local_board));

                vector<Move> next_moves;
                for (int i = 0; i < 8; i++)
                {
                    for (int j = 0; j < 8; j++)
                    {
                        if (pm.whites_turn)
                        {
                            add_possible_rock_moves(pm.rr, pm.local_board, whites_turn, next_moves);
                            if (pm.local_board[i][j] == 'P')
                                add_possible_pawn_moves({ i, j }, pm.local_board, next_moves);
                            else if (pm.local_board[i][j] == 'N')
                                add_possible_knight_moves({ i, j }, pm.local_board, next_moves);
                            else if (pm.local_board[i][j] == 'B')
                                add_possible_bishop_moves({ i, j }, pm.local_board, next_moves);
                            else if (pm.local_board[i][j] == 'R')
                                add_possible_rook_moves({ i, j }, pm.local_board, next_moves);
                            else if (pm.local_board[i][j] == 'Q')
                                add_possible_queen_moves({ i, j }, pm.local_board, next_moves);
                            else if (pm.local_board[i][j] == 'K')
                                add_possible_king_moves({ i, j }, pm.local_board, next_moves);
                        }
                        else
                        {
                            add_possible_rock_moves(pm.rr, pm.local_board, whites_turn, next_moves);
                            if (pm.local_board[i][j] == 'p')
                                add_possible_pawn_moves({ i, j }, pm.local_board, next_moves);
                            else if (pm.local_board[i][j] == 'n')
                                add_possible_knight_moves({ i, j }, pm.local_board, next_moves);
                            else if (pm.local_board[i][j] == 'b')
                                add_possible_bishop_moves({ i, j }, pm.local_board, next_moves);
                            else if (pm.local_board[i][j] == 'r')
                                add_possible_rook_moves({ i, j }, pm.local_board, next_moves);
                            else if (pm.local_board[i][j] == 'q')
                                add_possible_queen_moves({ i, j }, pm.local_board, next_moves);
                            else if (pm.local_board[i][j] == 'k')
                                add_possible_king_moves({ i, j }, pm.local_board, next_moves);
                        }
                    }
                }

                for (Move next_move : next_moves) 
                {
                    possible_move npm;
                    npm.move = next_move;
                    npm.whites_turn = pm.whites_turn;
                    npm.rr = pm.rr;
                    memcpy(npm.local_board, pm.local_board, sizeof(npm.local_board));
                    moves_value[move].first.push(npm);
                }
            }
        }
    }

    Move bestmove;
    int bestmove_value;
    bestmove_value = -INF;
    for (const auto& move_value : moves_value) 
    {
        /*cerr << square_to_notation(move_value.first.from) << " "
            << square_to_notation(move_value.first.to) << " "
            << bestmove_value << " " << move_value.second.second << endl;*/
        if (bestmove_value < move_value.second.second) 
        {
            bestmove_value = move_value.second.second;
            bestmove = move_value.first;
        }
    }

    return square_to_notation(bestmove.from) + square_to_notation(bestmove.to);
}

void reset_board() 
{
    board[0][0] = 'R'; board[7][0] = 'r';   //R N B Q K B N R
    board[0][7] = 'R'; board[7][7] = 'r';   //P P P P P P P P
    board[0][1] = 'N'; board[7][1] = 'n';   //# # # # # # # # 
    board[0][6] = 'N'; board[7][6] = 'n';   //# # # # # # # #
    board[0][2] = 'B'; board[7][2] = 'b';   //# # # # # # # # 
    board[0][5] = 'B'; board[7][5] = 'b';   //# # # # # # # #
    board[0][3] = 'Q'; board[7][3] = 'q';   //p p p p p p p p
    board[0][4] = 'K'; board[7][4] = 'k';   //r n b q k b n r

    for (int i = 0; i < 8; i++) { board[1][i] = 'P'; board[6][i] = 'p'; }
    for (int i = 2; i < 6; i++) { for (int j = 0; j < 8; j++) { board[i][j] = '#'; } }
}

void set_board(string FEN) 
{
    for (int i = 0; i < 8; i++) { for (int j = 0; j < 8; j++) { board[i][j] = '#'; } }

    int line = 7, column = 0;
    for (char i:FEN) 
    {
        if (i == '/')
            line--;
        else if (i < '9')
            column += i - '0';
        else
            board[line][column] = i, column++;
        
        column %= 8;
    }
}

int main() 
{
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
                whites_turn = true;
                rr = K | Q | k | q;
            }
            else 
            {
                string FEN;
                iss >> FEN;
                set_board(FEN);

                string info;
                int repeat = 5, iinfo;
                while (repeat--) 
                {
                    if (repeat == 4) 
                    {
                        iss >> info;
                        if (info == "w")
                            whites_turn = true;
                        else
                            whites_turn = false, played_moves = 1;
                    }
                    else if (repeat == 3) 
                    {
                        iss >> info;
                        for (char x : info) 
                        {
                            if (x == 'K')
                                rr |= K;
                            if (x == 'Q')
                                rr |= Q;
                            if (x == 'k')
                                rr |= k;
                            if (x == 'q')
                                rr |= q;
                        }
                    }
                    else if (repeat == 1) 
                    {
                        iss >> iinfo;
                        played_moves += 2 * (iinfo - 1);
                    }   
                }
            }

            string moves;
            iss >> moves;
            if (moves == "moves") 
            {
                while (iss >> moves)
                {
                    string here = moves.substr(0, 2);
                    string there = moves.substr(2, 2);
                    Move move = { notation_to_square(here), notation_to_square(there) };
                    make_move(move, board);
                    whites_turn = not whites_turn;
                    played_moves++;
                }
            }
            

            //debug_board();
        }
        else if (cmd == "go") 
        {
            string bestmove, info;
            int wtime = -1, btime = -1, winc = 0, binc = 0, depth = -1, movetime = -1;
            
            while (iss >> info) 
            {
                if (info == "wtime") iss >> wtime;
                else if (info == "btime") iss >> btime;
                else if (info == "winc") iss >> winc;
                else if (info == "binc") iss >> binc;
                else if (info == "depth") iss >> depth;
                else if (info == "movetime") iss >> movetime;
            }

            if (wtime > 0 or btime > 0) 
            {
                if (whites_turn) 
                {
                    int needed_time = wtime / (80 - played_moves / 2) + winc;
                    bestmove = bfs_search(needed_time);
                }
                else 
                {
                    int needed_time = btime / (80 - played_moves / 2) + binc;
                    bestmove = bfs_search(needed_time);
                }
            }
            else if (movetime > 0) 
            {
                bestmove = bfs_search(movetime);
            }
            else if (depth > 0) 
            {
                bestmove = bfs_search(1);
            }
            
            cout << "bestmove " << bestmove << endl;
        }
        else if (cmd == "quit") 
        {
            break;
        }
    }
}
