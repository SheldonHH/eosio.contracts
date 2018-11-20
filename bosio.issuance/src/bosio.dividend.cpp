#include "../include/bosio.issuance.hpp"
#include "bosio.helper.cpp"

using namespace eosio;
namespace bosio {
    // 这里不更新dailybids或则dailyorders，仍保持原样，用ava
    void bosio_issuance::calculatedividend(uint64_t today, uint64_t divdsymraw) {
        uint64_t first_divday; // 一级分红
        uint64_t second_divday; // 二级分红
        asset dividendamt;
        if (91 <= today <= 300) {
            first_divday = today - 90;
            if(today >= 270){
                second_divday = today - 180;
            }
        } else if (today > 300) {
            first_divday = today - 30;
            second_divday = today - 120;
        }
        auto firstday = _dailybids.get(index_encode(first_divday, divdsymraw));
        dividendamt = firstday.masset * 0.5 * 0.5;

        if(second_divday > 0){
            auto secday = _dailybids.get(index_encode(second_divday, divdsymraw));
            dividendamt += secday.masset * 0.25 * 0.5;
        }

        INLINE_ACTION_SENDER(eosio::token, transfer)(
                bostoken_account, {{_self, active_permission}},
                {_self, dividend_account, dividendamt, std::string("send dividend BOS on day x")}
        );
    }
}