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

    int open_file[6][2]{};
    int semi_open_file[6][2]{};

    int bishop_pair[2]{};
    int attacked_by_pawn[2]{};
};

int phase_values[6] = {0, 1, 1, 2, 4, 0};

const int material[6] = {S(111, 80), S(416, 307), S(402, 306), S(533, 565), S(1293, 944), S(0, 0)};

int pst[6][64] = {
    // Pawn PSTS
    {S(0, 0),     S(0, 0),     S(0, 0),    S(0, 0),     S(0, 0),     S(0, 0),    S(0, 0),    S(0, 0),
     S(119, 173), S(146, 149), S(89, 140), S(145, 103), S(104, 120), S(186, 87), S(12, 155), S(-24, 191),
     S(3, 94),    S(-6, 91),   S(26, 70),  S(35, 46),   S(70, 23),   S(85, 29),  S(18, 64),  S(-4, 76),
     S(-10, 34),  S(1, 19),    S(3, 11),   S(28, -12),  S(29, -7),   S(21, 0),   S(11, 8),   S(-14, 18),
     S(-27, 18),  S(-25, 11),  S(-2, -1),  S(17, -13),  S(23, -9),   S(19, -8),  S(-2, -2),  S(-20, 3),
     S(-18, 6),   S(-27, 6),   S(-5, -4),  S(-5, 3),    S(10, 3),    S(18, -4),  S(23, -12), S(0, -8),
     S(-29, 22),  S(-20, 8),   S(-27, 19), S(-16, 19),  S(-9, 19),   S(36, 1),   S(32, -9),  S(-11, -3),
     S(0, 0),     S(0, 0),     S(0, 0),    S(0, 0),     S(0, 0),     S(0, 0),    S(0, 0),    S(0, 0)},
    // Knight PSTS
    {S(-204, -34), S(-115, -22), S(-55, 9),  S(-50, -11), S(86, -27), S(-109, -10), S(-23, -58), S(-117, -89),
     S(-94, -7),   S(-55, 12),   S(89, -19), S(31, 16),   S(25, 5),   S(84, -20),   S(-3, -10),  S(-19, -40),
     S(-48, -15),  S(74, -11),   S(52, 25),  S(83, 23),   S(114, 2),  S(162, -5),   S(101, -14), S(64, -41),
     S(3, -7),     S(39, 14),    S(31, 42),  S(77, 35),   S(59, 36),  S(98, 22),    S(42, 17),   S(42, -11),
     S(0, -8),     S(26, 4),     S(41, 28),  S(33, 41),   S(52, 30),  S(44, 30),    S(43, 16),   S(8, -8),
     S(-10, -18),  S(8, 10),     S(34, 5),   S(41, 22),   S(54, 19),  S(47, -0),    S(47, -11),  S(1, -21),
     S(-11, -41),  S(-36, -12),  S(7, 3),    S(28, 4),    S(31, 8),   S(42, -12),   S(2, -17),   S(4, -41),
     S(-121, -8),  S(-2, -38),   S(-37, -8), S(-17, -1),  S(13, -13), S(-3, -11),   S(3, -38),   S(-12, -71)},
    // Bishop PSTS
    {S(-28, -2),  S(-6, -9),  S(-145, 19), S(-89, 13), S(-66, 20), S(-57, 9), S(-31, 5),  S(2, -14),
     S(-34, 10),  S(-1, 4),   S(-35, 17),  S(-52, 5),  S(18, 4),   S(48, -2), S(2, 6),    S(-51, 0),
     S(-18, 14),  S(31, -0),  S(37, 2),    S(27, -0),  S(31, 0),   S(57, 3),  S(33, 7),   S(4, 14),
     S(-0, 5),    S(12, 12),  S(12, 15),   S(51, 7),   S(35, 10),  S(30, 8),  S(13, 3),   S(8, 10),
     S(5, -2),    S(20, 4),   S(12, 13),   S(34, 13),  S(39, -1),  S(9, 6),   S(14, -1),  S(22, -6),
     S(13, -8),   S(31, -1),  S(29, 7),    S(16, 7),   S(24, 12),  S(46, -5), S(31, -1),  S(24, -13),
     S(20, -10),  S(40, -21), S(30, -7),   S(16, 3),   S(27, 1),   S(37, -3), S(61, -26), S(19, -27),
     S(-20, -10), S(20, -2),  S(13, 2),    S(5, 4),    S(9, 3),    S(10, 6),  S(-29, 8),  S(-5, -11)},
    // Rook PSTS
    {S(10, 28),  S(24, 21),  S(-14, 37), S(41, 19),  S(44, 21),  S(-14, 31), S(2, 28),  S(17, 22),
     S(12, 27),  S(6, 31),   S(50, 19),  S(55, 17),  S(80, -2),  S(87, 6),   S(-1, 31), S(36, 18),
     S(-26, 27), S(-7, 26),  S(-7, 23),  S(-4, 24),  S(-27, 25), S(48, 4),   S(77, -0), S(8, 10),
     S(-34, 24), S(-29, 22), S(-17, 32), S(-4, 17),  S(-7, 19),  S(32, 12),  S(-4, 11), S(-20, 20),
     S(-40, 22), S(-31, 24), S(-23, 25), S(-19, 20), S(-0, 10),  S(-3, 9),   S(21, 2),  S(-22, 8),
     S(-32, 12), S(-18, 16), S(-13, 7),  S(-15, 12), S(5, 3),    S(17, -1),  S(12, 3),  S(-11, -4),
     S(-22, 6),  S(-7, 8),   S(-14, 14), S(1, 13),   S(14, 1),   S(29, -2),  S(15, -4), S(-43, 11),
     S(8, 9),    S(7, 12),   S(14, 11),  S(26, 4),   S(31, -2),  S(36, 0),   S(-7, 10), S(12, -9)},
    // Queen PSTS
    {S(-30, 3),   S(-27, 49), S(-8, 42),  S(2, 37),   S(113, -8), S(110, -14), S(78, -8),  S(57, 38),
     S(-30, 1),   S(-58, 30), S(-21, 40), S(-20, 57), S(-54, 86), S(59, 18),   S(20, 39),  S(64, 13),
     S(-3, -22),  S(-22, 2),  S(6, -17),  S(-19, 56), S(25, 36),  S(77, 15),   S(62, 17),  S(67, 22),
     S(-37, 25),  S(-28, 17), S(-25, 14), S(-22, 23), S(-6, 46),  S(3, 44),    S(-4, 78),  S(0, 71),
     S(2, -28),   S(-31, 29), S(-0, -7),  S(-11, 33), S(2, 7),    S(4, 22),    S(2, 54),   S(5, 42),
     S(-11, 11),  S(16, -40), S(4, -3),   S(18, -34), S(13, -8),  S(16, 9),    S(22, 24),  S(12, 43),
     S(-17, -12), S(13, -36), S(30, -39), S(30, -40), S(41, -47), S(44, -38),  S(17, -37), S(32, -30),
     S(18, -32),  S(17, -47), S(27, -42), S(35, -7),  S(16, -8),  S(-1, -17),  S(-5, -6),  S(-37, -29)},
    // King PSTS
    {S(-109, -78), S(188, -74), S(180, -57), S(102, -46), S(-151, 13), S(-84, 31), S(50, -6),  S(64, -29),
     S(228, -60),  S(67, 4),    S(37, 8),    S(113, -2),  S(30, 14),   S(43, 34),  S(-34, 30), S(-150, 38),
     S(75, -6),    S(73, 11),   S(97, 10),   S(15, 16),   S(37, 14),   S(110, 36), S(134, 28), S(-10, 11),
     S(11, -19),   S(-14, 24),  S(9, 28),    S(-54, 40),  S(-58, 39),  S(-60, 47), S(-23, 35), S(-97, 16),
     S(-90, -9),   S(14, -5),   S(-74, 37),  S(-136, 50), S(-141, 55), S(-95, 42), S(-89, 25), S(-105, 1),
     S(-4, -26),   S(-17, 0),   S(-57, 23),  S(-98, 38),  S(-96, 42),  S(-83, 34), S(-34, 14), S(-50, -4),
     S(5, -38),    S(9, -13),   S(-41, 16),  S(-101, 28), S(-77, 28),  S(-54, 19), S(2, -4),   S(11, -27),
     S(-21, -67),  S(43, -50),  S(12, -27),  S(-95, 2),   S(-14, -21), S(-50, -5), S(30, -38), S(19, -64)},
};

