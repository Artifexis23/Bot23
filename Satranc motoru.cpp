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
    square en_passant_sq;
};

char board[8][8];
bool whites_turn;
int played_moves = 0;
int INF = INT_MAX;
int checkmate = INF - 1;
int rr;
square en_passant_sq;

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

void make_move(Move move, char(&local_board)[8][8])
{
    if (local_board[move.from.line][move.from.column] == 'K' and move.from.line == 0
        and move.from.column == 4 and move.to.line == 0 and move.to.column % 4 == 2)
    {
        if (move.to.column == 6)
        {
            local_board[0][7] = '#';
            local_board[0][6] = 'K';
            local_board[0][5] = 'R';
            local_board[0][4] = '#';
        }
        else 
        {
            local_board[0][0] = '#';
            local_board[0][2] = 'K';
            local_board[0][3] = 'R';
            local_board[0][4] = '#';
        }
        return;
    }

    else if (local_board[move.from.line][move.from.column] == 'k' and move.from.line == 7
        and move.from.column == 4 and move.to.line == 7 and move.to.column % 4 == 2)
    {
        if (move.to.column == 6)
        {
            local_board[7][7] = '#';
            local_board[7][6] = 'k';
            local_board[7][5] = 'r';
            local_board[7][4] = '#';
        }
        else 
        {
            local_board[7][0] = '#';
            local_board[7][2] = 'k';
            local_board[7][3] = 'r';
            local_board[7][4] = '#';
        }
        return;
    }

    if (local_board[move.to.line][move.to.column] == '#' and (local_board[move.from.line][move.from.column] == 'P'
         or local_board[move.from.line][move.from.column] == 'p') and
        (move.to.column == move.from.column + 1 or move.to.column == move.from.column - 1)) 
    {
        if (local_board[move.from.line][move.from.column] == 'P') 
        {
            local_board[move.to.line - 1][move.to.column] = '#';
        }
        else 
        {
            local_board[move.to.line + 1][move.to.column] = '#';
        }
    }

    local_board[move.to.line][move.to.column] = local_board[move.from.line][move.from.column];
    local_board[move.from.line][move.from.column] = '#';
}

void debug_board(char board[8][8], Move move) {
    char local_board[8][8];
    memcpy(local_board, board, sizeof(local_board));
    make_move(move, local_board);
    cerr << "DEBUG Board (whites_turn=" << whites_turn << "):" << endl;
    for (int i = 7; i >= 0; i--) {
        cerr << i + 1 << " ";
        for (int j = 0; j < 8; j++) {
            if (local_board[i][j] == '#') cerr << ". ";
            else cerr << local_board[i][j] << " ";
        }
        cerr << endl;
    }
    cerr << "  a b c d e f g h" << endl;
}

bool is_attacked_by_white(square sq, char board[8][8])
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
        if (line > 7 or column < 0 or islower(board[line][column]))
            break;

        if (board[line][column] == 'B' or board[line][column] == 'Q')
            return true;
    }

    line = sq.line, column = sq.column;
    while (true)
    {
        line--;
        column++;
        if (line < 0 or column > 7 or islower(board[line][column]))
            break;

        if (board[line][column] == 'B' or board[line][column] == 'Q')
            return true;
    }

    line = sq.line, column = sq.column;
    while (true)
    {
        line--;
        column--;
        if (line < 0 or column < 0 or islower(board[line][column]))
            break;

        if (board[line][column] == 'B' or board[line][column] == 'Q')
            return true;
    }

    line = sq.line, column = sq.column;
    while (true)
    {
        column++;
        if (column > 7 or islower(board[line][column]))
            break;

        if (board[line][column] == 'R' or board[line][column] == 'Q')
            return true;
    }

    column = sq.column;
    while (true)
    {
        column--;
        if (column < 0 or islower(board[line][column]))
            break;

        if (board[line][column] == 'R' or board[line][column] == 'Q')
            return true;
    }

    column = sq.column;
    while (true)
    {
        line++;
        if (line > 7 or islower(board[line][column]))
            break;

        if (board[line][column] == 'R' or board[line][column] == 'Q')
            return true;
    }

    line = sq.line;
    while (true)
    {
        line--;
        if (line < 0 or islower(board[line][column]))
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

    return false;
}

