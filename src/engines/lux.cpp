#include "lux.h"

#include <array>
#include <bit>
#include <cmath>
#include <iostream>
#include <sstream>

using namespace chess;
using namespace Lux;
using namespace std;

struct Trace {
    int score = 0;
    tune_t endgame_scale;

    int material[6][2]{};

    int pst[6][64][2]{};

    int pawn_passed[3][8][2]{};
    int pawn_phalanx[8][2]{};
    int pawn_doubled[2][2]{};
    int pawn_isolated[2]{};
    int pawn_support[2]{};

    int mobility[4][28][2]{};
    int attacked_by_pawn[6][2]{};
    int open_file[2]{};
    int semi_open_file[2]{};
    int bishop_pair[2]{};
    int minor_behind_pawn[2]{};
    int outpost[2]{};

    int king_open[28][2]{};
    int king_att_pawn[2]{};
    int king_shelter[2]{};
};

struct EvalInfo {
    int gamephase = 0;

    Bitboard pawn[2];
    Bitboard pawn_attacks[2];
};

int phase_values[6] = {0, 1, 1, 2, 4, 0};

const int material[6] = {};
int pst[6][64]        = {};

// Pawn Eval
const int pawn_passed[3][8] = {};
const int pawn_phalanx[8]   = {};
const int pawn_doubled[2]   = {};
const int pawn_isolated     = S(0, 0);
const int pawn_support      = S(0, 0);

// Piece Eval
const int mobility[4][28]     = {};
const int attacked_by_pawn[6] = {};
const int open_file           = S(0, 0);
const int semi_open_file      = S(0, 0);
const int bishop_pair         = S(0, 0);
const int minor_behind_pawn   = S(0, 0);
const int outpost             = S(0, 0);

// King Eval
const int king_open[28] = {};
const int king_att_pawn = S(0, 0);
const int king_shelter  = S(0, 0);

// ---------------------------------------------------------------------------------------------------------------------
// Evaluation
// ---------------------------------------------------------------------------------------------------------------------

#define TraceIncr(parameter) trace.parameter[(int)c]++
#define TraceAdd(parameter) trace.parameter[(int)c] += count

Bitboard passed_pawn_mask[2][64], pawn_isolated_mask[64], outpost_mask[2];

void init_eval_tables() {
    for (int i = (int)PieceType::PAWN; i <= (int)PieceType::KING; i++) {
        for (int j = Square::SQ_A1; j <= Square::SQ_H8; j++) {
            pst[i][j] += material[i];
        }
    }

    outpost_mask[0] = attacks::MASK_RANK[4] | attacks::MASK_RANK[5] | attacks::MASK_RANK[6] | attacks::MASK_RANK[7];
    outpost_mask[1] = attacks::MASK_RANK[0] | attacks::MASK_RANK[1] | attacks::MASK_RANK[2] | attacks::MASK_RANK[3];

    for (int i = Square::SQ_A1; i <= Square::SQ_H8; i++) {
        Bitboard file = attacks::MASK_FILE[(int)utils::squareFile(Square(i))];

        pawn_isolated_mask[i] = attacks::shift<Direction::EAST>(file) | attacks::shift<Direction::WEST>(file);

        passed_pawn_mask[0][i] = file | attacks::shift<Direction::EAST>(file) | attacks::shift<Direction::WEST>(file);
        passed_pawn_mask[1][i] = passed_pawn_mask[0][i];

        while (passed_pawn_mask[0][i] & (1ULL << i)) {
            passed_pawn_mask[0][i] = attacks::shift<Direction::NORTH>(passed_pawn_mask[0][i]);
        }
        while (passed_pawn_mask[1][i] & (1ULL << i)) {
            passed_pawn_mask[1][i] = attacks::shift<Direction::SOUTH>(passed_pawn_mask[1][i]);
        }
    }
}

// Get relative position of square based on color
template <Color c>
Square black_relative_square(Square &sq) {
    return (c == Color::WHITE) ? sq ^ 56 : sq;
}
template <Color c>
Square white_relative_square(Square &sq) {
    return (c == Color::BLACK) ? sq ^ 56 : sq;
}

// Get a bitboard with all pawn attacks based on pawn location and movement direction
template <Color c>
Bitboard get_pawn_attacks(Bitboard &pawns) {
    return attacks::pawnLeftAttacks<c>(pawns) | attacks::pawnRightAttacks<c>(pawns);
}

