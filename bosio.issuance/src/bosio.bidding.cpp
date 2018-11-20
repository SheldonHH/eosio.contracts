#include "../include/bosio.issuance.hpp"
#include "bosio.helper.cpp"
#include <stdio.h>      /* printf, scanf */
#include <time.h>       /* time_t, struct tm, time, mktime */
#include <unistd.h>
#include <eosiolib/time.hpp>

//namespace bosio {
using namespace eosio;
namespace bosio {
    void bosio_issuance::bidbos(const name owner, uint64_t symday, asset mappedasset) {
        require_auth(owner);
        userbids_table _userbids(_self, owner.value); // 建立user scope
        auto userbid_lookup = _userbids.find(symday);
        if (userbid_lookup == _userbids.end()) {
            _userbids.emplace(_self, [&](auto &new_userbid) { // contract账户为user account付款
                new_userbid.symday = symday;
                new_userbid.masset = mappedasset;
            });
        } else {
            _userbids.modify(userbid_lookup, _self, [&](auto &update_user_bid) {
                update_user_bid.masset = mappedasset + update_user_bid.masset;
            });
        }


        auto pioneeracct_lookup = _pioneeraccts.find(owner.value);
        if(pioneeracct_lookup == _pioneeraccts.end()){
            _pioneeraccts.emplace(_self, [&](auto &new_pioneeracct) { // contract账户为user account付款
                new_pioneeracct.acct = owner;
            });
        }

        auto dailybid_lookup = _dailybids.find(symday);
        if (dailybid_lookup == _dailybids.end()) {
            _dailybids.emplace(_self, [&](auto &new_dailybid) {
                new_dailybid.symday = symday;
                new_dailybid.masset = mappedasset;
                std::set <name> newset;
                newset.insert(owner);
                new_dailybid.biddedusers = newset;
            });
        } else {
            _dailybids.modify(dailybid_lookup, _self, [&](auto &update_daily_bid) {
                update_daily_bid.masset = mappedasset + update_daily_bid.masset;
                set <name> existing_set = update_daily_bid.biddedusers;
                existing_set.insert(owner);
                update_daily_bid.biddedusers = existing_set;
            });
        }

    }

    // 对于一个特定天和，计算应分发的bos数目
    void bosio_issuance::calculatebid(uint64_t yesterday, uint64_t bidsymraw) {
        // "eos"_n.value6138406292105986048day: 246eosday: 246
//        eosio::print("bidsymraw: ", bidsymraw);
//        eosio::print("yesterday: ", yesterday);
        uint64_t sym_day = index_encode(yesterday, bidsymraw);
        eosio::print("sym_day: ", sym_day);
        auto dailybid_lookup = _dailybids.find(sym_day);
        if (dailybid_lookup != _dailybids.end()) {
            _dailyorders.emplace(_self, [&](auto &new_dailyorder) {
                new_dailyorder.symday = sym_day;
                new_dailyorder.masset = dailybid_lookup->masset;
                new_dailyorder.bosasset = asset{static_cast<int64_t>(15000000000), symbol("BOS", 4)};
                new_dailyorder.userset = dailybid_lookup->biddedusers;
            });

            auto totaltoken = dailybid_lookup->masset.amount; //int64_t auto
            eosio::print("totaltoken int64_t", totaltoken);
            // how many BOS can get from one token
            double price = 1500000 / totaltoken;
            eosio::print("Num of bos for each token: ", price);


            std::set <name> userset = dailybid_lookup->biddedusers;
            std::set<name>::iterator setit;
            for (setit = userset.begin(); setit != userset.end(); ++setit) {
                name f = *setit; // Note the "*" here
                userbids_table _userbids(_self, f.value); // 建立user scope
                auto userbid_lookup = _userbids.find(sym_day);
                if (userbid_lookup != _userbids.end()) {
                    transaction t;
                    t.actions.emplace_back(permission_level{_self, active_permission},
                                           _self, "sendbos"_n,
                                           std::make_tuple(f, dailybid_lookup->masset, price, sym_day)
                    );
                    t.delay_sec = 1;
                    uint128_t deferred_id = _self.value + now();
                    t.send(deferred_id, _self, true);
                } else {
                    eosio::print("no user bids");
                }
            }

            // 找到eos income的所有用户
        } else {
            eosio::print("no eos income for day: ", yesterday);
        }
    }

    void bosio_issuance::sendbos(name receiver, asset masset, double price, uint64_t symday, uint64_t day) {
        // 发送并入table
        INLINE_ACTION_SENDER(eosio::token, transfer)(
                bostoken_account, {{_self, active_permission}},
                {_self, receiver, masset * price, std::string("distribute BOS on day "+ day)}
        );
        userorders_table _userorders(_self, receiver.value);
        _userorders.emplace(_self, [&](auto &new_userorder) { // contract账户为user account付款
            new_userorder.symday = symday;
            new_userorder.masset = masset;
            int64_t bosamt = masset.amount * price * 10000;
            eosio::print("!!!!bosamt: !!!", bosamt);
            asset bosasset = asset{bosamt, symbol("BOS", 4)};
            new_userorder.bosasset = bosasset;
        });
    }

}