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

char board[8][8];
bool whites_turn = true;
int played_moves = 0;
int INF = INT_MAX;

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

//done
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

//done
struct Move
{
    square from, to;

    bool operator<(const Move& other) const 
    {
        if (from != other.from) return from < other.from;
        else return to < other.to;
    }
};

//done
struct possible_move
{
    Move move, ancestor_move;
    int depth;
    char local_board[8][8];
    bool whites_turn;
};

//done
bool is_same_team(char piece1, char piece2) 
{
    if (piece1 == '#' or piece2 == '#') return false;
    return (isupper(piece1) and isupper(piece2)) or (islower(piece1) and islower(piece2));
}

//done
square notation_to_square(string notation)  
{
    return { notation[1] - '1', notation[0] - 'a' };
}  

//done
string square_to_notation(square sq) 
{
    return string{ static_cast<char>(sq.column + 'a'), static_cast<char>(sq.line + '1') };
}

//done
void make_move(Move move, char (&local_board)[8][8])
{
    local_board[move.to.line][move.to.column] = local_board[move.from.line][move.from.column];
    local_board[move.from.line][move.from.column] = '#';
}

//done
void add_possible_pawn_moves(square sq, char board[8][8], vector<Move>& ret)
{
    int line = sq.line, column = sq.column;
    if (board[line][column] == 'P' and
        board[line + 1][column] == '#')
    {
        ret.push_back({ sq, { line + 1, column } });
        if (board[line + 2][column] == '#' and line == 1)
        {
            ret.push_back({ sq, { line + 2, column } });
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

    if (board[line][column] == 'p' and
        board[line - 1][column] == '#')
    {
        ret.push_back({ sq, { line - 1, column } });
        if (board[line - 2][column] == '#' and line == 6)
        {
            ret.push_back({ sq, { line - 2, column } });
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

//done
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

//done
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

//done
void add_possible_rock_moves(square sq, char board[8][8], vector<Move>& ret)
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

//done
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

//done
void add_possible_king_moves(square sq, char board[8][8], vector<Move>&ret)
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
                    add_possible_rock_moves({ i, j }, local_board, moves);
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
                    add_possible_rock_moves({ i, j }, local_board, moves);
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
    vector<Move> playable_moves;
    map<Move, pair<int, int>> playable_move_value;
    queue<possible_move> possible_moves;

    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            if (whites_turn)
            {
                if (board[i][j] == 'P')
                    add_possible_pawn_moves({ i, j }, board, playable_moves);
                else if (board[i][j] == 'N')
                    add_possible_knight_moves({ i, j }, board, playable_moves);
                else if (board[i][j] == 'B')
                    add_possible_bishop_moves({ i, j }, board, playable_moves);
                else if (board[i][j] == 'R')
                    add_possible_rock_moves({ i, j }, board, playable_moves);
                else if (board[i][j] == 'Q')
                    add_possible_queen_moves({ i, j }, board, playable_moves);
                else if (board[i][j] == 'K')
                    add_possible_king_moves({ i, j }, board, playable_moves);
            }
            else
            {
                if (board[i][j] == 'p')
                    add_possible_pawn_moves({ i, j }, board, playable_moves);
                else if (board[i][j] == 'n')
                    add_possible_knight_moves({ i, j }, board, playable_moves);
                else if (board[i][j] == 'b')
                    add_possible_bishop_moves({ i, j }, board, playable_moves);
                else if (board[i][j] == 'r')
                    add_possible_rock_moves({ i, j }, board, playable_moves);
                else if (board[i][j] == 'q')
                    add_possible_queen_moves({ i, j }, board, playable_moves);
                else if (board[i][j] == 'k')
                    add_possible_king_moves({ i, j }, board, playable_moves);
            }
        }
    }

    for (Move move:playable_moves) 
    {
        //cerr << square_to_notation(move.from) << " " << square_to_notation(move.to) << endl;
        playable_move_value[move].first = INF;
        playable_move_value[move].second = 0;
        possible_move pm;
        pm.move = move;
        pm.ancestor_move = move;
        pm.whites_turn = whites_turn;
        pm.depth = 1;
        memcpy(pm.local_board, board, sizeof(pm.local_board));
        possible_moves.push(pm);
    }

    while (true) 
    {
        auto now = high_resolution_clock::now();
        if (now - start > chrono::milliseconds(time_limit)) break;

        possible_move pm = possible_moves.front();
        possible_moves.pop();
        if (playable_move_value[pm.ancestor_move].second < pm.depth)
        {
            playable_move_value[pm.ancestor_move].first = INF;
            playable_move_value[pm.ancestor_move].second = pm.depth;
        }
        make_move(pm.move, pm.local_board);
        pm.whites_turn = not pm.whites_turn;
        playable_move_value[pm.ancestor_move].first = min(evaluate(pm.local_board),
                                                playable_move_value[pm.ancestor_move].first);

        vector<Move> moves;
        for (int i = 0; i < 8; i++)
        {
            for (int j = 0; j < 8; j++)
            {
                if (pm.whites_turn)
                {
                    if (pm.local_board[i][j] == 'P')
                        add_possible_pawn_moves({ i, j }, pm.local_board, moves);
                    else if (pm.local_board[i][j] == 'N')
                        add_possible_knight_moves({ i, j }, pm.local_board, moves);
                    else if (pm.local_board[i][j] == 'B')
                        add_possible_bishop_moves({ i, j }, pm.local_board, moves);
                    else if (pm.local_board[i][j] == 'R')
                        add_possible_rock_moves({ i, j }, pm.local_board, moves);
                    else if (pm.local_board[i][j] == 'Q')
                        add_possible_queen_moves({ i, j }, pm.local_board, moves);
                    else if (pm.local_board[i][j] == 'K')
                        add_possible_king_moves({ i, j }, pm.local_board, moves);
                }
                else
                {
                    if (pm.local_board[i][j] == 'p')
                        add_possible_pawn_moves({ i, j }, pm.local_board, moves);
                    else if (pm.local_board[i][j] == 'n')
                        add_possible_knight_moves({ i, j }, pm.local_board, moves);
                    else if (pm.local_board[i][j] == 'b')
                        add_possible_bishop_moves({ i, j }, pm.local_board, moves);
                    else if (pm.local_board[i][j] == 'r')
                        add_possible_rock_moves({ i, j }, pm.local_board, moves);
                    else if (pm.local_board[i][j] == 'q')
                        add_possible_queen_moves({ i, j }, pm.local_board, moves);
                    else if (pm.local_board[i][j] == 'k')
                        add_possible_king_moves({ i, j }, pm.local_board, moves);
                }
            }
        }

        for (Move move : moves)
        {
            possible_move p_m;
            p_m.move = move;
            p_m.ancestor_move = pm.ancestor_move;
            p_m.whites_turn = pm.whites_turn;
            p_m.depth = pm.depth + 1;
            memcpy(p_m.local_board, pm.local_board, sizeof(p_m.local_board));
            possible_moves.push(p_m);
        }
    }

    Move bestmove = playable_moves[0];
    int bestmove_value = -INF;
    for (const auto& playable_move : playable_move_value) 
    {
        cerr << square_to_notation(playable_move.first.from) << " "
            << square_to_notation(playable_move.first.to) << " "
            << bestmove_value << " " << playable_move.second.first << " " 
            << playable_move.second.second << endl;
        if (bestmove_value < playable_move.second.first) 
        {
            bestmove_value = playable_move.second.first;
            bestmove = playable_move.first;
        }
    }
    
    //a = bestmove;

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
