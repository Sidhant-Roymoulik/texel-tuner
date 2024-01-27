#include "lux.h"

#include <array>
#include <bit>
#include <cmath>
#include <iostream>
#include <sstream>

using namespace chess;
using namespace Lux;
using namespace std;

struct EvalInfo
{
    int gamephase = 0;
    int score = 0;

    Bitboard pawn[2];
};

struct Trace
{
    int score = 0;

    int material[6][2]{};
    int pst[6][64][2]{};

    int bishop_pair[2]{};
    int open_file[6][2]{};
    int semi_open_file[6][2]{};
};

int phase_values[6] = {0, 1, 1, 2, 4, 0};

const int material[6] = {S(113, 120), S(383, 265), S(374, 263), S(443, 424), S(893, 789), S(0, 0)};

int pst[6][64] = {
    // Pawn PST
    {S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(125, 265), S(187, 243), S(85, 223), S(167, 174), S(128, 193), S(207, 167), S(15, 247), S(-61, 292),
     S(-44, 134), S(-31, 141), S(10, 112), S(23, 82), S(83, 54), S(77, 47), S(-4, 108), S(-59, 113),
     S(-57, 33), S(-17, 14), S(-24, -5), S(1, -21), S(4, -33), S(-8, -24), S(-3, -4), S(-65, 2),
     S(-78, 3), S(-40, -9), S(-44, -30), S(-15, -40), S(-5, -42), S(-14, -45), S(-12, -27), S(-70, -27),
     S(-75, -13), S(-44, -12), S(-44, -36), S(-53, -22), S(-30, -27), S(-20, -38), S(25, -37), S(-50, -41),
     S(-91, 4), S(-38, -10), S(-70, -9), S(-74, -9), S(-60, -9), S(13, -32), S(31, -30), S(-65, -38),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0)},
    // Knight PST
    {S(-267, -58), S(-153, -40), S(-69, -1), S(-53, -39), S(129, -54), S(-131, -32), S(-13, -91), S(-156, -134),
     S(-106, -27), S(-59, 1), S(133, -39), S(63, 3), S(55, -12), S(126, -43), S(16, -30), S(-7, -73),
     S(-64, -30), S(116, -31), S(78, 20), S(125, 13), S(167, -13), S(227, -23), S(139, -34), S(96, -70),
     S(7, -21), S(53, 9), S(52, 41), S(110, 34), S(87, 36), S(141, 14), S(56, 13), S(62, -27),
     S(4, -25), S(37, -5), S(48, 31), S(45, 46), S(70, 29), S(57, 32), S(57, 11), S(15, -27),
     S(-9, -34), S(11, 2), S(46, 0), S(43, 26), S(58, 20), S(57, -3), S(66, -27), S(3, -32),
     S(-24, -58), S(-59, -21), S(6, -8), S(22, -3), S(24, 2), S(53, -27), S(0, -29), S(0, -65),
     S(-170, -22), S(-6, -79), S(-67, -23), S(-33, -15), S(3, -30), S(-18, -22), S(1, -77), S(-7, -96)},
    // Bishop PST
    {S(-33, -27), S(4, -34), S(-178, -1), S(-96, -6), S(-72, -1), S(-66, -12), S(-17, -18), S(-3, -41),
     S(-36, -15), S(23, -11), S(-29, 9), S(-47, -12), S(49, -11), S(84, -22), S(27, -8), S(-61, -22),
     S(-20, -1), S(56, -17), S(69, -6), S(64, -8), S(63, -11), S(99, -3), S(53, -2), S(6, 1),
     S(1, -9), S(17, 10), S(31, 16), S(85, 10), S(65, 17), S(69, 8), S(19, -2), S(5, -2),
     S(-7, -14), S(30, -2), S(25, 17), S(45, 25), S(59, 8), S(25, 12), S(23, -8), S(10, -18),
     S(4, -23), S(30, -9), S(28, 10), S(30, 12), S(26, 19), S(52, -2), S(33, -12), S(21, -29),
     S(10, -24), S(32, -34), S(31, -14), S(4, -4), S(18, 1), S(37, -19), S(60, -32), S(11, -51),
     S(-52, -36), S(3, -16), S(-16, -43), S(-30, -11), S(-15, -16), S(-13, -32), S(-68, -5), S(-34, -26)},
    // Rook PST
    {S(-16, 65), S(7, 55), S(-36, 74), S(34, 52), S(44, 52), S(-50, 70), S(-26, 62), S(-14, 58),
     S(-28, 62), S(-27, 66), S(33, 51), S(36, 49), S(72, 22), S(83, 32), S(-49, 67), S(-1, 52),
     S(-85, 64), S(-52, 62), S(-41, 56), S(-41, 57), S(-76, 60), S(33, 30), S(62, 25), S(-34, 42),
     S(-101, 59), S(-81, 55), S(-65, 68), S(-43, 48), S(-48, 51), S(7, 40), S(-47, 41), S(-83, 58),
     S(-107, 54), S(-99, 58), S(-89, 61), S(-72, 51), S(-56, 39), S(-57, 37), S(-19, 26), S(-76, 32),
     S(-107, 42), S(-87, 47), S(-84, 38), S(-90, 45), S(-60, 31), S(-39, 23), S(-39, 26), S(-75, 19),
     S(-102, 40), S(-75, 37), S(-93, 49), S(-74, 48), S(-59, 31), S(-24, 25), S(-49, 23), S(-134, 49),
     S(-58, 40), S(-64, 50), S(-63, 56), S(-43, 44), S(-38, 38), S(-28, 33), S(-82, 47), S(-54, 11)},
    // Queen PST
    {S(-51, -25), S(-29, 43), S(20, 32), S(16, 32), S(163, -33), S(175, -63), S(83, -26), S(64, 12),
     S(-42, -42), S(-68, 23), S(-14, 42), S(-24, 72), S(-63, 113), S(98, 3), S(37, 28), S(78, -30),
     S(-14, -54), S(-28, -4), S(12, -7), S(-10, 81), S(38, 63), S(115, 7), S(77, 6), S(93, -24),
     S(-56, 7), S(-45, 26), S(-29, 27), S(-26, 63), S(-8, 88), S(20, 54), S(-11, 92), S(-6, 49),
     S(-11, -50), S(-51, 44), S(-15, 21), S(-18, 71), S(-7, 44), S(-3, 40), S(1, 50), S(-4, 19),
     S(-29, -26), S(8, -73), S(-18, 11), S(-1, -7), S(-8, 5), S(4, 16), S(20, 4), S(7, -1),
     S(-55, -43), S(-12, -47), S(22, -71), S(5, -41), S(15, -39), S(26, -55), S(4, -78), S(13, -78),
     S(-5, -67), S(-22, -63), S(-12, -49), S(20, -99), S(-23, -19), S(-43, -55), S(-41, -52), S(-85, -76)},
    // King PST
    {S(-92, -108), S(294, -107), S(234, -74), S(143, -63), S(-195, 11), S(-107, 39), S(91, -11), S(104, -39),
     S(301, -80), S(95, 4), S(67, 1), S(174, -12), S(74, 5), S(58, 43), S(-47, 39), S(-201, 50),
     S(105, -12), S(106, 9), S(135, 8), S(37, 11), S(72, 9), S(172, 36), S(186, 34), S(-12, 15),
     S(-5, -22), S(-15, 29), S(17, 30), S(-55, 45), S(-51, 41), S(-55, 55), S(-10, 39), S(-113, 18),
     S(-99, -24), S(27, -16), S(-84, 41), S(-157, 59), S(-164, 64), S(-108, 49), S(-97, 28), S(-116, -5),
     S(23, -43), S(-7, -9), S(-55, 22), S(-115, 45), S(-109, 50), S(-94, 39), S(-23, 11), S(-50, -10),
     S(32, -58), S(32, -29), S(-20, 9), S(-120, 31), S(-89, 32), S(-44, 14), S(29, -16), S(41, -43),
     S(-7, -98), S(78, -78), S(32, -48), S(-107, -9), S(8, -51), S(-56, -16), S(59, -58), S(54, -96)},
};

