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
    int score;

    Bitboard pawn[2];
};

struct Trace
{
    int score;

    int material[6][2]{};
    int pst[6][64][2]{};

    int bishop_pair[2]{};
    int open_file[2]{};
    int semi_open_file[2]{};
};

int phase_values[6] = {0, 1, 1, 2, 4, 0};

const int material[6] = {S(135, 154), S(475, 309), S(454, 306), S(545, 493), S(1071, 932), S(0, 0)};

int pst[6][64] = {
    // Pawn PST
    {S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(63, 426), S(182, 371), S(27, 349), S(143, 285), S(102, 313), S(198, 265), S(-4, 374), S(-126, 428),
     S(-45, 148), S(-32, 156), S(16, 124), S(37, 85), S(102, 54), S(95, 45), S(4, 119), S(-58, 123),
     S(-59, 27), S(-16, 5), S(-23, -17), S(10, -38), S(14, -53), S(-3, -41), S(5, -17), S(-70, -9),
     S(-86, -8), S(-43, -22), S(-48, -47), S(-9, -60), S(2, -62), S(-9, -65), S(-5, -44), S(-76, -45),
     S(-82, -27), S(-47, -26), S(-47, -54), S(-57, -37), S(-27, -46), S(-16, -57), S(38, -56), S(-51, -61),
     S(-102, -6), S(-41, -23), S(-77, -22), S(-82, -21), S(-66, -20), S(24, -50), S(44, -48), S(-70, -58),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0)},
    // Knight PST
    {S(-323, -66), S(-181, -50), S(-68, -16), S(-35, -58), S(182, -74), S(-178, -38), S(-31, -111), S(-198, -152),
     S(-140, -30), S(-68, -5), S(189, -63), S(80, -2), S(87, -27), S(152, -57), S(24, -45), S(6, -94),
     S(-69, -44), S(141, -41), S(95, 17), S(160, 9), S(242, -33), S(288, -36), S(165, -47), S(139, -92),
     S(17, -34), S(70, 4), S(68, 43), S(146, 34), S(114, 36), S(191, 5), S(71, 11), S(83, -40),
     S(9, -33), S(58, -12), S(63, 31), S(60, 52), S(94, 27), S(73, 35), S(75, 8), S(24, -40),
     S(-5, -48), S(15, 0), S(60, -5), S(56, 27), S(78, 17), S(73, -9), S(85, -35), S(10, -45),
     S(-22, -76), S(-77, -24), S(14, -13), S(31, -8), S(35, -2), S(71, -37), S(0, -39), S(6, -88),
     S(-193, -25), S(-3, -95), S(-76, -28), S(-35, -20), S(10, -41), S(-21, -32), S(6, -97), S(-3, -116)},
    // Bishop PST
    {S(1, -45), S(26, -49), S(-197, -10), S(-123, -10), S(-112, -3), S(-61, -20), S(-14, -19), S(2, -50),
     S(-13, -33), S(34, -18), S(-33, 2), S(-48, -21), S(68, -23), S(113, -34), S(28, -11), S(-49, -31),
     S(-17, -7), S(76, -24), S(86, -10), S(88, -19), S(82, -16), S(129, -9), S(71, -7), S(11, -1),
     S(9, -16), S(19, 9), S(37, 16), S(115, 2), S(82, 13), S(87, 6), S(11, 1), S(13, -7),
     S(-10, -19), S(36, -5), S(30, 16), S(56, 28), S(73, 5), S(30, 9), S(24, -13), S(14, -26),
     S(6, -29), S(34, -14), S(31, 10), S(38, 10), S(34, 19), S(63, -6), S(42, -19), S(29, -40),
     S(12, -35), S(42, -45), S(49, -25), S(8, -7), S(27, -1), S(51, -25), S(78, -46), S(14, -66),
     S(-65, -41), S(12, -22), S(-16, -51), S(-30, -16), S(-12, -22), S(-10, -42), S(-77, -11), S(-32, -34)},
    // Rook PST
    {S(-2, 64), S(6, 58), S(-31, 75), S(48, 52), S(54, 53), S(-49, 73), S(-22, 62), S(-16, 61),
     S(-19, 63), S(-27, 70), S(37, 55), S(56, 48), S(96, 16), S(115, 25), S(-39, 67), S(20, 47),
     S(-102, 70), S(-50, 63), S(-41, 58), S(-48, 64), S(-85, 61), S(46, 28), S(101, 15), S(-34, 41),
     S(-110, 60), S(-91, 57), S(-78, 75), S(-52, 50), S(-58, 53), S(3, 43), S(-31, 34), S(-92, 61),
     S(-119, 53), S(-110, 58), S(-96, 61), S(-83, 51), S(-56, 34), S(-60, 34), S(-5, 19), S(-83, 28),
     S(-120, 40), S(-94, 45), S(-95, 34), S(-98, 43), S(-60, 26), S(-41, 20), S(-36, 20), S(-84, 13),
     S(-112, 35), S(-83, 32), S(-101, 48), S(-78, 47), S(-61, 27), S(-16, 17), S(-59, 19), S(-154, 47),
     S(-61, 43), S(-68, 55), S(-68, 61), S(-43, 46), S(-37, 40), S(-25, 34), S(-92, 51), S(-58, 8)},
    // Queen PST
    {S(-55, -40), S(-83, 88), S(25, 37), S(7, 40), S(185, -38), S(210, -74), S(86, -29), S(72, 18),
     S(-50, -49), S(-79, 21), S(-16, 47), S(-39, 88), S(-73, 132), S(126, -8), S(60, 33), S(117, -50),
     S(-19, -62), S(-29, -16), S(12, -17), S(-5, 96), S(40, 73), S(126, 22), S(103, -12), S(123, -33),
     S(-68, 16), S(-43, 11), S(-27, 22), S(-31, 60), S(-25, 117), S(25, 57), S(-12, 115), S(-4, 70),
     S(-7, -71), S(-60, 56), S(-10, 4), S(-13, 73), S(-8, 45), S(1, 37), S(4, 63), S(-1, 26),
     S(-30, -33), S(20, -99), S(-14, 4), S(7, -19), S(-6, 1), S(13, 11), S(31, 0), S(15, 2),
     S(-57, -62), S(-4, -66), S(36, -91), S(14, -57), S(29, -56), S(43, -76), S(9, -97), S(25, -105),
     S(-2, -80), S(-17, -88), S(-6, -64), S(32, -118), S(-21, -23), S(-49, -64), S(-50, -61), S(-99, -97)},
    // King PST
    {S(-107, -137), S(332, -121), S(271, -82), S(156, -78), S(-234, 15), S(-121, 41), S(121, -23), S(122, -46),
     S(338, -92), S(68, 21), S(66, 13), S(238, -16), S(116, 4), S(64, 52), S(-103, 56), S(-272, 73),
     S(103, -8), S(136, 10), S(165, 10), S(43, 14), S(96, 12), S(224, 39), S(240, 41), S(4, 12),
     S(-9, -20), S(-53, 44), S(37, 35), S(-63, 55), S(-66, 51), S(-67, 65), S(-6, 49), S(-166, 27),
     S(-116, -33), S(45, -23), S(-92, 47), S(-182, 70), S(-184, 74), S(-136, 60), S(-112, 34), S(-151, -2),
     S(37, -49), S(-7, -11), S(-59, 26), S(-134, 54), S(-123, 58), S(-108, 46), S(-22, 13), S(-62, -10),
     S(34, -64), S(36, -32), S(-22, 12), S(-144, 38), S(-105, 38), S(-53, 18), S(35, -17), S(51, -51),
     S(-14, -113), S(94, -91), S(39, -55), S(-129, -10), S(10, -59), S(-65, -18), S(70, -67), S(66, -114)},
};

