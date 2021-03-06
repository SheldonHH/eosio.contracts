#pragma once
#include <eosiolib/eosio.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/transaction.hpp>
#include <eosiolib/singleton.hpp>
#include <string>
#include <regex>
#include <stdio.h>      /* printf, scanf */
#include <time.h>       /* time_t, struct tm, time, mktime */
#include <unistd.h>
#include "eosio.token.hpp"

using namespace eosio;
using namespace std;

namespace bos {
    typedef uint64_t symbol_name;

    class [[eosio::contract("bos.issuance")]] bosio_issuance : public contract {
    public:
        using contract::contract;
        bosio_issuance(name receiver, name code,  datastream<const char*> ds): contract(receiver, code, ds),
                                                                               _issuance(_self, _self.value),
                                                                               _dailyorders(_self, _self.value),
                                                                               _dailybids(_self, _self.value),
                                                                               _pioneeraccts(_self, _self.value)
        {
                _istate = _issuance.exists() ? _issuance.get() :get_default_param();
        }
        struct [[eosio::table, eosio::contract("bos.issuance")]] dailybid {
            uint64_t symday;
            asset masset;
            std::set<name> biddedusers;
            uint64_t primary_key() const { return symday; }

            EOSLIB_SERIALIZE( dailybid, (symday)(masset)(biddedusers) )
        };
        typedef eosio::multi_index<"dailybids"_n, dailybid > dailybids_table;
        dailybids_table _dailybids;

        struct [[eosio::table, eosio::contract("bos.issuance")]] dailyorder {
            uint64_t symday;
            asset masset;
            uint64_t symbol;
            asset bosasset;
            std::set<name> userset;
            uint64_t primary_key() const { return symday; }
            uint64_t by_sym() const { return masset.symbol.code().raw(); }
        };
        typedef eosio::multi_index<"dailyorders"_n, dailyorder,  indexed_by< name("symbol"), const_mem_fun<dailyorder, uint64_t, &dailyorder::by_sym>>> dailyorders_table;
        dailyorders_table _dailyorders;

        struct [[eosio::table, eosio::contract("bos.issuance")]] userbid {
            uint64_t symday;
            asset masset;
            uint64_t primary_key() const { return symday;}
        };
        typedef eosio::multi_index<"userbids"_n, userbid > userbids_table;

        struct [[eosio::table, eosio::contract("bos.issuance")]] userorder {
            uint64_t symday;
            asset masset;
            uint64_t symbol;
            asset bosasset;

            uint64_t primary_key() const { return  symday; }
            uint64_t by_sym() const { return masset.symbol.code().raw(); }
        };
        typedef eosio::multi_index<"userorders"_n, userorder, indexed_by< name("symbol"), const_mem_fun<userorder, uint64_t, &userorder::by_sym>>> userorders_table;


        struct [[eosio::table("pioneeracct"), eosio::contract("bos.issuance")]]  pioneeracct{
            name acct;
            uint64_t primary_key() const { return  acct.value; }
        };
        typedef eosio::multi_index< "pioneeraccts"_n, pioneeracct >  pioneeraccts_table;
        pioneeraccts_table _pioneeraccts;



        struct [[eosio::table, eosio::contract("bos.issuance")]] trxferstruct {
            name from;
            name to;
            asset quantity;
            string memo;
            EOSLIB_SERIALIZE( trxferstruct, (from)(to)(quantity)(memo))
        };

        struct [[eosio::table("global"), eosio::contract("bos.issuance")]] issuestate{
            uint64_t curday;
            uint64_t startbidtime;
            uint64_t oneday;           // oneday: 一天的长度
            uint64_t endbidtime;
            uint64_t refundbuffertime; // refund距上一天竞买需要多少秒
            uint64_t primary_key() const { return  curday; }
        };
        typedef eosio::singleton< "global"_n, issuestate >   issuance_state_singleton;


        issuestate get_default_param();

//        [[eosio::action]]
//        void init(const name owner);