template <Color c>
int eval_pawn(EvalInfo &info, const Board &board, Trace &trace) {
    int score = 0, count = 0;
    Bitboard bb = board.pieces(PieceType::PAWN, c);

    // Init useful bitboards
    Bitboard phalanx_pawns    = bb & attacks::shift<Direction::WEST>(bb);
    Bitboard pawn_attacks     = get_pawn_attacks<c>(bb);
    info.pawn_attacks[(int)c] = pawn_attacks;

    // Penalty for doubled pawns
    count = builtin::popcount(bb & attacks::shift<Direction::NORTH>(bb));
    score += pawn_doubled[0] * count;
    TraceAdd(pawn_doubled[0]);

    count = builtin::popcount(bb & attacks::shift<Direction::NORTH>(attacks::shift<Direction::NORTH>(bb)));
    score += pawn_doubled[1] * count;
    TraceAdd(pawn_doubled[1]);

    // Bonus for pawns protecting pawns
    count = builtin::popcount(bb & get_pawn_attacks<~c>(bb));
    score += pawn_support * count;
    TraceAdd(pawn_support);

    // Bonus for phalanx pawns
    while (phalanx_pawns) {
        Square sq = builtin::poplsb(phalanx_pawns);

        score += pawn_phalanx[white_relative_square<c>(sq) >> 3];
        TraceIncr(pawn_phalanx[white_relative_square<c>(sq) >> 3]);
    }

    while (bb) {
        Square sq = builtin::poplsb(bb);

        // Penalty is pawn is isolated
        if (!(pawn_isolated_mask[sq] & info.pawn[(int)c])) {
            score += pawn_isolated;
            TraceIncr(pawn_isolated);
        }

        // Bonus if pawn is passed
        if (!(passed_pawn_mask[(int)c][sq] & info.pawn[(int)~c])) {
            int protectors = builtin::popcount(info.pawn[(int)c] & attacks::pawn(~c, sq));

            // Bonus if passed pawn is protected by pawn
            score += pawn_passed[protectors][white_relative_square<c>(sq) >> 3];
            TraceIncr(pawn_passed[protectors][white_relative_square<c>(sq) >> 3]);
        }

        score += pst[0][black_relative_square<c>(sq)];
        TraceIncr(pst[0][black_relative_square<c>(sq)]);
        TraceIncr(material[0]);
    }

    return score;
}

template <Color c, PieceType p>
int eval_piece(EvalInfo &info, const Board &board, Trace &trace) {
    int score = 0, count = 0;
    Bitboard bb = board.pieces(p, c);

    // Init useful directions
    const Direction UP   = c == Color::WHITE ? Direction::NORTH : Direction::SOUTH;
    const Direction DOWN = c == Color::WHITE ? Direction::SOUTH : Direction::NORTH;

    // Increase gamephase
    info.gamephase += phase_values[(int)p] * builtin::popcount(bb);

    // Bishop pair bonus
    if (p == PieceType::BISHOP && (bb & (bb - 1))) {
        score += bishop_pair;
        TraceIncr(bishop_pair);
    }

    // Bonus for minor piece behind pawn
    if (p == PieceType::KNIGHT || p == PieceType::BISHOP) {
        count = builtin::popcount(bb & attacks::shift<DOWN>(info.pawn[(int)c]));
        score += minor_behind_pawn * count;
        TraceAdd(minor_behind_pawn);

        // Bonus for knight/bishop outposts
        count = builtin::popcount(bb & info.pawn_attacks[(int)c] & outpost_mask[(int)c]);
        score += outpost * count;
        TraceAdd(outpost);
    }

    while (bb) {
        Square sq = builtin::poplsb(bb);

        // Open and Semi-Open file bonus
        if (p == PieceType::ROOK) {
            Bitboard file = attacks::MASK_FILE[(int)utils::squareFile(sq)];
            if (!(file & info.pawn[(int)c])) {
                if (!(file & info.pawn[(int)~c])) {
                    score += open_file;
                    TraceIncr(open_file);
                } else {
                    score += semi_open_file;
                    TraceIncr(semi_open_file);
                }
            }
        }

        // Penalty is piece is attacked by pawn
        if (info.pawn[(int)~c] & attacks::pawn(c, sq)) {
            score += attacked_by_pawn[(int)p];
            TraceIncr(attacked_by_pawn[(int)p]);
        }

        // Get move bb for piece
        Bitboard moves = 0;
        if (p == PieceType::KNIGHT)
            moves = attacks::knight(sq);
        else if (p == PieceType::BISHOP)
            moves = attacks::bishop(sq, board.occ());
        else if (p == PieceType::ROOK)
            moves = attacks::rook(sq, board.occ());
        else if (p == PieceType::QUEEN)
            moves = attacks::queen(sq, board.occ());

        // Bonus/penalty for number of moves per piece
        score += mobility[(int)p - 1][builtin::popcount(moves & ~board.us(c) & ~info.pawn_attacks[(int)~c])];
        TraceIncr(mobility[(int)p - 1][builtin::popcount(moves & ~board.us(c) & ~info.pawn_attacks[(int)~c])]);

        score += pst[(int)p][black_relative_square<c>(sq)];
        TraceIncr(pst[(int)p][black_relative_square<c>(sq)]);
        TraceIncr(material[(int)p]);
    }

    return score;
}