bool is_attacked_by_black(square sq, char board[8][8])
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
        if (line > 7 or column > 7 or isupper(board[line][column]))
            break;

        if (board[line][column] == 'b' or board[line][column] == 'q')
            return true;
    }

    line = sq.line, column = sq.column;
    while (true)
    {
        line++;
        column--;
        if (line > 7 or column < 0 or isupper(board[line][column]))
            break;

        if (board[line][column] == 'b' or board[line][column] == 'q')
            return true;
    }

    line = sq.line, column = sq.column;
    while (true)
    {
        line--;
        column++;
        if (line < 0 or column > 7 or isupper(board[line][column]))
            break;

        if (board[line][column] == 'b' or board[line][column] == 'q')
            return true;
    }

    line = sq.line, column = sq.column;
    while (true)
    {
        line--;
        column--;
        if (line < 0 or column < 0 or isupper(board[line][column]))
            break;

        if (board[line][column] == 'b' or board[line][column] == 'q')
            return true;
    }

    line = sq.line, column = sq.column;
    while (true)
    {
        column++;
        if (column > 7 or isupper(board[line][column]))
            break;

        if (board[line][column] == 'r' or board[line][column] == 'q')
            return true;
    }

    column = sq.column;
    while (true)
    {
        column--;
        if (column < 0 or isupper(board[line][column]))
            break;

        if (board[line][column] == 'r' or board[line][column] == 'q')
            return true;
    }

    column = sq.column;
    while (true)
    {
        line++;
        if (line > 7 or isupper(board[line][column]))
            break;

        if (board[line][column] == 'r' or board[line][column] == 'q')
            return true;
    }

    line = sq.line;
    while (true)
    {
        line--;
        if (line < 0 or isupper(board[line][column]))
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

    return false;
}

bool is_illegal(Move move, char board[8][8], bool whites_turn)
{
    char local_board[8][8];
    memcpy(local_board, board, sizeof(local_board));
    make_move(move, local_board);
    //debug_board(local_board);

    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            if (whites_turn and local_board[i][j] == 'K')
                return is_attacked_by_black({ i, j }, local_board);
            if (not whites_turn and local_board[i][j] == 'k')
                return is_attacked_by_white({ i, j }, local_board);
        }
    }
}

void fix_rock_rights(char board[8][8], Move move, int& rr)
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

void fix_en_passant_sq(char board[8][8], Move move, square& en_passant_sq)
{
    if (board[move.from.line][move.from.column] == 'P' and
        move.from.line == 1 and move.to.line == 3)
    {
        en_passant_sq = { 2, move.from.column };
    }
    else if (board[move.from.line][move.from.column] == 'p' and
        move.from.line == 6 and move.to.line == 4)
    {
        en_passant_sq = { 5, move.from.column };
    }
    else
    {
        en_passant_sq = { -1, -1 };
    }
}

