#include "../include/bosio.issuance.hpp"
#include "bosio.helper.cpp"

using namespace eosio;
namespace bosio {
    // available refund percentage
    uint64_t bosio_issuance::get_bought_period(uint64_t day) {
        if (day <= 90) {
            return 1;
        } else if (90 < day <= 180) {
            return 2;
        } else if (180 < day <= 270) {
            return 3;
        } else if (270 < day <= 300) {
            return 4;
        } else {
            eosio::print("bidding has ended");
            return 5;
        }
    }

    double bosio_issuance::avarefundpert(uint64_t symday) {
        std::tuple <uint64_t, uint64_t> tu_symday = index_decode(symday);
        uint64_t boughtDay = std::get<1>(tu_symday);

        uint64_t boughtPeriod = get_bought_period(boughtDay);
        uint64_t bought_elapseday =  _istate.curday - boughtDay;
        switch (boughtPeriod) {
            case 1:
                if (1 <= bought_elapseday <= 90) {
                    return 1;
                } else if (91 < bought_elapseday <= 180) {
                    return 0.5;
                } else if (181 < bought_elapseday <= 270) {
                    return 0.25;
                }
                break;
            case 2:
                if (1 <= bought_elapseday <= 60) {
                    return 1;
                } else if (61 < bought_elapseday <= 120) {
                    return 0.5;
                } else if (121 < bought_elapseday <= 180) {
                    return 0.25;
                }
                break;
            case 3:
                if (1 <= bought_elapseday <= 30) {
                    return 1;
                } else if (31 < bought_elapseday <= 60) {
                    return 0.5;
                } else if (61 < bought_elapseday <= 90) {
                    return 0.25;
                }
                break;
            case 4:
                if (1 <= bought_elapseday <= 20) {
                    return 1;
                } else if (21 < bought_elapseday <= 40) {
                    return 0.5;
                } else if (41 < bought_elapseday <= 60) {
                    return 0.25;
                }
                break;
        }

    }

    // refundtoken and modify userorders and dailyorders table [bosrefund 代表refund的bos数量]
    void bosio_issuance::refundtoken(name owner, uint64_t symday, asset bosrefund) {
        eosio::print(bosrefund);
        asset tokenrefund;
        userorders_table
        _userorders(_self, owner.value);
        auto userorders_lookup = _userorders.find(symday);
        if (userorders_lookup != _userorders.end()) {
            asset ava_refund = userorders_lookup->bosasset * avarefundpert(symday);
            bool availability = bosrefund > ava_refund;
            eosio_assert(availability, "in userorders, overdrawn available refund");

            //TODO: fix the type conversion
            //TODO: decide how to calculate the price_of_bos based on userorder or userbid
            // 在userbids中找打对于一个token当时user是分得多少bos
            double price_of_bos = userorders_lookup->bosasset / userorders_lookup->masset;
            tokenrefund = bosrefund / price_of_bos;
            INLINE_ACTION_SENDER(eosio::token, transfer)(
                    bostoken_account, {{_self, active_permission}},
                    {_self, owner, tokenrefund, std::string("send bos to refund to receiver on day x")}
            );

            _userorders.modify(userorders_lookup, _self, [&](auto &update_user_order) {
                update_user_order.bosasset = update_user_order.bosasset - bosrefund;
                update_user_order.masset = update_user_order.masset - tokenrefund;

            });
        }

        // modify dailyorder table
        auto dailyorder_lookup = _dailyorders.find(symday);
        if (dailyorder_lookup != _dailyorders.end()) {
            _dailyorders.modify(dailyorder_lookup, _self, [&](auto &update_daily_order) {
                update_daily_order.bosasset = update_daily_order.bosasset - bosrefund;
                update_daily_order.masset = update_daily_order.masset - tokenrefund;
            });
        }

    }
}