template <Color c>
int eval_king(EvalInfo &info, const Board &board, Trace &trace) {
    int score = 0, count = 0;
    Bitboard bb    = board.pieces(PieceType::KING, c);
    Square king_sq = builtin::poplsb(bb);

    // Init useful bitboards
    Bitboard moves = attacks::queen(king_sq, board.occ());

    // Bonus/penalty for number of squares king could be attacked from
    score += king_open[builtin::popcount(moves & ~board.us(c) & ~info.pawn_attacks[(int)~c])];
    TraceIncr(king_open[builtin::popcount(moves & ~board.us(c) & ~info.pawn_attacks[(int)~c])]);

    // Bonus/penalty if king threatens enemy pawn
    if (attacks::king(king_sq) & info.pawn[(int)~c]) {
        score += king_att_pawn;
        TraceIncr(king_att_pawn);
    }

    // Bonus for pawn sheltering king
    count = builtin::popcount(passed_pawn_mask[(int)c][king_sq] & info.pawn[(int)c]);
    score += king_shelter * count;
    TraceAdd(king_shelter);

    score += pst[5][black_relative_square<c>(king_sq)];
    TraceIncr(pst[5][black_relative_square<c>(king_sq)]);

    return score;
}

int eval_pieces(EvalInfo &info, const Board &board, Trace &trace) {
    return eval_pawn<Color::WHITE>(info, board, trace) - eval_pawn<Color::BLACK>(info, board, trace) +

           eval_piece<Color::WHITE, PieceType::KNIGHT>(info, board, trace) +
           eval_piece<Color::WHITE, PieceType::BISHOP>(info, board, trace) +
           eval_piece<Color::WHITE, PieceType::ROOK>(info, board, trace) +
           eval_piece<Color::WHITE, PieceType::QUEEN>(info, board, trace) -

           eval_piece<Color::BLACK, PieceType::KNIGHT>(info, board, trace) -
           eval_piece<Color::BLACK, PieceType::BISHOP>(info, board, trace) -
           eval_piece<Color::BLACK, PieceType::ROOK>(info, board, trace) -
           eval_piece<Color::BLACK, PieceType::QUEEN>(info, board, trace) +

           eval_king<Color::WHITE>(info, board, trace) - eval_king<Color::BLACK>(info, board, trace);
}

void init_eval_info(EvalInfo &info, const Board &board) {
    // Add pawn bb to eval info
    info.pawn[0]         = board.pieces(PieceType::PAWN, Color::WHITE);
    info.pawn[1]         = board.pieces(PieceType::PAWN, Color::BLACK);
    info.pawn_attacks[0] = get_pawn_attacks<Color::WHITE>(info.pawn[0]);
    info.pawn_attacks[1] = get_pawn_attacks<Color::BLACK>(info.pawn[1]);
}

double endgame_scale(EvalInfo &info, int score) {
    // Divide the endgame score if the stronger side doesn't have many pawns left
    int num_missing_stronger_pawns = 8 - builtin::popcount(info.pawn[score < 0]);
    return (128 - num_missing_stronger_pawns * num_missing_stronger_pawns) / 128.0;
}

int evaluate(const Board &board, Trace &trace) {
    // Check for draw by insufficient material
    if (board.isInsufficientMaterial()) return 0;

    EvalInfo info;

    init_eval_info(info, board);

    int score = eval_pieces(info, board, trace);

    trace.endgame_scale = endgame_scale(info, score);

    int gamephase = std::min(info.gamephase, 24);

    score = (mg_score(score) * gamephase + eg_score(score) * (24 - gamephase) * endgame_scale(info, score)) / 24;

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

    get_initial_parameter_array_2d(parameters, pawn_passed, 3, 8);
    get_initial_parameter_array(parameters, pawn_phalanx, 8);
    get_initial_parameter_array(parameters, pawn_doubled, 2);
    get_initial_parameter_single(parameters, pawn_isolated);
    get_initial_parameter_single(parameters, pawn_support);

    get_initial_parameter_array_2d(parameters, mobility, 4, 28);
    get_initial_parameter_array(parameters, attacked_by_pawn, 6);
    get_initial_parameter_single(parameters, open_file);
    get_initial_parameter_single(parameters, semi_open_file);
    get_initial_parameter_single(parameters, bishop_pair);
    get_initial_parameter_single(parameters, minor_behind_pawn);
    get_initial_parameter_single(parameters, outpost);

    get_initial_parameter_array(parameters, king_open, 28);
    get_initial_parameter_single(parameters, king_att_pawn);
    get_initial_parameter_single(parameters, king_shelter);

    return parameters;
}

