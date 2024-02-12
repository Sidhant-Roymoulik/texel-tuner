#include "lux.h"

#include <array>
#include <bit>
#include <cmath>
#include <iostream>
#include <sstream>

using namespace chess;
using namespace Lux;
using namespace std;

struct EvalInfo {
    int gamephase = 0;
    int score     = 0;
    double scale  = 0;

    Bitboard pawn[2];
};

struct Trace {
    int score = 0;
    tune_t endgame_scale;

    int material[6][2]{};

    int pst[6][64][2]{};
    int mobility[5][28][2]{};

    int open_file[3][2]{};
    int semi_open_file[3][2]{};
    int attacked_by_pawn[2][2]{};
    int protected_by_pawn[6][2]{};

    int bishop_pair[2]{};
    int doubled_pawn[2]{};
    int isolated_pawn[2]{};
};

Bitboard isolated_pawn_mask[64];

int phase_values[6] = {0, 1, 1, 2, 4, 0};

const int material[6] = {S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0)};

int pst[6][64] = {
    // Pawn PSTS
    {S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0)},
    // Knight PSTS
    {S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0)},
    // Bishop PSTS
    {S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0)},
    // Rook PSTS
    {S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0)},
    // Queen PSTS
    {S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0)},
    // King PSTS
    {S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0)},
};
const int mobility[5][28] = {
    {S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0)},
    {S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0)},
    {S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0)},
    {S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0)},
    {S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0)},
};

const int open_file[3]         = {S(0, 0), S(0, 0), S(0, 0)};
const int semi_open_file[3]    = {S(0, 0), S(0, 0), S(0, 0)};
const int attacked_by_pawn[2]  = {S(0, 0), S(0, 0)};
const int protected_by_pawn[6] = {S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0)};

const int bishop_pair   = S(0, 0);
const int doubled_pawn  = S(0, 0);
const int isolated_pawn = S(0, 0);

// ---------------------------------------------------------------------------------------------------------------------
// Evaluation
// ---------------------------------------------------------------------------------------------------------------------

#define TraceIncr(parameter) trace.parameter[(int)c]++
#define TraceAdd(parameter, count) trace.parameter[(int)c] += count

void init_eval_tables() {
    for (int i = (int)PieceType::PAWN; i <= (int)PieceType::KING; i++) {
        for (int j = Square::SQ_A1; j <= Square::SQ_H8; j++) {
            pst[i][j] += material[i];
        }
    }
    for (Square i = Square::SQ_A1; i <= Square::SQ_H8; i = Square((int)i + 1)) {
        Bitboard file         = attacks::MASK_FILE[(int)utils::squareFile(i)];
        isolated_pawn_mask[i] = attacks::shift<Direction::WEST>(file) | attacks::shift<Direction::EAST>(file);
    }
}

// Get a bitboard with all pawn attacks based on pawn location and movement direction
template <Direction D>
Bitboard get_pawn_attacks(Bitboard &pawns) {
    Bitboard shifted = attacks::shift<D>(pawns);
    return attacks::shift<Direction::WEST>(shifted) | attacks::shift<Direction::EAST>(shifted);
}

template <Color c>
int eval_pawn(EvalInfo &info, const Board &board, Trace &trace) {
    int score   = 0;
    Bitboard bb = board.pieces(PieceType::PAWN, c);

    // Add pawn bb to eval info
    info.pawn[(int)c] = bb;

    // Init useful directions
    const Direction UP = c == Color::WHITE ? Direction::NORTH : Direction::SOUTH;

    // Init friendly pawn attacks
    Bitboard pawn_protection = get_pawn_attacks<UP>(bb);

    // Penalty for doubled pawns
    score +=
        doubled_pawn * builtin::popcount(bb & (attacks::shift<UP>(bb) | attacks::shift<UP>(attacks::shift<UP>(bb))));
    TraceAdd(doubled_pawn,
             builtin::popcount(bb & (attacks::shift<UP>(bb) | attacks::shift<UP>(attacks::shift<UP>(bb)))));

    while (bb) {
        Square sq = builtin::poplsb(bb);

        // Bonus if pawn is protected by pawn
        if (pawn_protection & (1ULL << sq)) {
            score += protected_by_pawn[0];
            TraceIncr(protected_by_pawn[0]);
        }

        // Penalty for isolated pawns
        if (!(isolated_pawn_mask[sq] & info.pawn[(int)c])) {
            score += isolated_pawn;
            TraceIncr(isolated_pawn);
        }

        if (c == Color::WHITE) sq = sq ^ 56;

        score += pst[(int)PieceType::PAWN][sq];
        TraceIncr(pst[(int)PieceType::PAWN][sq]);
        TraceIncr(material[(int)PieceType::PAWN]);
    }

    return score;
}

