#ifndef LUX_H
#define LUX_H 1

#define TAPERED 1

#include <string>
#include <vector>

#include "../base.h"
#include "../external/chess.hpp"

namespace Lux {

class LuxEval {
   public:
    constexpr static bool includes_additional_score      = false;
    constexpr static bool supports_external_chess_eval   = true;
    constexpr static bool retune_from_zero               = true;
    constexpr static tune_t preferred_k                  = 3.56494;
    constexpr static int32_t max_epoch                   = 2000;
    constexpr static bool enable_qsearch                 = true;
    constexpr static bool filter_in_check                = false;
    constexpr static tune_t initial_learning_rate        = 1;
    constexpr static int32_t learning_rate_drop_interval = 10000;
    constexpr static tune_t learning_rate_drop_ratio     = 1;
    constexpr static bool print_data_entries             = false;
    constexpr static int32_t data_load_print_interval    = 100000;

    static parameters_t get_initial_parameters();
    static EvalResult get_fen_eval_result(const std::string &fen);
    static EvalResult get_external_eval_result(const chess::Board &board);
    static void print_parameters(const parameters_t &parameters);
};

}  // namespace Lux

#endif  // LUX_H
