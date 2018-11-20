#include "../include/bosio.issuance.hpp"
#include "bosio.helper.cpp"
#include <stdio.h>      /* printf, scanf */
#include <time.h>       /* time_t, struct tm, time, mktime */
#include <unistd.h>
#include <eosiolib/time.hpp>
#include "bosio.bidding.cpp"
#include "bosio.refund.cpp"
#include "bosio.dividend.cpp"
#include "bosio.reward.cpp"
#include "bosio.helper.cpp"

//namespace bosio {
using namespace eosio;
namespace bosio {
    // 也可使用非上链的方式，每隔0.5秒用nodeos trigger该函数
    // 12.12: 1544601600
    // 12.24: 1545638400
    // 12.25: 1545724800
    // 1.1:   1546329600

    bosio_issuance::issuestate bosio_issuance::get_default_param() { //TODO：可否在hpp文件中指定
        _issuance.set(issuestate{1, 1545724800, 86400, 1571644800, 43200}, issuance_account);
        return _istate;
    }

    void bosio_issuance::timechecking() {
        uint64_t i;
        uint64_t now = current_time() / 1000000;
        eosio::print("now: ", now);
        uint64_t countforday = 1;
//         _issuance.set(issuestate{countforday}, issuance_account); //uncomment this line while re-testing
        eosio::print(" _istate.curday:", _istate.curday);
        for (i = startbidtime; i <= endbidtime; i += oneday) {
            if (i > now) {
                if (_istate.curday != countforday) {
                    eosio::print("set new day");
                    _issuance.set(issuestate{countforday}, issuance_account);
                    // 第一天init不calculatepwd
                    foreach_token(countforday - 1, "calculatebid");
                    if (countforday >= 91) {
                        foreach_token(countforday, "calculatedividend");
                    }
                } else {
                    eosio::print("i: ", i);
                    eosio::print("same day, no calculation nor distribution");
                }
                return;
            }
            countforday++;
        }
        eosio::print("**i**", i);
        eosio::print("**now**", now);
        if (i < now) {
            eosio::print("bos bidding has ended");
            _issuance.set(issuestate{301}, issuance_account);
            foreach_token(301, "reward_token");
        }
        eosio::print("_istate: ", _istate.curday);
    }

    void bosio_issuance::init(name owner) {
        require_auth(owner);
        eosio::print("My bos account: ", owner);
        _issuance.set(issuestate{1, }, issuance_account);
    }

    // 设置：起始竞买时间，每一天是多长，和refund距上一天竞买需要多少秒
    void bosio_issuance::set_issuance_params(uint64_t startbidtime, uint64_t oneday, uint64_t refundbuffertime) {
        _issuance.set(issuestate{_istate.curday, startbidtime, oneday, startbidtime+300*oneday, refundbuffertime}, issuance_account);
    }

    void bosio_issuance::foreach_token(uint64_t day, string option) {
        if (option == "calculatebid") {
            calculatebid(day, symbol_code("EOS").raw());
            calculatebid(day, symbol_code("BTC").raw());
            calculatebid(day, symbol_code("ETH").raw());
        } else if (option == "reward_token") {
            reward_token(symbol_code("EOS").raw());
            reward_token(symbol_code("BTC").raw());
            reward_token(symbol_code("ETH").raw());
        } else if (option == "calculatedividend") {
            calculatedividend(day, symbol_code("EOS").raw());
            calculatedividend(day, symbol_code("BTC").raw());
            calculatedividend(day, symbol_code("ETH").raw());
        }
    }

    void bosio_issuance::transfer(const name sender, const name receiver) {
        array<char, 33> owner_pubkey_char;
        array<char, 33> active_pubkey_char;
        const auto transfer = unpack_action_data<bosio_issuance::transferstruct>();
        if (transfer.from == _self || transfer.to != _self) {
            // this is an outgoing transfer, do nothing
            return;
        }
        eosio_assert(transfer.quantity.symbol == symbol("BOS", 4) || transfer.quantity.symbol == symbol("EOS", 4) ||
                     transfer.quantity.symbol == symbol("ETH", 8) ||
                     transfer.quantity.symbol == symbol("BTC", 8),
                     "Must be BOS or EOS or ETH or BTC");

        eosio_assert(transfer.quantity.is_valid(), "Invalid token transfer");
        eosio_assert(transfer.quantity.amount > 0, "Quantity must be positive");

        uint64_t symday;
        uint64_t today = _istate.curday;
        if (transfer.quantity.symbol == symbol("BOS", 4)) {
            symday = index_encode(symbol("BOS", 4).raw(), today);
            bosio_issuance::bidbos(sender, symday, transfer.quantity);
        }else if(validaterefundmemo(transfer.memo)){
                symday = index_encode(transfer.quantity.symbol.raw(), today);
                refundtoken(sender, symday, transfer.quantity);
        }

    };


    bool bosio_issuance::validaterefundmemo(string memo) {
        bool memoformat = true;
        uint64_t biday;
        if (memoformat) {
            uint64_t endbidtimeof_biday = biday * oneday + startbidtime;
            uint64_t now = current_time() / 1000000;
            if(now > endbidtimeof_biday + refundbuffertime){
                return true;
            }
        }
        return false;
    }


    void bosio_issuance::printhello() {
        eosio::print("singleton_example");
    }

    void bosio_issuance::notify(name user, std::string msg) {
//        require_auth(get_self());
        require_recipient(user);
    }


    void send_summary(name user, std::string message) {
//        action(
//                permission_level{_self(),"active"_n},
//                _self(),
//                "notify"_n,
//                std::make_tuple(user, name{user}.to_string() + message)
//        ).send();
    };

#define EOSIO_DISPATCH_EX(TYPE, MEMBERS)                                            \
  extern "C" {                                                                 \
  void apply(uint64_t receiver, uint64_t code, uint64_t action) {              \
    if (action == "onerror"_n.value) {                                                \
      /* onerror is only valid if it is for the "eosio" code account and       \
       * authorized by "eosio"'s "active permission */                         \
      eosio_assert(code == "eosio"_n.value, "onerror action's are only valid from "   \
                                     "the \"eosio\" system account");          \
    }                                                                          \
    auto self = receiver;                                                      \
    if (code == receiver || code == "eosio.token"_n.value || code == "bosbidding"_n.value || action == "onerror"_n.value  ) {      \
      switch (action) { EOSIO_DISPATCH_HELPER(TYPE, MEMBERS) }                             \
      /* does not allow destructor of thiscontract to run: eosio_exit(0); */   \
    }                                                                          \
  }                                                                            \
  }
    EOSIO_DISPATCH_EX(bosio_issuance, (transfer)(init)(timechecking)(addtoq)(processq)(processxfer))
}