        [[eosio::action]]
        void init(uint64_t startbidtime, uint64_t oneday, uint64_t refundbuffertime);

        [[eosio::action]]
        void timechecking();

        [[eosio::action]]
        void transfer(const name sender, const name receiver);

        [[eosio::action]]
        void notify(name user, std::string msg);

        [[eosio::action]]
        void printhello();



        static constexpr eosio::name active_permission{"active"_n};
        static constexpr eosio::name dividend_account{"bos.fund"_n};
        static constexpr eosio::name bostoken_account{"bosissuances"_n};
        static constexpr eosio::name pegtoken_account{"bosissuances"_n};
        static constexpr eosio::name issuance_account{"bosissuances"_n};



        // @abi action
        ACTION addtoq ( const name    _token_contract,
                        const name    _from,
                        const name    _to,
                        const asset           _asset,
                        const string          _memo ) ;

        // @abi action
        ACTION processq () ;

        // @abi action
        ACTION processxfer ( uint64_t payload);

        // @abi table pendxfers i64
        struct [[eosio::table, eosio::contract("bos.issuance")]] pendxfer {
                uint64_t        xfer_id;
                name    token_contract;
                name    from;
                name    to;
                asset           quantity;
                string          memo;
                uint32_t        created;

                uint64_t primary_key() const { return xfer_id; }
                EOSLIB_SERIALIZE (pendxfer, (xfer_id)(token_contract)(from)(to)(quantity)(memo)(created));
        };
        typedef eosio::multi_index<"pendxfers"_n, pendxfer> pendxfer_table;


        // @abi table compxfers i64
        struct [[eosio::table, eosio::contract("bos.issuance")]] compxfer {
                uint64_t        xfer_id;
                name            token_contract;
                name            from;
                name            to;
                asset           quantity;
                string          memo;
                uint32_t        created;
                uint32_t        completed;

                uint64_t primary_key() const { return xfer_id; }
                EOSLIB_SERIALIZE (compxfer, (xfer_id)(token_contract)(from)(to)(quantity)(memo)(created)(completed));
        };
        typedef eosio::multi_index<"compxfers"_n, compxfer> compxfer_table;

        struct [[eosio::table, eosio::contract("bos.issuance")]] deferfuncarg {
                uint64_t payload;
        };
    private:
        uint64_t startbidtime = 1542211943; //TODO: 测试时需修改，也可作为全局的参数
        uint64_t oneday = 60; //一天设置为一分钟
        uint64_t refundbuffertime = 30; // 30秒，或半天 43200
        uint64_t endbidtime = startbidtime + oneday * 300; // 300分钟或300天

        issuance_state_singleton _issuance;
        issuestate _istate;
        void foreach_token(uint64_t day, string option);

        // bos.bidding
        void bidbos(const name owner, uint64_t symday, asset mappedasset); // when receive eos, btc, eth
        void calculatebid(uint64_t yesterday, uint64_t bidsymraw);
        void sendbos(name receiver, asset masset, double price, uint64_t symday, uint64_t day); // send bos

        // bos.dividend
        void calculatedividend(uint64_t today, uint64_t divdsymraw);

        // bos.refund
        void refundtoken(name owner, uint64_t symday, asset bosrefund); // when receive bos,  send eos, btc, eth
        double avarefundpert(uint64_t symday);
        uint64_t get_bought_period(uint64_t day);
        bool validaterefundmemo(string memo);


        // bos.reward
        void reward_token(uint64_t symraw);
        uint64_t superewardpert(uint64_t super_reward, uint64_t count);
        void sendreward(name receiver, asset usertokenreward, uint64_t day); // eos, btc, eth

        void send_summary(name user, std::string message);

        uint64_t index_encode( uint64_t sym, uint64_t day);
        std::tuple<uint64_t ,uint64_t> index_decode(uint64_t symday);
        void MapSortOfValue(std::vector<std::pair<uint64_t,uint64_t> >& vec,std::map<uint64_t,uint64_t>& m);
        std::string uint_to_string(const uint64_t name) const;
    };

}