const int bishop_pair = S(41, 74);
const int open_file[6] = {S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0)};
const int semi_open_file[6] = {S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0)};

// ---------------------------------------------------------------------------------------------------------------------
// Evaluation
// ---------------------------------------------------------------------------------------------------------------------

#define TraceIncr(parameter) trace.parameter[(int)c]++

void init_eval_tables()
{
    for (int i = (int)PieceType::PAWN; i <= (int)PieceType::KING; i++)
    {
        for (int j = Square::SQ_A1; j <= Square::SQ_H8; j++)
        {
            pst[i][j] += material[i];
        }
    }
}

template <Color c>
int eval_pawn(EvalInfo &info, const Board &board, Trace &trace)
{
    int score = 0;
    Bitboard pawns = board.pieces(PieceType::PAWN, c);

    info.pawn[(int)c] = pawns;

    while (pawns)
    {
        Square sq = builtin::poplsb(pawns);

        if (c == Color::WHITE)
            sq = sq ^ 56;

        score += pst[(int)PieceType::PAWN][sq];
        TraceIncr(pst[(int)PieceType::PAWN][sq]);
        TraceIncr(material[(int)PieceType::PAWN]);
    }

    return score;
}

template <Color c, PieceType p>
int eval_piece(EvalInfo &info, const Board &board, Trace &trace)
{
    int score = 0;
    Bitboard copy = board.pieces(p, c);
    info.gamephase += phase_values[(int)p] * builtin::popcount(copy);

    if (p == PieceType::BISHOP && (copy & (copy - 1)))
    {
        score += bishop_pair;
        TraceIncr(bishop_pair);
    }

    while (copy)
    {
        Square sq = builtin::poplsb(copy);

        if ((int)p >= 3)
        {
            Bitboard file = attacks::MASK_FILE[(int)utils::squareFile(sq)];
            if (!(file & (info.pawn[0] | info.pawn[1])))
            {
                score += open_file[(int)p];
                TraceIncr(open_file[(int)p]);
            }
            else if (!(file & info.pawn[(int)c]))
            {
                score += semi_open_file[(int)p];
                TraceIncr(semi_open_file[(int)p]);
            }
        }

        if (c == Color::WHITE)
            sq = sq ^ 56;

        score += pst[(int)p][sq];
        TraceIncr(pst[(int)p][sq]);
        TraceIncr(material[(int)p]);
    }

    return score;
}