template <Color c, PieceType p>
int eval_piece(EvalInfo &info, const Board &board, Trace &trace) {
    int score   = 0;
    Bitboard bb = board.pieces(p, c);
    info.gamephase += phase_values[(int)p] * builtin::popcount(bb);

    // Init useful directions
    const Direction UP   = c == Color::WHITE ? Direction::NORTH : Direction::SOUTH;
    const Direction DOWN = c == Color::BLACK ? Direction::NORTH : Direction::SOUTH;

    // Init friendly and enemy pawn attacks
    Bitboard pawn_protection = get_pawn_attacks<UP>(info.pawn[(int)c]);
    Bitboard pawn_attacks    = get_pawn_attacks<DOWN>(info.pawn[(int)~c]);

    // Bishop pair bonus
    if (p == PieceType::BISHOP && (bb & (bb - 1))) {
        score += bishop_pair;
        TraceIncr(bishop_pair);
    }

    while (bb) {
        Square sq = builtin::poplsb(bb);

        // Open and Semi-Open file bonus
        if ((int)p >= 3) {
            Bitboard file = attacks::MASK_FILE[(int)utils::squareFile(sq)];
            if (!(file & info.pawn[(int)c])) {
                if (!(file & info.pawn[(int)~c])) {
                    score += open_file[(int)p - 3];
                    TraceIncr(open_file[(int)p - 3]);
                } else {
                    score += semi_open_file[(int)p - 3];
                    TraceIncr(semi_open_file[(int)p - 3]);
                }
            }
        }

        // Bonus if piece is protected by pawn
        if (pawn_protection & (1ULL << sq)) {
            score += protected_by_pawn[(int)p];
            TraceIncr(protected_by_pawn[(int)p]);
        }

        // Penalty is piece is attacked by pawn
        if (pawn_attacks & (1ULL << sq)) {
            score += attacked_by_pawn[c == board.sideToMove()];
            TraceIncr(attacked_by_pawn[c == board.sideToMove()]);
        }

        // Get move bb for piece
        Bitboard moves = 0;
        if (p == PieceType::KNIGHT)
            moves = attacks::knight(sq);
        else if (p == PieceType::BISHOP)
            moves = attacks::bishop(sq, board.occ());
        else if (p == PieceType::ROOK)
            moves = attacks::rook(sq, board.occ());
        else if (p == PieceType::QUEEN || p == PieceType::KING)
            moves = attacks::queen(sq, board.occ());

        // Bonus/penalty for number of moves per piece
        score += mobility[(int)p - 1][builtin::popcount(moves & ~board.us(c) & ~pawn_attacks)];
        TraceIncr(mobility[(int)p - 1][builtin::popcount(moves & ~board.us(c) & ~pawn_attacks)]);

        if (c == Color::WHITE) sq = sq ^ 56;

        score += pst[(int)p][sq];
        TraceIncr(pst[(int)p][sq]);
        TraceIncr(material[(int)p]);
    }

    return score;
}

void eval_pieces(EvalInfo &info, const Board &board, Trace &trace) {
    info.score += eval_pawn<Color::WHITE>(info, board, trace);
    info.score -= eval_pawn<Color::BLACK>(info, board, trace);

    info.score += eval_piece<Color::WHITE, PieceType::KNIGHT>(info, board, trace);
    info.score += eval_piece<Color::WHITE, PieceType::BISHOP>(info, board, trace);
    info.score += eval_piece<Color::WHITE, PieceType::ROOK>(info, board, trace);
    info.score += eval_piece<Color::WHITE, PieceType::QUEEN>(info, board, trace);

    info.score -= eval_piece<Color::BLACK, PieceType::KNIGHT>(info, board, trace);
    info.score -= eval_piece<Color::BLACK, PieceType::BISHOP>(info, board, trace);
    info.score -= eval_piece<Color::BLACK, PieceType::ROOK>(info, board, trace);
    info.score -= eval_piece<Color::BLACK, PieceType::QUEEN>(info, board, trace);

    info.score += eval_piece<Color::WHITE, PieceType::KING>(info, board, trace);
    info.score -= eval_piece<Color::BLACK, PieceType::KING>(info, board, trace);
}

