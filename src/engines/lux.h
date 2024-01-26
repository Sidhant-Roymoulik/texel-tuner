#ifndef LUX_H
#define LUX_H 1

#include "../base.h"
#include "../external/chess.hpp"
#include <string>
#include <vector>

namespace Lux
{
    class LuxEval
    {
    public:
        constexpr static bool includes_additional_score = false;
        constexpr static bool supports_external_chess_eval = true;

        static parameters_t get_initial_parameters();
        static EvalResult get_fen_eval_result(const std::string &fen);
        static EvalResult get_external_eval_result(const chess::Board &board);
        static void print_parameters(const parameters_t &parameters);
    };
}

#endif // LUX_H
