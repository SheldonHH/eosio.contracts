#include "../include/bos.issuance.hpp"
#include "bos.helper.cpp"

using namespace eosio;
namespace bosio {
    uint64_t bosio_issuance::superewardpert(uint64_t super_reward, uint64_t rank) {
        if (rank == 1) {
            return super_reward * 0.4;
        } else if (rank == 2) {
            return super_reward * 0.2;
        } else if (rank == 3) {
            return super_reward * 0.16;
        } else if (rank == 4) {
            return super_reward * 0.12;
        } else if (rank == 5) {
            return super_reward * 0.08;
        } else if (rank == 6) {
            return super_reward * 0.04;
        }
    }


    void bosio_issuance::addtoq(const name _token_contract,
                                const name _from,
                                const name _to,
                                const asset _asset,
                                const string _memo) {

        pendxfer_table pendxfer(_self, _self.value);
        pendxfer.emplace(_self, [&](auto &t) {
            t.xfer_id = pendxfer.available_primary_key();
            t.token_contract = _token_contract;
            t.from = _from;
            t.to = _to;
            t.quantity = _asset;
            t.memo = _memo;
            t.created = now();
        });
    }


// TODO: 对每一种分类，然后分发开奖，secondary_index即应当选择symbol
    void bosio_issuance::reward_token(uint64_t symraw) {
        // for all sym == eos
        uint64_t total_token_remain, total_bos_sold;
        std::string symstr = uint_to_string(symraw);

        auto sym_index = _dailyorders.get_index<name("symbol")>();
        // 对特定symbol
        auto itrdorder = sym_index.find(symraw);
        std::map <uint64_t, uint64_t> dayamt_map;                     // map<期数，剩余token的数目>
        std::vector <std::pair<uint64_t, uint64_t>> sortedayamt_vec;  // 用于开大奖，按照排序好的day和pert
        std::vector <std::pair<uint64_t, uint64_t>> daypert_vec;      // 用于普通开奖， pair<day, pert>

        //遍历特定币种的masset.amount，【day，masset.amount】生成map，对该map按照value进行排序
        for (; itrdorder != sym_index.end(); itrdorder++) {
            uint64_t day = std::get<1>(index_decode(itrdorder->symday));
            uint64_t tokenremain_oftheday = itrdorder->masset.amount;
            dayamt_map.insert(std::pair<uint64_t, uint64_t>(day, tokenremain_oftheday));
            total_token_remain += itrdorder->masset.amount;
            total_bos_sold += itrdorder->bosasset.amount;
        }
        MapSortOfValue(sortedayamt_vec, dayamt_map);


        uint64_t super_reward = total_token_remain * 0.25;
        // TODO: for eos, btc, eth for each day, select top 6 most valuable
        std::vector < std::pair < uint64_t, uint64_t > > ::iterator
        sortedit;
        uint64_t rank = 1;
        for (sortedit = sortedayamt_vec.begin(); sortedit != sortedayamt_vec.end() && rank <= 6; sortedit++) {
            eosio::print("\n the ranked by eos \n");
            eosio::print("day: ", sortedit->first);
            eosio::print("amount: ", sortedit->second);
            // get the symday for super reward
            uint64_t symday_topsix = index_encode(symraw, sortedit->first);
            auto topsix_dailyorder = _dailyorders.get(symday_topsix);

            uint64_t bos_of_theday = topsix_dailyorder.bosasset.amount;
            uint64_t supereward_of_theday = superewardpert(super_reward, rank);    // 对于该天，bos reward会是多少 0.1的所有部分
            double supereward_foreach_bos =
                    supereward_of_theday / bos_of_theday;  // 对于该天，每一个bos，能分得多少token: 用该天可获得的reward的数目除以该天获得的bos数目

            std::set <name> userset = topsix_dailyorder.userset;                    // 对于该天，有哪些人参与了募集
            std::set<name>::iterator setit;
            for (setit = userset.begin(); setit != userset.end(); ++setit) {
                name f = *setit;
                userorders_table _userorders(_self, f.value);
                auto one_user = _userorders.get(symday_topsix);
                asset usertokenreward = supereward_foreach_bos * one_user.bosasset;
                addtoq(pegtoken_account, _self, f, usertokenreward, "Top "+std::to_string(rank)+" super reward");   // 完成统计
            }
            rank++;
        } // end of super reward (top six)
        processq();

        // 每个用户会执行一次
        uint64_t normal_reward = total_token_remain * 0.25;
        double price_ofreward = normal_reward / total_bos_sold;    // TODO: double 处理，计算每期token占总token比重，用于开normal reward
        for (auto &acct : _pioneeraccts) {   // 对每一个pioneer的pioneer table进行遍历
            userorders_table _userorders(_self, acct.acct.value);
            asset totalbos_oftheuser;       // 一个user总共的bos数目
            for (auto &user: _userorders) {
                totalbos_oftheuser += user.bosasset;
            }
            asset usertokenreward = totalbos_oftheuser * price_ofreward;
            addtoq(pegtoken_account, _self, acct.acct, usertokenreward, "reward");
        }
        processq();
    }