void eval_pieces(EvalInfo &info, const Board &board, Trace &trace)
{
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

int evaluate(const Board &board, Trace &trace)
{
    // Check for draw by insufficient material
    if (board.isInsufficientMaterial())
        return 0;

    EvalInfo info;

    eval_pieces(info, board, trace);

    info.gamephase = std::min(info.gamephase, 24);

    int score = (mg_score(info.score) * info.gamephase + eg_score(info.score) * (24 - info.gamephase)) / 24;

    return (board.sideToMove() == Color::WHITE ? score : -score);
}

Trace eval(const Board &board)
{
    init_eval_tables();

    Trace trace{};

    int score = evaluate(board, trace);

    trace.score = score;

    return trace;
}

// ---------------------------------------------------------------------------------------------------------------------
// Trace Stuff
// ---------------------------------------------------------------------------------------------------------------------

parameters_t LuxEval::get_initial_parameters()
{
    parameters_t parameters;

    get_initial_parameter_array(parameters, material, 6);

    get_initial_parameter_array_2d(parameters, pst, 6, 64);

    get_initial_parameter_single(parameters, bishop_pair);
    get_initial_parameter_array(parameters, open_file, 6);
    get_initial_parameter_array(parameters, semi_open_file, 6);

    return parameters;
}

static coefficients_t get_coefficients(const Trace &trace)
{
    coefficients_t coefficients;

    get_coefficient_array(coefficients, trace.material, 6);

    get_coefficient_array_2d(coefficients, trace.pst, 6, 64);

    get_coefficient_single(coefficients, trace.bishop_pair);
    get_coefficient_array(coefficients, trace.open_file, 6);
    get_coefficient_array(coefficients, trace.semi_open_file, 6);

    return coefficients;
}

EvalResult LuxEval::get_fen_eval_result(const std::string &fen)
{
    Board board;
    board.setFen(fen);

    const auto trace = eval(board);
    EvalResult result;
    result.coefficients = get_coefficients(trace);
    result.score = trace.score;
    return result;
}

EvalResult LuxEval::get_external_eval_result(const chess::Board &board)
{
    const auto trace = eval(board);
    EvalResult result;
    result.coefficients = get_coefficients(trace);
    result.score = trace.score;
    return result;
}

// ---------------------------------------------------------------------------------------------------------------------
// Printing all parameters
// ---------------------------------------------------------------------------------------------------------------------

static int32_t round_value(tune_t value)
{
    return static_cast<int32_t>(round(value));
}

#if TAPERED
static void print_parameter(std::stringstream &ss, const pair_t parameter)
{
    const auto mg = round_value(parameter[static_cast<int32_t>(PhaseStages::Midgame)]);
    const auto eg = round_value(parameter[static_cast<int32_t>(PhaseStages::Endgame)]);
    ss << "S(" << mg << ", " << eg << ")";
}
#else
static void print_parameter(std::stringstream &ss, const tune_t parameter)
{
    ss << round_value(std::round(parameter));
}
#endif

static void print_single(std::stringstream &ss, const parameters_t &parameters, int &index, const std::string &name)
{
    ss << "const int " << name << " = ";
    print_parameter(ss, parameters[index]);
    index++;

    ss << ";" << endl;
}

static void print_array(std::stringstream &ss, const parameters_t &parameters, int &index, const std::string &name, int count)
{
    ss << "const int " << name << "[" << count << "] = {";
    for (auto i = 0; i < count; i++)
    {
        print_parameter(ss, parameters[index]);
        index++;

        if (i != count - 1)
        {
            ss << ", ";
        }
    }
    ss << "};" << endl;
}

static void print_array_2d(std::stringstream &ss, const parameters_t &parameters, int &index, const std::string &name, int count1, int count2)
{
    ss << "const int " << name << "[" << count1 << "][" << count2 << "] = {\n";
    for (auto i = 0; i < count1; i++)
    {
        ss << "{";
        for (auto j = 0; j < count2; j++)
        {
            print_parameter(ss, parameters[index]);
            index++;

            if (j != count2 - 1)
            {
                ss << ", ";
            }
        }
        ss << "},\n";
    }
    ss << "};\n";
}

static void print_pst(std::stringstream &ss, parameters_t &parameters, int &index, const std::string &name, int count1, int count2)
{
    // Normalize PST
    for (auto i = 0; i < count1; i++)
    {
        int sum = 0;
        for (auto j = 0; j < count2; j++)
        {
            if (i == 0 && (j < 8 || j >= 56))
                continue;

            sum += parameters[index + count2 * i + j][0] + parameters[index + count2 * i + j][1];
        }

        for (auto j = 0; j < count2; j++)
        {
            if (i == 0 && (j < 8 || j >= 56))
                continue;

            parameters[index + count2 * i + j][0] -= sum / count2 / 2;
            parameters[index + count2 * i + j][1] -= sum / count2 / 2;
        }
    }

    string names[6] = {"Pawn", "Knight", "Bishop", "Rook", "Queen", "King"};

    ss << "int " << name << "[" << count1 << "][" << count2 << "] = {\n";
    for (auto i = 0; i < count1; i++)
    {
        ss << "// " << names[i] << " PST\n";
        ss << "{";
        for (auto j = 0; j < count2; j++)
        {
            print_parameter(ss, parameters[index]);
            index++;

            if (j != count2 - 1)
            {
                ss << ", ";
            }
            else
            {
                ss << "},";
            }
            if (j % 8 == 7)
            {
                ss << endl;
            }
        }
    }
    ss << "};\n";
}

void LuxEval::print_parameters(const parameters_t &parameters)
{
    int index = 0;
    stringstream ss;
    parameters_t copy = parameters;

    print_array(ss, copy, index, "material", 6);
    ss << endl;

    print_pst(ss, copy, index, "pst", 6, 64);
    ss << endl;

    print_single(ss, copy, index, "bishop_pair");
    ss << endl;

    print_array(ss, copy, index, "open_file", 6);
    print_array(ss, copy, index, "semi_open_file", 6);

    cout << ss.str() << "\n";
}