const int bishop_pair = S(33, 110);

const int open_file[6]      = {S(0, 0), S(0, 0), S(0, 0), S(28, 31), S(0, 0), S(0, 0)};
const int semi_open_file[6] = {S(0, 0), S(0, 0), S(0, 0), S(17, 15), S(0, 0), S(0, 0)};

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

const int attacked_by_pawn = S(0, 0);

// ---------------------------------------------------------------------------------------------------------------------
// Evaluation
// ---------------------------------------------------------------------------------------------------------------------

#define TraceIncr(parameter) trace.parameter[(int)c]++

void init_eval_tables() {
    for (int i = (int)PieceType::PAWN; i <= (int)PieceType::KING; i++) {
        for (int j = Square::SQ_A1; j <= Square::SQ_H8; j++) {
            pst[i][j] += material[i];
        }
    }
}

template <Color c>
int eval_pawn(EvalInfo &info, const Board &board, Trace &trace) {
    int score      = 0;
    Bitboard pawns = board.pieces(PieceType::PAWN, c);

    info.pawn[(int)c] = pawns;

    while (pawns) {
        Square sq = builtin::poplsb(pawns);

        if (c == Color::WHITE) sq = sq ^ 56;

        score += pst[(int)PieceType::PAWN][sq];
        TraceIncr(pst[(int)PieceType::PAWN][sq]);
        TraceIncr(material[(int)PieceType::PAWN]);
    }

    return score;
}