    void bosio_issuance::processq() {
        pendxfer_table pendxfer(_self, _self.value);

        bool empty = false;
        auto itr = pendxfer.begin();

        uint64_t payload = itr->xfer_id;
        eosio::transaction out{};
        deferfuncarg
        a = {.payload = payload};
        print("payload: ", payload);
        out.actions.emplace_back(permission_level{_self, "active"_n}, _self, "processxfer"_n, a);
        out.delay_sec = 1;
        uint64_t sender_id = now() + itr->xfer_id;
        out.send(sender_id, _self);
    }

    void bosio_issuance::processxfer(uint64_t payload) {
        pendxfer_table pendxfer(_self, _self.value);
        auto itr = pendxfer.find(payload);
        eosio_assert(itr != pendxfer.end(), "Transfer ID is not found.");

        print("---------- Transfer -----------\n");
        print("Transfer ID:      ", itr->xfer_id, "\n");
        print("Token Contract:   ", name{itr->token_contract}, "\n");
        print("From:             ", name{itr->from}, "\n");
        print("To:               ", name{itr->to}, "\n");
        print("Amount:           ", itr->quantity, "\n");
        print("Memo:             ", itr->memo, "\n");
        print("---------- End Transfer -------\n");

        // 发送transfer action
        action(
                permission_level{itr->from, "active"_n},
                itr->token_contract, "transfer"_n,
                std::make_tuple(itr->from, itr->to, itr->quantity, itr->memo))
                .send();




        // 计入compxfer table
        compxfer_table compxfer(_self, _self.value);
        compxfer.emplace(_self, [&](auto &t) {
            t.xfer_id = compxfer.available_primary_key();
            t.token_contract = itr->token_contract;
            t.from = itr->from;
            t.to = itr->to;
            t.quantity = itr->quantity;
            t.memo = itr->memo;
            t.created = itr->created;
            t.completed = now();
        });

        itr++;
        if (itr != pendxfer.end()) {
            uint64_t payload = itr->xfer_id;
            eosio::transaction out{};
            print("---------- Deferring Transaction -----------\n");
            print("Transfer ID:      ", itr->xfer_id, "\n");
            print("Token Contract:   ", name{itr->token_contract}, "\n");
            print("From:             ", name{itr->from}, "\n");
            print("To:               ", name{itr->to}, "\n");
            print("Amount:           ", itr->quantity, "\n");
            print("Memo:             ", itr->memo, "\n");
            print("---------- End Transfer ------------------\n");
            deferfuncarg
            a = {.payload = payload};
            out.actions.emplace_back(permission_level{_self, "active"_n}, _self, "processxfer"_n, a);
            out.delay_sec = 2;
            uint64_t sender_id = now() + itr->xfer_id;
            out.send(sender_id, _self);
        }
        itr--;
        itr = pendxfer.erase(itr);

    }


}


// 增加user action table中user.reward的数量
//userorders_table _userorders(_self, itr->to.value);
//auto userorders_lookup = _userorders.find(symday);  // if (userorders_lookup != _userorders.end())
//_userorders.modify(userorders_lookup, _self, [&](auto &update_user_order) {
//update_user_order.reward += itr->quantity;
//});