static coefficients_t get_coefficients(const Trace &trace) {
    coefficients_t coefficients;

    get_coefficient_array(coefficients, trace.material, 6);

    get_coefficient_array_2d(coefficients, trace.pst, 6, 64);

    get_coefficient_array_2d(coefficients, trace.pawn_passed, 3, 8);
    get_coefficient_array(coefficients, trace.pawn_phalanx, 8);
    get_coefficient_array(coefficients, trace.pawn_doubled, 2);
    get_coefficient_single(coefficients, trace.pawn_isolated);
    get_coefficient_single(coefficients, trace.pawn_support);

    get_coefficient_array_2d(coefficients, trace.mobility, 4, 28);
    get_coefficient_array(coefficients, trace.attacked_by_pawn, 6);
    get_coefficient_single(coefficients, trace.open_file);
    get_coefficient_single(coefficients, trace.semi_open_file);
    get_coefficient_single(coefficients, trace.bishop_pair);
    get_coefficient_single(coefficients, trace.minor_behind_pawn);
    get_coefficient_single(coefficients, trace.outpost);

    get_coefficient_array(coefficients, trace.king_open, 28);
    get_coefficient_single(coefficients, trace.king_att_pawn);
    get_coefficient_single(coefficients, trace.king_shelter);

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

static void print_mobility(std::stringstream &ss, const parameters_t &parameters, int &index, const std::string &name) {
    string names[4] = {"Knight", "Bishop", "Rook", "Queen"};
    int moves[4]    = {8, 13, 14, 27};

    ss << "int " << name << "[4][28] = {\n";
    for (auto i = 0; i < 4; i++) {
        ss << "// " << names[i] << " (0-" << moves[i] << ")\n";
        ss << "{";
        for (auto j = 0; j < 28; j++) {
            if (j <= moves[i]) {
                print_parameter(ss, parameters[index]);

                if (j == moves[i]) ss << "}";

                ss << ", ";
            }

            index++;
        }
        ss << '\n';
    }
    ss << "};\n";
}

static void normalize_2d(parameters_t &parameters, int index, int count1, int count2, int offset) {
    for (auto i = 0; i < count1; i++) {
        tune_t sum0 = 0, sum1 = 0;
        int cnt0 = 0, cnt1 = 0;

        for (auto j = 0; j < count2; j++) {
            int parameter_index = index + count2 * i + j;
            if (parameters[parameter_index][0] != 0 && parameters[parameter_index][1] != 0) {
                sum0 += parameters[parameter_index][0];
                cnt0++;

                sum1 += parameters[parameter_index][1];
                cnt1++;
            }
        }

        if (i + offset < 5) {
            if (cnt0 > 0) parameters[i + offset][0] += sum0 / cnt0;
            if (cnt1 > 0) parameters[i + offset][1] += sum1 / cnt1;
        }

        for (auto j = 0; j < count2; j++) {
            int parameter_index = index + count2 * i + j;
            if (parameters[parameter_index][0] != 0 && parameters[parameter_index][1] != 0) {
                parameters[parameter_index][0] -= sum0 / cnt0;
                parameters[parameter_index][1] -= sum1 / cnt1;
            }
        }
    }
}

void LuxEval::print_parameters(const parameters_t &parameters) {
    int index = 0;
    stringstream ss;
    parameters_t copy = parameters;

    normalize_2d(copy, 6, 6, 64, 0);
    normalize_2d(copy, 6 + 6 * 64 + 3 * 8 + 8 + 2 + 1 + 1, 4, 28, 1);

    print_array(ss, copy, index, "material", 6);
    ss << endl;

    print_pst(ss, copy, index, "pst", 6, 64);
    ss << endl;

    ss << "// Pawn Eval" << endl;
    print_array_2d(ss, copy, index, "pawn_passed", 3, 8);
    print_array(ss, copy, index, "pawn_phalanx", 8);
    print_array(ss, copy, index, "pawn_doubled", 2);
    print_single(ss, copy, index, "pawn_isolated");
    print_single(ss, copy, index, "pawn_support");
    ss << endl;

    ss << "// Piece Eval" << endl;
    print_mobility(ss, copy, index, "mobility");
    print_array(ss, copy, index, "attacked_by_pawn", 6);
    print_single(ss, copy, index, "open_file");
    print_single(ss, copy, index, "semi_open_file");
    print_single(ss, copy, index, "bishop_pair");
    print_single(ss, copy, index, "minor_behind_pawn");
    print_single(ss, copy, index, "outpost");
    ss << endl;

    ss << "// King Eval" << endl;
    print_array(ss, copy, index, "king_open", 28);
    print_single(ss, copy, index, "king_att_pawn");
    print_single(ss, copy, index, "king_shelter");
    ss << endl;

    std::cout << ss.str() << "\n";
}