const int bishop_pair = S(59, 90);
const int open_file = S(117, 0);
const int semi_open_file = S(39, 20);

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
    int score;
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
    int score;
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

        if (p == PieceType::ROOK)
        {
            Bitboard file = attacks::MASK_FILE[(int)utils::squareFile(sq)];
            if (!(file & (info.pawn[0] | info.pawn[1])))
            {
                score += open_file;
                TraceIncr(open_file);
            }
            else if (!(file & info.pawn[(int)c]))
            {
                score += semi_open_file;
                TraceIncr(semi_open_file);
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

template <Color c>
int eval_king(const Board &board, Trace &trace)
{
    int score;

    Square sq = board.kingSq(c);

    if (c == Color::WHITE)
        sq = sq ^ 56;

    score += pst[(int)PieceType::KING][sq];
    TraceIncr(pst[(int)PieceType::KING][sq]);
    TraceIncr(material[(int)PieceType::KING]);

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

    info.score += eval_king<Color::WHITE>(board, trace);
    info.score -= eval_king<Color::BLACK>(board, trace);
}

Trace evaluate(const Board &board)
{
    Trace trace{};
    init_eval_tables();

    // Check for draw by insufficient material
    // if (board.isInsufficientMaterial())
    //     return trace;

    EvalInfo info;

    eval_pieces(info, board, trace);

    info.gamephase = std::min(info.gamephase, 24);

    trace.score = (mg_score(info.score) * info.gamephase + eg_score(info.score) * (24 - info.gamephase)) / 24;

    trace.score = board.sideToMove() == Color::WHITE ? trace.score : -trace.score;

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
    get_initial_parameter_single(parameters, open_file);
    get_initial_parameter_single(parameters, semi_open_file);

    return parameters;
}

static coefficients_t get_coefficients(const Trace &trace)
{
    coefficients_t coefficients;

    get_coefficient_array(coefficients, trace.material, 6);

    get_coefficient_array_2d(coefficients, trace.pst, 6, 64);

    get_coefficient_single(coefficients, trace.bishop_pair);
    get_coefficient_single(coefficients, trace.open_file);
    get_coefficient_single(coefficients, trace.semi_open_file);

    return coefficients;
}

EvalResult LuxEval::get_fen_eval_result(const std::string &fen)
{
    Board board;
    board.setFen(fen);

    const auto trace = evaluate(board);
    EvalResult result;
    result.coefficients = get_coefficients(trace);
    result.score = trace.score;
    return result;
}

EvalResult LuxEval::get_external_eval_result(const chess::Board &board)
{
    const auto trace = evaluate(board);
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
    ss << "};" << endl
       << endl;
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
    ss << "};\n\n";
}

void LuxEval::print_parameters(const parameters_t &parameters)
{
    int index = 0;
    stringstream ss;
    parameters_t copy = parameters;

    print_array(ss, copy, index, "material", 6);

    print_pst(ss, copy, index, "pst", 6, 64);

    print_single(ss, copy, index, "bishop_pair");
    print_single(ss, copy, index, "open_file");
    print_single(ss, copy, index, "semi_open_file");

    cout << ss.str() << "\n";
}