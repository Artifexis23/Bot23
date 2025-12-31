#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <unordered_map>
#include <cstring>
#include <climits>
#include <sstream>
#include <chrono>
#include <thread>
#include <atomic>

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
};

struct possible_move
{
    Move move;
    char local_board[8][8];
    bool whites_turn;
    int rr;
    square en_passant_sq;
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

char board[8][8];
bool whites_turn;
int played_moves = 0;
int rr;
square en_passant_sq;
unordered_map<string, int> position_count;
atomic<bool> stop_search;
thread search_thread;
timer passed_time;
int INF = INT_MAX;
float limit_time = INF;
int checkmate = INF - 1;
int draw = 10;

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

string create_FEN(char board[8][8], bool whites_turn, int rr, square en_passant_sq)
{
    string FEN = "";

    for (int line = 0; line < 8; line++)
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

    FEN.push_back('|');
    FEN += whites_turn ? "w" : "b";

    FEN.push_back('|');
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

    FEN.push_back('|');
    if (en_passant_sq != square{ -1, -1 })
        FEN += square_to_notation(en_passant_sq);
    else FEN.push_back('-');

    return FEN;
}

void calculate_time(float time, float inc, char board[8][8])
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

    if (local_board[move.from.line][move.from.column] == 'P' and move.to.line == 7)
    {
        if (move.pawn_promotion == 'q')
        {
            local_board[move.to.line][move.to.column] = 'Q';
        }
        else if (move.pawn_promotion == 'r')
        {
            local_board[move.to.line][move.to.column] = 'R';
        }
        else if (move.pawn_promotion == 'n')
        {
            local_board[move.to.line][move.to.column] = 'N';
        }
        else
        {
            local_board[move.to.line][move.to.column] = 'B';
        }
        local_board[move.from.line][move.from.column] = '#';
        return;
    }

    if (local_board[move.from.line][move.from.column] == 'p' and move.to.line == 0)
    {
        if (move.pawn_promotion == 'q')
        {
            local_board[move.to.line][move.to.column] = 'q';
        }
        else if (move.pawn_promotion == 'r')
        {
            local_board[move.to.line][move.to.column] = 'r';
        }
        else if (move.pawn_promotion == 'n')
        {
            local_board[move.to.line][move.to.column] = 'n';
        }
        else
        {
            local_board[move.to.line][move.to.column] = 'b';
        }
        local_board[move.from.line][move.from.column] = '#';
        return;
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

int dfs_search(int depth, int rr, square en_passant_sq, Move move, bool local_whites_turn, char board[8][8])
{
    if (passed_time.elapsed_ms() > limit_time or stop_search)
    {
        stop_search = true;
        return 0;
    }
    
    bool king_is_attacked = is_illegal(move, board, not local_whites_turn);

    char local_board[8][8];
    memcpy(local_board, board, sizeof(local_board));
    make_move(move, local_board);
    local_whites_turn = not local_whites_turn;
    fix_rock_rights(local_board, move, rr);
    fix_en_passant_sq(local_board, move, en_passant_sq);

    string position_FEN = create_FEN(local_board, local_whites_turn, rr, en_passant_sq);
    position_count[position_FEN]++;

    if (position_count[position_FEN] == 3)
    {
        position_count[position_FEN]--;
        return not local_whites_turn == whites_turn ? -draw : draw;
    }

    if (depth == 0)
    {
        position_count[position_FEN]--;
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

            if (stop_search)
                break;

            best_value = max(best_value, move_value);
        }

        position_count[position_FEN]--;
        if (best_value == -checkmate and not king_is_attacked) return -draw;
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

            if (stop_search)
                break;

            worst_value = min(worst_value, move_value);
        }

        position_count[position_FEN]--;
        if (worst_value == checkmate and not king_is_attacked) return draw;
        return worst_value;
    }
}

void move_generator(int depth, int rr, square en_passant_sq, bool whites_turn, char board[8][8])
{
    vector<Move> moves;
    add_possible_moves(rr, board, whites_turn, en_passant_sq, moves);

    if (depth != 0)
    {
        Move bestmove = { {0, 0}, {0, 0} };
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

        if (bestmove.pawn_promotion == '-')
            cout << "bestmove " << square_to_notation(bestmove.from) + square_to_notation(bestmove.to) << endl;
        else
            cout << "bestmove " << square_to_notation(bestmove.from) + square_to_notation(bestmove.to) +
            bestmove.pawn_promotion << endl;
    }

    else
    {
        passed_time.reset();
        Move last_bestmove = { {0, 0}, {0, 0} };
        int depth = 1;

        while (passed_time.elapsed_ms() < limit_time and not stop_search)
        {
            Move bestmove = { {0, 0}, {0, 0} };
            int bestmove_value = -INF;

            for (Move move : moves)
            {
                if (is_illegal(move, board, whites_turn)) continue;

                int move_value = dfs_search(depth - 1, rr, en_passant_sq, move, whites_turn, board);

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

                cout << "info depth " << depth
                    << " time " << passed_time.elapsed_ms() << endl;
            }
            depth++;
        }

        if (last_bestmove.pawn_promotion == '-')
            cout << "bestmove " << square_to_notation(last_bestmove.from) + square_to_notation(last_bestmove.to) << endl;
        else
            cout << "bestmove " << square_to_notation(last_bestmove.from) + square_to_notation(last_bestmove.to) +
            last_bestmove.pawn_promotion << endl;
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
                en_passant_sq = { -1, -1 };
                position_count.clear();
                stop_search = false;
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

            string notation;
            iss >> notation;
            if (notation == "moves")
            {
                while (iss >> notation)
                {
                    square here = notation_to_square(notation.substr(0, 2));
                    square there = notation_to_square(notation.substr(2, 2));
                    Move move = { here, there };
                    if (notation.size() == 5)
                        move.pawn_promotion = notation[4];
                    fix_rock_rights(board, move, rr);
                    fix_en_passant_sq(board, move, en_passant_sq);
                    make_move(move, board);
                    whites_turn = not whites_turn;
                    played_moves++;
                    position_count[create_FEN(board, whites_turn, rr, en_passant_sq)]++;
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
                limit_time = (float) movetime;
            else if (infinite)
                limit_time = (float) INF;
            else if (depth == 0)
            {
                if (whites_turn)
                    calculate_time(wtime, winc, board);
                else
                    calculate_time(btime, binc, board);
            }

            search_thread = thread(move_generator, depth, rr, en_passant_sq, whites_turn, ref(board));
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