double endgame_scale(EvalInfo &info) {
    // Divide the endgame score if the stronger side doesn't have many pawns left
    int num_missing_stronger_pawns = 8 - builtin::popcount(info.pawn[info.score < 0]);
    return (128 - num_missing_stronger_pawns * num_missing_stronger_pawns) / 128.0;
}

int evaluate(const Board &board, Trace &trace) {
    // Check for draw by insufficient material
    if (board.isInsufficientMaterial()) return 0;

    EvalInfo info;

    eval_pieces(info, board, trace);

    info.gamephase = std::min(info.gamephase, 24);
    info.scale     = endgame_scale(info);

    trace.endgame_scale = info.scale;

    int score =
        (mg_score(info.score) * info.gamephase + eg_score(info.score) * (24 - info.gamephase) * info.scale) / 24;

    return (board.sideToMove() == Color::WHITE ? score : -score);
}

Trace eval(const Board &board) {
    Trace trace{};

    int score = evaluate(board, trace);

    trace.score = score;

    return trace;
}

// ---------------------------------------------------------------------------------------------------------------------
// Trace Stuff
// ---------------------------------------------------------------------------------------------------------------------

parameters_t LuxEval::get_initial_parameters() {
    init_eval_tables();

    parameters_t parameters;

    get_initial_parameter_array(parameters, material, 6);

    get_initial_parameter_array_2d(parameters, pst, 6, 64);
    get_initial_parameter_array_2d(parameters, mobility, 5, 28);

    get_initial_parameter_array(parameters, open_file, 3);
    get_initial_parameter_array(parameters, semi_open_file, 3);
    get_initial_parameter_array(parameters, attacked_by_pawn, 2);
    get_initial_parameter_array(parameters, protected_by_pawn, 6);

    get_initial_parameter_single(parameters, bishop_pair);
    get_initial_parameter_single(parameters, doubled_pawn);
    get_initial_parameter_single(parameters, isolated_pawn);

    return parameters;
}

static coefficients_t get_coefficients(const Trace &trace) {
    coefficients_t coefficients;

    get_coefficient_array(coefficients, trace.material, 6);

    get_coefficient_array_2d(coefficients, trace.pst, 6, 64);
    get_coefficient_array_2d(coefficients, trace.mobility, 5, 28);

    get_coefficient_array(coefficients, trace.open_file, 3);
    get_coefficient_array(coefficients, trace.semi_open_file, 3);
    get_coefficient_array(coefficients, trace.attacked_by_pawn, 2);
    get_coefficient_array(coefficients, trace.protected_by_pawn, 6);

    get_coefficient_single(coefficients, trace.bishop_pair);
    get_coefficient_single(coefficients, trace.doubled_pawn);
    get_coefficient_single(coefficients, trace.isolated_pawn);

    return coefficients;
}

EvalResult LuxEval::get_fen_eval_result(const std::string &fen) {
    Board board;
    board.setFen(fen);

    const auto trace = eval(board);
    EvalResult result;
    result.coefficients  = get_coefficients(trace);
    result.score         = trace.score;
    result.endgame_scale = trace.endgame_scale;
    return result;
}

EvalResult LuxEval::get_external_eval_result(const chess::Board &board) {
    const auto trace = eval(board);
    EvalResult result;
    result.coefficients  = get_coefficients(trace);
    result.score         = trace.score;
    result.endgame_scale = trace.endgame_scale;
    return result;
}

// ---------------------------------------------------------------------------------------------------------------------
// Printing all parameters
// ---------------------------------------------------------------------------------------------------------------------

static int32_t round_value(tune_t value) { return static_cast<int32_t>(round(value)); }

#if TAPERED
static void print_parameter(std::stringstream &ss, const pair_t parameter) {
    const auto mg = round_value(parameter[static_cast<int32_t>(PhaseStages::Midgame)]);
    const auto eg = round_value(parameter[static_cast<int32_t>(PhaseStages::Endgame)]);
    ss << "S(" << mg << ", " << eg << ")";
}
#else
static void print_parameter(std::stringstream &ss, const tune_t parameter) { ss << round_value(std::round(parameter)); }
#endif