template <Color c, PieceType p>
int eval_piece(EvalInfo &info, const Board &board, Trace &trace) {
    int score     = 0;
    Bitboard copy = board.pieces(p, c);
    info.gamephase += phase_values[(int)p] * builtin::popcount(copy);

    const Direction UP_EAST = c == Color::BLACK ? Direction::NORTH_EAST : Direction::SOUTH_EAST;
    const Direction UP_WEST = c == Color::BLACK ? Direction::NORTH_WEST : Direction::SOUTH_WEST;

    Bitboard pawn_attacks = attacks::shift<UP_EAST>(info.pawn[(int)~c]) | attacks::shift<UP_WEST>(info.pawn[(int)~c]);

    if (p == PieceType::BISHOP && (copy & (copy - 1))) {
        score += bishop_pair;
        TraceIncr(bishop_pair);
    }

    while (copy) {
        Square sq = builtin::poplsb(copy);

        if ((int)p >= 3) {
            Bitboard file = attacks::MASK_FILE[(int)utils::squareFile(sq)];
            if (!(file & info.pawn[(int)c])) {
                if (!(file & info.pawn[(int)~c])) {
                    score += open_file[(int)p];
                    TraceIncr(open_file[(int)p]);
                } else {
                    score += semi_open_file[(int)p];
                    TraceIncr(semi_open_file[(int)p]);
                }
            }
        }

        if (pawn_attacks & (1ULL << sq)) {
            score += attacked_by_pawn;
            TraceIncr(attacked_by_pawn);
        }

        Bitboard moves = 0;
        if (p == PieceType::KNIGHT)
            moves = attacks::knight(sq);
        else if (p == PieceType::BISHOP)
            moves = attacks::bishop(sq, board.occ());
        else if (p == PieceType::ROOK)
            moves = attacks::rook(sq, board.occ());
        else if (p == PieceType::QUEEN || p == PieceType::KING)
            moves = attacks::queen(sq, board.occ());

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

    get_initial_parameter_array(parameters, open_file, 6);
    get_initial_parameter_array(parameters, semi_open_file, 6);

    get_initial_parameter_single(parameters, bishop_pair);
    get_initial_parameter_single(parameters, attacked_by_pawn);

    return parameters;
}

static coefficients_t get_coefficients(const Trace &trace) {
    coefficients_t coefficients;

    get_coefficient_array(coefficients, trace.material, 6);

    get_coefficient_array_2d(coefficients, trace.pst, 6, 64);
    get_coefficient_array_2d(coefficients, trace.mobility, 5, 28);

    get_coefficient_array(coefficients, trace.open_file, 6);
    get_coefficient_array(coefficients, trace.semi_open_file, 6);

    get_coefficient_single(coefficients, trace.bishop_pair);
    get_coefficient_single(coefficients, trace.attacked_by_pawn);

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

            if (j != count2 - 1) {
                ss << ", ";
            }
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
    parameters_t copy = parameters;

    int pst_index_offset = 6, mobility_index_offset = 6 + 6 * 64;
    normalize_2d(copy, pst_index_offset, 6, 64, 0);
    normalize_2d(copy, mobility_index_offset, 5, 28, 1);

    print_array(ss, copy, index, "material", 6);
    ss << endl;

    print_pst(ss, copy, index, "pst", 6, 64);
    print_array_2d(ss, copy, index, "mobility", 5, 28);
    ss << endl;

    print_array(ss, copy, index, "open_file", 6);
    print_array(ss, copy, index, "semi_open_file", 6);
    ss << endl;

    print_single(ss, copy, index, "bishop_pair");
    print_single(ss, copy, index, "attacked_by_pawn");
    ss << endl;

    cout << ss.str() << "\n";
}