void add_possible_pawn_moves(square sq, square en_passant_sq, char board[8][8], vector<Move>& ret)
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

        if (column != 7 and (board[line + 1][column + 1] != '#' and
            not is_same_team(board[line][column], board[line + 1][column + 1]) or
            (line + 1 == en_passant_sq.line and column + 1 == en_passant_sq.column)))
        {
            ret.push_back({ sq, { line + 1, column + 1 } });
        }

        if (column != 0 and (board[line + 1][column - 1] != '#' and
            not is_same_team(board[line][column], board[line + 1][column - 1]) or
            (line + 1 == en_passant_sq.line and column - 1 == en_passant_sq.column)))
        {
            ret.push_back({ sq, { line + 1, column - 1 } });
        }


    }

    else if (board[line][column] == 'p')
    {
        if (board[line - 1][column] == '#')
        {
            ret.push_back({ sq, { line - 1, column } });
            if (board[line - 2][column] == '#' and line == 6)
            {
                ret.push_back({ sq, { line - 2, column } });
            }
        }

        if (column != 7 and (board[line - 1][column + 1] != '#' and
            not is_same_team(board[line][column], board[line - 1][column + 1]) or
            (line - 1 == en_passant_sq.line and column + 1 == en_passant_sq.column)))
        {
            ret.push_back({ sq, { line - 1, column + 1 } });
        }

        if (column != 0 and (board[line - 1][column - 1] != '#' and
            not is_same_team(board[line][column], board[line - 1][column - 1]) or
            (line - 1 == en_passant_sq.line and column - 1 == en_passant_sq.column)))
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

void add_possible_moves(int rr, char local_board[8][8], bool whites_turn, square en_passant_sq, vector<Move>& moves)
{
    add_possible_rock_moves(rr, local_board, whites_turn, moves);

    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            if (whites_turn)
            {
                if (local_board[i][j] == 'P')
                    add_possible_pawn_moves({ i, j }, en_passant_sq, local_board, moves);
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
                    add_possible_pawn_moves({ i, j }, en_passant_sq, local_board, moves);
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
    for (char i : FEN)
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

/*string bfs_search(int time_limit)
{
    //debug_board();

    auto start = high_resolution_clock::now();
    vector<Move> moves;
    map<Move, pair<queue<possible_move>, int>> moves_value;

    add_possible_moves(rr, board, whites_turn, en_passant_sq, moves);

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

                fix_rock_rights(pm.local_board, pm.move, pm.rr);
                fix_en_passant_sq(pm.local_board, pm.move, pm.en_passant_sq);

                make_move(pm.move, pm.local_board);

                pm.whites_turn = not pm.whites_turn;
                moves_value[move].second = min(moves_value[move].second, evaluate(pm.local_board));

                vector<Move> next_moves;
                add_possible_moves(pm.rr, pm.local_board, pm.whites_turn, pm.en_passant_sq,
                    next_moves);

                for (Move next_move : next_moves)
                {
                    if (is_illegal(next_move, pm.local_board, pm.whites_turn)) continue;
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
        cerr << square_to_notation(move_value.first.from) << " "
            << square_to_notation(move_value.first.to) << " "
            << bestmove_value << " " << move_value.second.second << endl;
        if (bestmove_value < move_value.second.second)
        {
            bestmove_value = move_value.second.second;
            bestmove = move_value.first;
        }
    }

    return square_to_notation(bestmove.from) + square_to_notation(bestmove.to);
}*/

int dfs_search(int depth, int rr, square en_passant_sq, Move move, bool local_whites_turn, char board[8][8])
{
    char local_board[8][8];
    memcpy(local_board, board, sizeof(local_board));
    make_move(move, local_board);
    local_whites_turn = not local_whites_turn;
    fix_rock_rights(local_board, move, rr);
    fix_en_passant_sq(local_board, move, en_passant_sq);

    if (depth == 0)
    {
        return evaluate(local_board);
    }

    vector<Move> moves;
    add_possible_moves(rr, local_board, local_whites_turn, en_passant_sq, moves);

    if (whites_turn == local_whites_turn)
    {
        int best_value = -INF;
        for (Move move : moves)
        {
            if (is_illegal(move, local_board, local_whites_turn))
            {
                best_value = max(best_value, -checkmate);
                continue;
            }

            int move_value = dfs_search(depth - 1, rr, en_passant_sq, move, local_whites_turn, local_board);
            best_value = max(best_value, move_value);
        }

        return best_value;
    }
    else
    {
        int worst_value = INF;
        for (Move move : moves)
        {
            if (is_illegal(move, local_board, local_whites_turn))
            {
                worst_value = min(worst_value, checkmate);
                continue;
            }

            int move_value = dfs_search(depth - 1, rr, en_passant_sq, move, local_whites_turn, local_board);
            worst_value = min(worst_value, move_value);
        }

        return worst_value;
    }
}

string move_generator(int depth, int rr, square en_passant_sq, bool whites_turn, char board[8][8])
{
    vector<Move> moves;
    add_possible_moves(rr, board, whites_turn, en_passant_sq, moves);

    Move bestmove = { {0, 0}, {1, 1} };
    int bestmove_value = -INF;
    for (Move move : moves)
    {
        if (is_illegal(move, board, whites_turn)) continue;

        int move_value = dfs_search(depth - 1, rr, en_passant_sq, move, whites_turn, board);
        cerr << square_to_notation(move.from) << " " << square_to_notation(move.to) << " " << move_value << endl;
        debug_board(board, move);
        if (bestmove_value < move_value)
        {
            bestmove_value = move_value;
            bestmove = move;
        }
    }

    return square_to_notation(bestmove.from) + square_to_notation(bestmove.to);
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
                en_passant_sq = { -1, -1 };
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
                    else if (repeat == 2)
                    {
                        iss >> info;
                        if (info.size() == 2)
                            en_passant_sq = { notation_to_square(info) };
                    }
                    else if (repeat == 1) 
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

            string moves;
            iss >> moves;
            if (moves == "moves") 
            {
                while (iss >> moves)
                {
                    string here = moves.substr(0, 2);
                    string there = moves.substr(2, 2);
                    Move move = { notation_to_square(here), notation_to_square(there) };
                    fix_rock_rights(board, move, rr);
                    fix_en_passant_sq(board, move, en_passant_sq);
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
            int wtime = -1, btime = -1, winc = 0, binc = 0, depth = 4, movetime = -1;
            
            while (iss >> info) 
            {
                if (info == "wtime") iss >> wtime;
                else if (info == "btime") iss >> btime;
                else if (info == "winc") iss >> winc;
                else if (info == "binc") iss >> binc;
                else if (info == "depth") iss >> depth;
                else if (info == "movetime") iss >> movetime;
            }

            bestmove = move_generator(depth, rr, en_passant_sq, whites_turn, board);
            
            cout << "bestmove " << bestmove << endl;
        }
        else if (cmd == "quit") 
        {
            break;
        }
    }
}