static void print_single(std::stringstream &ss, const parameters_t &parameters, int &index, const std::string &name) {
    ss << "const int " << name << " = ";
    print_parameter(ss, parameters[index]);
    index++;

    ss << ";" << endl;
}

static void print_array(std::stringstream &ss, const parameters_t &parameters, int &index, const std::string &name,
                        int count) {
    ss << "const int " << name << "[" << count << "] = {";
    for (auto i = 0; i < count; i++) {
        print_parameter(ss, parameters[index]);
        index++;

        if (i != count - 1) {
            ss << ", ";
        }
    }
    ss << "};" << endl;
}

static void print_array_2d(std::stringstream &ss, const parameters_t &parameters, int &index, const std::string &name,
                           int count1, int count2) {
    ss << "const int " << name << "[" << count1 << "][" << count2 << "] = {\n";
    for (auto i = 0; i < count1; i++) {
        ss << "{";
        for (auto j = 0; j < count2; j++) {
            print_parameter(ss, parameters[index]);
            index++;

            if (j != count2 - 1) ss << ", ";
        }
        ss << "},\n";
    }
    ss << "};\n";
}

static void print_pst(std::stringstream &ss, const parameters_t &parameters, int &index, const std::string &name,
                      int count1, int count2) {
    string names[6] = {"Pawn", "Knight", "Bishop", "Rook", "Queen", "King"};

    ss << "int " << name << "[" << count1 << "][" << count2 << "] = {\n";
    for (auto i = 0; i < count1; i++) {
        ss << "// " << names[i] << " PST\n";
        ss << "{";
        for (auto j = 0; j < count2; j++) {
            print_parameter(ss, parameters[index]);
            index++;

            if (j == count2 - 1) {
                ss << "}";
            }

            ss << ", ";

            if (j % 8 == 7) {
                ss << endl;
            }
        }
    }
    ss << "};\n";
}

static void normalize_2d(parameters_t &parameters, int &index, int count1, int count2, int offset) {
    for (auto i = 0; i < count1; i++) {
        int sum0 = 0, sum1 = 0, cnt0 = 0, cnt1 = 0;

        for (auto j = 0; j < count2; j++) {
            if (parameters[index + count2 * i + j][0] != 0 && parameters[index + count2 * i + j][1] != 0) {
                sum0 += parameters[index + count2 * i + j][0];
                cnt0++;

                sum1 += parameters[index + count2 * i + j][1];
                cnt1++;
            }
        }

        if (i + offset < 5) {
            if (cnt0 > 0) parameters[i + offset][0] += sum0 / cnt0;
            if (cnt1 > 0) parameters[i + offset][1] += sum1 / cnt1;
        }

        for (auto j = 0; j < count2; j++) {
            if (parameters[index + count2 * i + j][0] != 0 && parameters[index + count2 * i + j][1] != 0) {
                parameters[index + count2 * i + j][0] -= sum0 / cnt0;
                parameters[index + count2 * i + j][1] -= sum1 / cnt1;
            }
        }
    }
}

void LuxEval::print_parameters(const parameters_t &parameters) {
    int index = 0;
    stringstream ss;
    parameters_t bb = parameters;

    int pst_index_offset = 6, mobility_index_offset = 6 + 6 * 64;
    normalize_2d(bb, pst_index_offset, 6, 64, 0);
    normalize_2d(bb, mobility_index_offset, 5, 28, 1);

    print_array(ss, bb, index, "material", 6);
    ss << endl;

    print_pst(ss, bb, index, "pst", 6, 64);
    print_array_2d(ss, bb, index, "mobility", 5, 28);
    ss << endl;

    print_array(ss, bb, index, "open_file", 3);
    print_array(ss, bb, index, "semi_open_file", 3);
    print_array(ss, bb, index, "attacked_by_pawn", 2);
    print_array(ss, bb, index, "protected_by_pawn", 6);
    ss << endl;

    print_single(ss, bb, index, "bishop_pair");
    print_single(ss, bb, index, "doubled_pawn");
    print_single(ss, bb, index, "isolated_pawn");
    ss << endl;

    cout << ss.str() << "\n";
}