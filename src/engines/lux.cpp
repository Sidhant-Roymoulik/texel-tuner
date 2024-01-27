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

const int material[6] = {S(134, 155), S(475, 308), S(454, 305), S(532, 494), S(1035, 939), S(0, 0)};

int pst[6][64] = {
    // Pawn PST
    {S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
     S(57, 435), S(188, 377), S(20, 356), S(130, 296), S(83, 329), S(178, 283), S(-14, 389), S(-139, 438),
     S(-46, 149), S(-33, 157), S(17, 124), S(37, 86), S(111, 53), S(102, 43), S(5, 119), S(-64, 126),
     S(-64, 28), S(-15, 5), S(-22, -18), S(11, -39), S(14, -53), S(-4, -41), S(5, -17), S(-72, -10),
     S(-89, -8), S(-41, -23), S(-49, -47), S(-8, -61), S(2, -64), S(-9, -67), S(-4, -45), S(-78, -45),
     S(-85, -27), S(-46, -27), S(-47, -55), S(-56, -39), S(-27, -47), S(-17, -58), S(39, -57), S(-54, -61),
     S(-105, -6), S(-39, -25), S(-78, -23), S(-81, -23), S(-66, -21), S(24, -51), S(46, -49), S(-72, -58),
     S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0)},
    // Knight PST
    {S(-305, -79), S(-188, -53), S(-71, -16), S(-33, -56), S(174, -74), S(-177, -42), S(-12, -115), S(-191, -153),
     S(-135, -37), S(-65, -5), S(196, -64), S(78, -2), S(89, -27), S(147, -54), S(19, -42), S(-6, -95),
     S(-75, -42), S(147, -43), S(96, 17), S(166, 7), S(250, -35), S(281, -33), S(164, -46), S(146, -95),
     S(8, -31), S(71, 4), S(74, 40), S(148, 35), S(116, 36), S(195, 6), S(74, 9), S(90, -44),
     S(10, -33), S(54, -10), S(63, 32), S(61, 52), S(94, 28), S(75, 35), S(75, 10), S(23, -40),
     S(-3, -48), S(16, -1), S(61, -5), S(57, 27), S(77, 17), S(74, -8), S(86, -36), S(18, -45),
     S(-25, -73), S(-78, -25), S(13, -13), S(30, -9), S(34, -2), S(71, -38), S(0, -39), S(5, -86),
     S(-207, -24), S(-2, -96), S(-76, -30), S(-33, -21), S(9, -41), S(-22, -32), S(6, -96), S(1, -115)},
    // Bishop PST
    {S(-17, -45), S(27, -47), S(-206, -9), S(-125, -8), S(-101, -6), S(-42, -26), S(-5, -23), S(3, -53),
     S(-6, -36), S(37, -18), S(-31, 2), S(-53, -17), S(81, -28), S(109, -31), S(28, -12), S(-68, -25),
     S(-24, -4), S(75, -24), S(92, -10), S(88, -18), S(81, -15), S(138, -12), S(74, -7), S(9, 0),
     S(11, -15), S(12, 12), S(35, 16), S(123, 3), S(81, 17), S(89, 7), S(11, 1), S(13, -5),
     S(-28, -14), S(41, -5), S(30, 18), S(53, 31), S(72, 8), S(30, 11), S(25, -11), S(15, -26),
     S(6, -28), S(32, -12), S(30, 12), S(38, 11), S(32, 20), S(67, -6), S(40, -16), S(28, -37),
     S(12, -38), S(40, -43), S(47, -22), S(6, -6), S(26, 0), S(53, -25), S(77, -44), S(12, -66),
     S(-66, -40), S(10, -21), S(-18, -51), S(-29, -15), S(-15, -21), S(-12, -42), S(-80, -11), S(-35, -31)},
    // Rook PST
    {S(-21, 73), S(12, 59), S(-21, 76), S(45, 57), S(63, 55), S(-42, 75), S(-46, 72), S(-26, 67),
     S(-19, 65), S(-29, 74), S(36, 58), S(47, 54), S(106, 15), S(117, 25), S(-36, 69), S(21, 50),
     S(-104, 73), S(-50, 68), S(-35, 59), S(-60, 69), S(-80, 62), S(51, 30), S(92, 21), S(-41, 45),
     S(-115, 64), S(-93, 62), S(-82, 80), S(-60, 56), S(-57, 55), S(-6, 49), S(-39, 38), S(-98, 66),
     S(-125, 59), S(-122, 66), S(-104, 66), S(-82, 54), S(-64, 39), S(-71, 40), S(-19, 25), S(-88, 31),
     S(-126, 45), S(-101, 48), S(-99, 37), S(-105, 47), S(-70, 32), S(-47, 23), S(-40, 23), S(-91, 19),
     S(-117, 40), S(-87, 36), S(-107, 53), S(-83, 51), S(-68, 32), S(-24, 24), S(-58, 23), S(-160, 53),
     S(-66, 48), S(-71, 58), S(-74, 67), S(-48, 52), S(-43, 45), S(-30, 40), S(-98, 57), S(-63, 13)},
    // Queen PST
    {S(-97, -9), S(-97, 102), S(3, 58), S(-9, 54), S(176, -29), S(219, -75), S(15, 18), S(33, 47),
     S(-72, -33), S(-88, 32), S(-19, 58), S(-51, 100), S(-81, 144), S(117, 2), S(59, 40), S(100, -36),
     S(-21, -64), S(-36, -2), S(1, -2), S(-14, 112), S(41, 80), S(144, 22), S(97, 2), S(110, -37),
     S(-81, 27), S(-57, 27), S(-33, 31), S(-43, 81), S(-31, 129), S(19, 69), S(-27, 135), S(-15, 75),
     S(-16, -61), S(-72, 71), S(-19, 14), S(-22, 84), S(-16, 54), S(-8, 50), S(-5, 68), S(-13, 35),
     S(-36, -29), S(10, -92), S(-28, 18), S(-3, -5), S(-19, 15), S(-6, 28), S(20, 14), S(3, 7),
     S(-68, -55), S(-19, -48), S(26, -86), S(3, -47), S(16, -45), S(33, -69), S(3, -90), S(15, -98),
     S(-11, -75), S(-27, -81), S(-19, -51), S(23, -117), S(-32, -14), S(-61, -53), S(-54, -53), S(-108, -92)},
    // King PST
    {S(-121, -134), S(287, -112), S(271, -79), S(169, -80), S(-232, 15), S(-135, 45), S(121, -23), S(142, -51),
     S(329, -93), S(70, 18), S(59, 17), S(240, -15), S(125, 3), S(58, 55), S(-86, 50), S(-278, 76),
     S(109, -8), S(132, 13), S(164, 13), S(30, 18), S(103, 13), S(231, 40), S(254, 42), S(-3, 14),
     S(-23, -17), S(-55, 46), S(49, 33), S(-65, 57), S(-63, 51), S(-60, 65), S(-9, 50), S(-161, 27),
     S(-111, -32), S(53, -24), S(-93, 48), S(-182, 72), S(-186, 76), S(-141, 63), S(-114, 36), S(-148, -1),
     S(41, -49), S(-4, -10), S(-58, 27), S(-134, 56), S(-124, 60), S(-108, 48), S(-22, 14), S(-60, -9),
     S(38, -63), S(36, -29), S(-19, 12), S(-141, 39), S(-105, 40), S(-50, 19), S(35, -16), S(54, -50),
     S(-13, -112), S(97, -92), S(39, -54), S(-126, -10), S(13, -59), S(-65, -17), S(72, -66), S(68, -114)},
};

const int bishop_pair = S(57, 89);
const int open_file = S(121, -1);
const int semi_open_file = S(38, 21);

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