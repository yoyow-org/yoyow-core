/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <graphene/app/database_api.hpp>
#include <graphene/app/util.hpp>
#include <graphene/chain/get_config.hpp>
#include <graphene/utilities/string_escape.hpp>

#include <fc/bloom_filter.hpp>
#include <fc/smart_ref_impl.hpp>

#include <fc/crypto/hex.hpp>

#include <boost/range/iterator_range.hpp>
#include <boost/rational.hpp>
#include <boost/multiprecision/cpp_int.hpp>

#include <cctype>

#include <cfenv>
#include <iostream>

#define GET_REQUIRED_FEES_MAX_RECURSION 4

typedef std::map< std::pair<graphene::chain::asset_aid_type, graphene::chain::asset_aid_type>, std::vector<fc::variant> > market_queue_type;

namespace graphene { namespace app {

signed_block_with_info::signed_block_with_info( const signed_block& block )
   : signed_block( block )
{
   block_id = id();
   signing_key = signee();
   transaction_ids.reserve( transactions.size() );
   for( const processed_transaction& tx : transactions )
      transaction_ids.push_back( tx.id() );
}


class database_api_impl : public std::enable_shared_from_this<database_api_impl>
{
   public:
      explicit database_api_impl(graphene::chain::database& db, const application_options* app_options);
      ~database_api_impl();

      // Objects
      fc::variants get_objects(const vector<object_id_type>& ids)const;

      // Subscriptions
      void set_subscribe_callback( std::function<void(const variant&)> cb, bool notify_remove_create );
      void set_pending_transaction_callback( std::function<void(const variant&)> cb );
      void set_block_applied_callback( std::function<void(const variant& block_id)> cb );
      void cancel_all_subscriptions();

      // Blocks and transactions
      optional<block_header> get_block_header(uint32_t block_num)const;
      map<uint32_t, optional<block_header>> get_block_header_batch(const vector<uint32_t> block_nums)const;
      optional<signed_block_with_info> get_block(uint32_t block_num)const;
      processed_transaction get_transaction( uint32_t block_num, uint32_t trx_in_block )const;

      // Globals
      chain_property_object get_chain_properties()const;
      global_property_object get_global_properties()const;
      fc::variant_object get_config()const;
      chain_id_type get_chain_id()const;
      dynamic_global_property_object get_dynamic_global_properties()const;

      // Keys
      vector<vector<account_uid_type>> get_key_references( vector<public_key_type> key )const;
      bool is_public_key_registered(string public_key) const;

      // Accounts
      vector<optional<account_object>> get_accounts(const vector<account_id_type>& account_ids)const;
      vector<optional<account_object>> get_accounts_by_uid(const vector<account_uid_type>& account_uids)const;
      std::map<string,full_account> get_full_accounts( const vector<string>& names_or_ids, bool subscribe );
      std::map<account_uid_type,full_account> get_full_accounts_by_uid( const vector<account_uid_type>& uids,
                                                                        const full_account_query_options& options );
      vector<pledge_balance_object> get_account_core_asset_pledge(account_uid_type account_uid)const;
      account_statistics_object get_account_statistics_by_uid(account_uid_type uid)const;
      optional<account_object> get_account_by_name( string name )const;
      vector<account_uid_type> get_account_references( account_uid_type uid )const;
      vector<optional<account_object>> lookup_account_names(const vector<string>& account_names)const;
      map<string,account_uid_type> lookup_accounts_by_name(const string& lower_bound_name, uint32_t limit)const;
      uint64_t get_account_count()const;

      uint64_t get_account_auth_platform_count(const account_uid_type platform)const;
      vector<account_auth_platform_object> list_account_auth_platform_by_platform(const account_uid_type platform,
                                                                                  const account_uid_type lower_bound_account,
                                                                                  const uint32_t limit)const;
      vector<account_auth_platform_object> list_account_auth_platform_by_account(const account_uid_type account,
                                                                                 const account_uid_type lower_bound_platform,
                                                                                 const uint32_t limit)const;

      //pledge mining
      vector<pledge_mining_object> list_pledge_mining_by_witness(const account_uid_type witness,
         const account_uid_type lower_bound_account,
         const uint32_t limit)const;

      vector<pledge_mining_object> list_pledge_mining_by_account(const account_uid_type account,
         const account_uid_type lower_bound_witness,
         const uint32_t limit)const;

      // CSAF
      vector<csaf_lease_object> get_csaf_leases_by_from( const account_uid_type from,
                                                         const account_uid_type lower_bound_to,
                                                         const uint32_t limit )const;
      vector<csaf_lease_object> get_csaf_leases_by_to( const account_uid_type to,
                                                       const account_uid_type lower_bound_from,
                                                       const uint32_t limit )const;


      // Platforms and posts
      vector<optional<platform_object>> get_platforms( const vector<account_uid_type>& platform_uids )const;
      fc::optional<platform_object> get_platform_by_account( account_uid_type account )const;
      vector<platform_object> lookup_platforms( const account_uid_type lower_bound_uid,
                                               uint32_t limit,
                                               data_sorting_type order_by )const;
      uint64_t get_platform_count()const;
      optional<post_object> get_post( const account_uid_type platform_owner,
                                      const account_uid_type poster_uid,
                                      const post_pid_type post_pid )const;
      vector<post_object> get_posts_by_platform_poster( const account_uid_type platform_owner,
                                      optional<account_uid_type> poster,
                                      const object_id_type lower_bound_post,
                                      const uint32_t limit )const;
      optional<score_object> get_score(const account_uid_type platform,
                                       const account_uid_type poster_uid,
                                       const post_pid_type    post_pid,
                                       const account_uid_type from_account)const;
      vector<score_object> get_scores_by_uid(const account_uid_type  scorer,
                                             const uint32_t period,
                                             const object_id_type lower_bound_score,
                                             const uint32_t limit)const;

      vector<score_object> list_scores(const account_uid_type platform,
                                       const account_uid_type poster_uid,
                                       const post_pid_type    post_pid,
                                       const object_id_type   lower_bound_score,
                                       const uint32_t         limit,
                                       const bool             list_cur_period)const;

      optional<license_object> get_license(const account_uid_type platform,
                                           const license_lid_type license_lid)const;
      vector<license_object> list_licenses(const account_uid_type platform, const object_id_type lower_bound_license, const uint32_t limit)const;

      vector<advertising_object> list_advertisings(const account_uid_type platform, const advertising_aid_type lower_bound_advertising, const uint32_t limit)const;
      optional<advertising_object> get_advertising(const account_uid_type platform, const advertising_aid_type advertising_aid)const;

      vector<advertising_order_object> list_advertising_orders_by_purchaser(const account_uid_type purchaser,
                                                                            const object_id_type lower_bound_advertising_order,
                                                                            uint32_t limit)const;
      vector<advertising_order_object> list_advertising_orders_by_ads_aid(const account_uid_type platform,
                                                                          const advertising_aid_type id,
                                                                          const advertising_order_oid_type lower_bound_advertising_order,
                                                                          uint32_t limit)const;

      vector<custom_vote_object> list_custom_votes(optional<custom_vote_id_type> lower_bound_custom_vote_id, optional<bool> is_finished, uint32_t limit)const;
      vector<custom_vote_object> lookup_custom_votes(const account_uid_type creator, const custom_vote_vid_type lower_bound_custom_vote, uint32_t limit)const;

      vector<cast_custom_vote_object> list_cast_custom_votes_by_id(const account_uid_type     creator, 
                                                                   const custom_vote_vid_type vote_vid, 
                                                                   const object_id_type       lower_bound_cast_custom_vote, 
                                                                   uint32_t                   limit)const;
      vector<cast_custom_vote_object> list_cast_custom_votes_by_voter(const account_uid_type voter, const object_id_type lower_bound_cast_custom_vote, uint32_t limit)const;

      vector<active_post_object> get_post_profits_detail(const uint32_t         begin_period,
                                                         const uint32_t         end_period,
                                                         const account_uid_type platform,
                                                         const account_uid_type poster,
                                                         const post_pid_type    post_pid)const;

      vector<Platform_Period_Profit_Detail> get_platform_profits_detail(const uint32_t         begin_period,
                                                                        const uint32_t         end_period,
                                                                        const account_uid_type platform,
                                                                        const uint32_t         lower_bound_index,
                                                                        uint32_t               limit)const;

      vector<Poster_Period_Profit_Detail> get_poster_profits_detail(const uint32_t         begin_period,
                                                                    const uint32_t         end_period,
                                                                    const account_uid_type poster,
                                                                    const uint32_t         lower_bound_index,
                                                                    uint32_t               limit)const;

      uint64_t get_posts_count(optional<account_uid_type> platform, optional<account_uid_type> poster)const;

      share_type get_score_profit(account_uid_type account, uint32_t period)const;

      // Balances
      vector<asset> get_account_balances(account_uid_type uid, const flat_set<asset_aid_type>& assets)const;
      vector<asset> get_named_account_balances(const std::string& name, const flat_set<asset_aid_type>& assets)const;

      // Assets
      asset_aid_type get_asset_id_from_string(const std::string& symbol_or_id)const;
      vector<optional<asset_object_with_data>> get_assets(const vector<asset_aid_type>& asset_ids)const;
      vector<asset_object_with_data>           list_assets(const string& lower_bound_symbol, uint32_t limit)const;
      vector<optional<asset_object_with_data>> lookup_asset_symbols(const vector<string>& symbols_or_ids)const;

      // Markets / feeds
      vector<limit_order_object>         get_limit_orders(const std::string& a, const std::string& b, uint32_t limit)const;
      vector<limit_order_object>         get_account_limit_orders(const string& account_name_or_id,
                                                                  const string &base,
                                                                  const string &quote, uint32_t limit,
                                                                  optional<limit_order_id_type> ostart_id,
                                                                  optional<price> ostart_price);
      vector<limit_order_object>         get_account_all_limit_orders(const string& account_name_or_id,
                                                                      uint32_t limit,
                                                                      optional<limit_order_id_type> ostart_id);

      void subscribe_to_market(std::function<void(const variant&)> callback, const std::string& a, const std::string& b);
      void unsubscribe_from_market(const std::string& a, const std::string& b);

      market_ticker                      get_ticker(const string& base, const string& quote, bool skip_order_book = false)const;
      market_volume                      get_24_volume(const string& base, const string& quote)const;
      order_book                         get_order_book(const string& base, const string& quote, unsigned limit = 50)const;
      vector<market_ticker>              get_top_markets(uint32_t limit)const;
      vector<market_trade>               get_trade_history(const string& base, const string& quote, fc::time_point_sec start, fc::time_point_sec stop, unsigned limit = 100)const;
      vector<market_trade>               get_trade_history_by_sequence(const string& base, const string& quote, int64_t start, fc::time_point_sec stop, unsigned limit = 100)const;

      // Witnesses
      vector<optional<witness_object>> get_witnesses(const vector<account_uid_type>& witness_uids)const;
      fc::optional<witness_object> get_witness_by_account(account_uid_type account)const;
      vector<witness_object> lookup_witnesses(const account_uid_type lower_bound_uid, uint32_t limit,
                                              data_sorting_type order_by)const;
      uint64_t get_witness_count()const;

      // Committee members and proposals
      vector<optional<committee_member_object>> get_committee_members(const vector<account_uid_type>& committee_member_uids)const;
      fc::optional<committee_member_object> get_committee_member_by_account(account_uid_type account)const;
      vector<committee_member_object> lookup_committee_members(const account_uid_type lower_bound_uid, uint32_t limit,
                                                               data_sorting_type order_by)const;
      uint64_t get_committee_member_count()const;
      vector<committee_proposal_object> list_committee_proposals()const;

      // Authority / validation
      std::string get_transaction_hex(const signed_transaction& trx)const;
      std::pair<std::pair<flat_set<public_key_type>,flat_set<public_key_type>>,flat_set<signature_type>> get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const;
      set<public_key_type> get_potential_signatures( const signed_transaction& trx )const;
      bool verify_authority( const signed_transaction& trx )const;
      bool verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& signers )const;
      processed_transaction validate_transaction( const signed_transaction& trx )const;
      vector< fc::variant > get_required_fees( const vector<operation>& ops, asset_id_type id )const;
      vector< required_fee_data > get_required_fee_data( const vector<operation>& ops )const;

      // Proposed transactions
      vector<proposal_object> get_proposed_transactions( account_uid_type uid )const;

   //private:
      static string price_to_string(const price& _price, const asset_object& _base, const asset_object& _quote);

      template<typename T>
      void subscribe_to_item( const T& i )const
      {
         auto vec = fc::raw::pack(i);
         if( !_subscribe_callback )
            return;

         if( !is_subscribed_to_item(i) )
         {
            _subscribe_filter.insert( vec.data(), vec.size() );
         }
      }

      template<typename T>
      bool is_subscribed_to_item( const T& i )const
      {
         if( !_subscribe_callback )
            return false;

         return _subscribe_filter.contains( i );
      }

      bool is_impacted_account( const flat_set<account_uid_type>& accounts)
      {
         if( !_subscribed_accounts.size() || !accounts.size() )
            return false;

         return std::any_of(accounts.begin(), accounts.end(), [this](const account_uid_type& account) {
            return _subscribed_accounts.find(account) != _subscribed_accounts.end();
         });
      }

      const account_object* get_account_from_string(const std::string& name_or_id) const
      {
         // TODO cache the result to avoid repeatly fetching from db
         FC_ASSERT(name_or_id.size() > 0);
         const account_object* account = nullptr;
         if (std::isdigit(name_or_id[0]))
            account = _db.find_account_by_uid(fc::variant(name_or_id).as<account_uid_type>(1));
         else
         {
            const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
            auto itr = idx.find(name_or_id);
            if (itr != idx.end())
               account = &*itr;
         }
         FC_ASSERT(account, "no such account");
         return account;
      }

      const asset_object* get_asset_from_string(const std::string& symbol_or_id) const
      {
         // TODO cache the result to avoid repeatly fetching from db
         FC_ASSERT(symbol_or_id.size() > 0);
         const asset_object* asset = nullptr;
         if (std::isdigit(symbol_or_id[0]))
            asset = _db.find(fc::variant(symbol_or_id, 1).as<asset_id_type>(1));
         else
         {
            const auto& idx = _db.get_index_type<asset_index>().indices().get<by_symbol>();
            auto itr = idx.find(symbol_or_id);
            if (itr != idx.end())
               asset = &*itr;
         }
         FC_ASSERT(asset, "no such asset");
         return asset;
      }

      //const std::pair<asset_id_type, asset_id_type> get_order_market(const force_settlement_object& order)
      //{
      //    // TODO cache the result to avoid repeatly fetching from db
      //    asset_id_type backing_id = order.balance.asset_id(_db).bitasset_data(_db).options.short_backing_asset;
      //    auto tmp = std::make_pair(order.balance.asset_id, backing_id);
      //    if (tmp.first > tmp.second) std::swap(tmp.first, tmp.second);
      //    return tmp;
      //}

      vector<limit_order_object> get_limit_orders(const asset_aid_type a, const asset_aid_type b, const uint32_t limit)const
      {
          FC_ASSERT(limit <= 300);

          const auto& limit_order_idx = _db.get_index_type<limit_order_index>();
          const auto& limit_price_idx = limit_order_idx.indices().get<by_price>();

          vector<limit_order_object> result;
          result.reserve(limit * 2);

          uint32_t count = 0;
          auto limit_itr = limit_price_idx.lower_bound(price::max(a, b));
          auto limit_end = limit_price_idx.upper_bound(price::min(a, b));
          while (limit_itr != limit_end && count < limit)
          {
              result.push_back(*limit_itr);
              ++limit_itr;
              ++count;
          }
          count = 0;
          limit_itr = limit_price_idx.lower_bound(price::max(b, a));
          limit_end = limit_price_idx.upper_bound(price::min(b, a));
          while (limit_itr != limit_end && count < limit)
          {
              result.push_back(*limit_itr);
              ++limit_itr;
              ++count;
          }

          return result;
      }

      void broadcast_updates( const vector<variant>& updates );
      void handle_object_changed(bool force_notify, bool full_object, const vector<object_id_type>& ids, const flat_set<account_uid_type>& impacted_accounts, std::function<const object*(object_id_type id)> find_object);

      /** called every time a block is applied to report the objects that were changed */
      void on_objects_new(const vector<object_id_type>& ids, const flat_set<account_uid_type>& impacted_accounts);
      void on_objects_changed(const vector<object_id_type>& ids, const flat_set<account_uid_type>& impacted_accounts);
      void on_objects_removed(const vector<object_id_type>& ids, const vector<const object*>& objs, const flat_set<account_uid_type>& impacted_accounts);
      void on_applied_block();

      bool _notify_remove_create = false;
      mutable fc::bloom_filter _subscribe_filter;
      std::set<account_uid_type> _subscribed_accounts;
      std::function<void(const fc::variant&)> _subscribe_callback;
      std::function<void(const fc::variant&)> _pending_trx_callback;
      std::function<void(const fc::variant&)> _block_applied_callback;

      boost::signals2::scoped_connection                                                   _new_connection;
      boost::signals2::scoped_connection                                                   _change_connection;
      boost::signals2::scoped_connection                                                   _removed_connection;
      boost::signals2::scoped_connection                                                   _applied_block_connection;
      boost::signals2::scoped_connection                                                   _pending_trx_connection;
      map< pair<asset_aid_type, asset_aid_type>, std::function<void(const variant&)> >     _market_subscriptions;
      graphene::chain::database&                                                           _db;
      const application_options* _app_options = nullptr;
};

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Constructors                                                     //
//                                                                  //
//////////////////////////////////////////////////////////////////////

database_api::database_api(graphene::chain::database& db, const application_options* app_options)
   : my(new database_api_impl(db, app_options)) {}

database_api::~database_api() {}

database_api_impl::database_api_impl(graphene::chain::database& db, const application_options* app_options) 
   : _db(db), _app_options(app_options)
{
   wlog("creating database api ${x}", ("x",int64_t(this)) );
   _new_connection = _db.new_objects.connect([this](const vector<object_id_type>& ids, const flat_set<account_uid_type>& impacted_accounts) {
                                on_objects_new(ids, impacted_accounts);
                                });
   _change_connection = _db.changed_objects.connect([this](const vector<object_id_type>& ids, const flat_set<account_uid_type>& impacted_accounts) {
                                on_objects_changed(ids, impacted_accounts);
                                });
   _removed_connection = _db.removed_objects.connect([this](const vector<object_id_type>& ids, const vector<const object*>& objs, const flat_set<account_uid_type>& impacted_accounts) {
                                on_objects_removed(ids, objs, impacted_accounts);
                                });
   _applied_block_connection = _db.applied_block.connect([this](const signed_block&){ on_applied_block(); });

   _pending_trx_connection = _db.on_pending_transaction.connect([this](const signed_transaction& trx ){
                         if( _pending_trx_callback ) _pending_trx_callback( fc::variant(trx, GRAPHENE_MAX_NESTED_OBJECTS) );
                      });
}

database_api_impl::~database_api_impl()
{
   elog("freeing database api ${x}", ("x",int64_t(this)) );
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Market ticker constructor                                        //
//                                                                  //
//////////////////////////////////////////////////////////////////////
market_ticker::market_ticker(const market_ticker_object& mto,
                             const fc::time_point_sec& now,
                             const asset_object& asset_base,
                             const asset_object& asset_quote,
                             const order_book& orders)
{
   time = now;
   base = asset_base.symbol;
   quote = asset_quote.symbol;
   percent_change = "0";
   lowest_ask = "0";
   highest_bid = "0";

   fc::uint128 bv;
   fc::uint128 qv;
   price latest_price = asset(mto.latest_base, mto.base) / asset(mto.latest_quote, mto.quote);
   if (mto.base != asset_base.asset_id)
      latest_price = ~latest_price;
   latest = database_api_impl::price_to_string(latest_price, asset_base, asset_quote);
   if (mto.last_day_base != 0 && mto.last_day_quote != 0 // has trade data before 24 hours
      && (mto.last_day_base != mto.latest_base || mto.last_day_quote != mto.latest_quote)) // price changed
   {
      price last_day_price = asset(mto.last_day_base, mto.base) / asset(mto.last_day_quote, mto.quote);
      if (mto.base != asset_base.asset_id)
         last_day_price = ~last_day_price;
      percent_change = price_diff_percent_string(last_day_price, latest_price);
   }
   if (asset_base.asset_id == mto.base)
   {
      bv = mto.base_volume;
      qv = mto.quote_volume;
   }
   else
   {
      bv = mto.quote_volume;
      qv = mto.base_volume;
   }
   base_volume = uint128_amount_to_string(bv, asset_base.precision);
   quote_volume = uint128_amount_to_string(qv, asset_quote.precision);

   if (!orders.asks.empty())
      lowest_ask = orders.asks[0].price;
   if (!orders.bids.empty())
      highest_bid = orders.bids[0].price;
}
market_ticker::market_ticker(const fc::time_point_sec& now,
                             const asset_object& asset_base,
                             const asset_object& asset_quote)
{
   time = now;
   base = asset_base.symbol;
   quote = asset_quote.symbol;
   latest = "0";
   lowest_ask = "0";
   highest_bid = "0";
   percent_change = "0";
   base_volume = "0";
   quote_volume = "0";
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Objects                                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////

fc::variants database_api::get_objects(const vector<object_id_type>& ids)const
{
   return my->get_objects( ids );
}

fc::variants database_api_impl::get_objects(const vector<object_id_type>& ids)const
{
   if( _subscribe_callback )  {
      for( auto id : ids )
      {
         if( id.type() == operation_history_object_type && id.space() == protocol_ids ) continue;
         if( id.type() == impl_account_transaction_history_object_type && id.space() == implementation_ids ) continue;

         this->subscribe_to_item( id );
      }
   }

   fc::variants result;
   result.reserve(ids.size());

   std::transform(ids.begin(), ids.end(), std::back_inserter(result),
                  [this](object_id_type id) -> fc::variant {
      if(auto obj = _db.find_object(id))
         return obj->to_variant();
      return {};
   });

   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Subscriptions                                                    //
//                                                                  //
//////////////////////////////////////////////////////////////////////

void database_api::set_subscribe_callback( std::function<void(const variant&)> cb, bool notify_remove_create )
{
   my->set_subscribe_callback( cb, notify_remove_create );
}

void database_api_impl::set_subscribe_callback( std::function<void(const variant&)> cb, bool notify_remove_create )
{
   //edump((clear_filter));
   _subscribe_callback = cb;
   _notify_remove_create = notify_remove_create;
   _subscribed_accounts.clear();

   static fc::bloom_parameters param;
   param.projected_element_count    = 10000;
   param.false_positive_probability = 1.0/100;
   param.maximum_size = 1024*8*8*2;
   param.compute_optimal_parameters();
   _subscribe_filter = fc::bloom_filter(param);
}

void database_api::set_pending_transaction_callback( std::function<void(const variant&)> cb )
{
   my->set_pending_transaction_callback( cb );
}

void database_api_impl::set_pending_transaction_callback( std::function<void(const variant&)> cb )
{
   _pending_trx_callback = cb;
}

void database_api::set_block_applied_callback( std::function<void(const variant& block_id)> cb )
{
   my->set_block_applied_callback( cb );
}

void database_api_impl::set_block_applied_callback( std::function<void(const variant& block_id)> cb )
{
   _block_applied_callback = cb;
}

void database_api::cancel_all_subscriptions()
{
   my->cancel_all_subscriptions();
}

void database_api_impl::cancel_all_subscriptions()
{
   set_subscribe_callback( std::function<void(const fc::variant&)>(), true);
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Blocks and transactions                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////

optional<block_header> database_api::get_block_header(uint32_t block_num)const
{
   return my->get_block_header( block_num );
}

optional<block_header> database_api_impl::get_block_header(uint32_t block_num) const
{
   auto result = _db.fetch_block_by_number(block_num);
   if(result)
      return *result;
   return {};
}
map<uint32_t, optional<block_header>> database_api::get_block_header_batch(const vector<uint32_t> block_nums)const
{
   return my->get_block_header_batch( block_nums );
}

map<uint32_t, optional<block_header>> database_api_impl::get_block_header_batch(const vector<uint32_t> block_nums) const
{
   FC_ASSERT( block_nums.size() <= 1000 );
   map<uint32_t, optional<block_header>> results;
   for (const uint32_t block_num : block_nums)
   {
      results[block_num] = get_block_header(block_num);
   }
   return results;
}

optional<signed_block_with_info> database_api::get_block(uint32_t block_num)const
{
   return my->get_block( block_num );
}

optional<signed_block_with_info> database_api_impl::get_block(uint32_t block_num)const
{
   return _db.fetch_block_by_number(block_num);
}

processed_transaction database_api::get_transaction( uint32_t block_num, uint32_t trx_in_block )const
{
   return my->get_transaction( block_num, trx_in_block );
}

optional<signed_transaction> database_api::get_recent_transaction_by_id( const transaction_id_type& id )const
{
   try {
      return my->_db.get_recent_transaction( id );
   } catch ( ... ) {
      return optional<signed_transaction>();
   }
}

processed_transaction database_api_impl::get_transaction(uint32_t block_num, uint32_t trx_num)const
{
   auto opt_block = _db.fetch_block_by_number(block_num);
   FC_ASSERT( opt_block );
   FC_ASSERT( opt_block->transactions.size() > trx_num );
   return opt_block->transactions[trx_num];
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Globals                                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////

chain_property_object database_api::get_chain_properties()const
{
   return my->get_chain_properties();
}

chain_property_object database_api_impl::get_chain_properties()const
{
   return _db.get(chain_property_id_type());
}

global_property_object database_api::get_global_properties()const
{
   return my->get_global_properties();
}

global_property_object database_api_impl::get_global_properties()const
{
   return _db.get(global_property_id_type());
}

fc::variant_object database_api::get_config()const
{
   return my->get_config();
}

fc::variant_object database_api_impl::get_config()const
{
   return graphene::chain::get_config();
}

chain_id_type database_api::get_chain_id()const
{
   return my->get_chain_id();
}

chain_id_type database_api_impl::get_chain_id()const
{
   return _db.get_chain_id();
}

dynamic_global_property_object database_api::get_dynamic_global_properties()const
{
   return my->get_dynamic_global_properties();
}

dynamic_global_property_object database_api_impl::get_dynamic_global_properties()const
{
   return _db.get(dynamic_global_property_id_type());
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Keys                                                             //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<vector<account_uid_type>> database_api::get_key_references( vector<public_key_type> key )const
{
   return my->get_key_references( key );
}

/**
 *  @return all accounts that referr to the key or account id in their owner or active authorities.
 */
vector<vector<account_uid_type>> database_api_impl::get_key_references( vector<public_key_type> keys )const
{
   uint64_t api_limit_get_key_references = _app_options->api_limit_get_key_references;
   FC_ASSERT(keys.size() <= api_limit_get_key_references);

   wdump( (keys) );
   vector< vector<account_uid_type> > final_result;
   final_result.reserve(keys.size());

   for( auto& key : keys )
   {
      subscribe_to_item( key );

      const auto& idx = _db.get_index_type<account_index>();
      const auto& aidx = dynamic_cast<const primary_index<account_index>&>(idx);
      const auto& refs = aidx.get_secondary_index<graphene::chain::account_member_index>();
      auto itr = refs.account_to_key_memberships.find(key);
      vector<account_uid_type> result;

      if( itr != refs.account_to_key_memberships.end() )
      {
         result.reserve( itr->second.size() );
         for( auto item : itr->second ) result.push_back(item);
      }
      final_result.emplace_back( std::move(result) );
   }

   for( auto i : final_result )
      subscribe_to_item(i);

   return final_result;
}

bool database_api::is_public_key_registered(string public_key) const
{
    return my->is_public_key_registered(public_key);
}

bool database_api_impl::is_public_key_registered(string public_key) const
{
    // Short-circuit
    if (public_key.empty()) {
        return false;
    }

    // Search among all keys using an existing map of *current* account keys
    public_key_type key;
    try {
        key = public_key_type(public_key);
    } catch ( ... ) {
        // An invalid public key was detected
        return false;
    }
    const auto& idx = _db.get_index_type<account_index>();
    const auto& aidx = dynamic_cast<const primary_index<account_index>&>(idx);
    const auto& refs = aidx.get_secondary_index<graphene::chain::account_member_index>();
    auto itr = refs.account_to_key_memberships.find(key);
    bool is_known = itr != refs.account_to_key_memberships.end();

    return is_known;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Accounts                                                         //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<optional<account_object>> database_api::get_accounts(const vector<account_id_type>& account_ids)const
{
   return my->get_accounts( account_ids );
}

vector<optional<account_object>> database_api::get_accounts_by_uid(const vector<account_uid_type>& account_uids)const
{
   return my->get_accounts_by_uid( account_uids );
}

vector<optional<account_object>> database_api_impl::get_accounts(const vector<account_id_type>& account_ids)const
{
   vector<optional<account_object>> result; result.reserve(account_ids.size());
   std::transform(account_ids.begin(), account_ids.end(), std::back_inserter(result),
                  [this](account_id_type id) -> optional<account_object> {
      if(auto o = _db.find(id))
      {
         subscribe_to_item( id );
         return *o;
      }
      return {};
   });
   return result;
}

vector<optional<account_object>> database_api_impl::get_accounts_by_uid(const vector<account_uid_type>& account_uids)const
{
   FC_ASSERT( account_uids.size() <= 100 );
   vector<optional<account_object>> result; result.reserve(account_uids.size());
   std::transform(account_uids.begin(), account_uids.end(), std::back_inserter(result),
                  [this](account_uid_type uid) -> optional<account_object> {
      if(auto o = _db.find_account_by_uid(uid))
      {
         //subscribe_to_item( id );
         return *o;
      }
      return {};
   });
   return result;
}

std::map<string,full_account> database_api::get_full_accounts( const vector<string>& names_or_ids, bool subscribe )
{
   return my->get_full_accounts( names_or_ids, subscribe );
}

std::map<std::string, full_account> database_api_impl::get_full_accounts( const vector<std::string>& names_or_ids, bool subscribe)
{
   //idump((names_or_ids));
   std::map<std::string, full_account> results;

   for (const std::string& account_name_or_id : names_or_ids)
   {
      const account_object* account = nullptr;
      if( graphene::utilities::is_number(account_name_or_id) )
      {
          account = _db.find_account_by_uid( fc::variant( account_name_or_id ).as<uint64_t>( 1 ) );
      }else if (std::isdigit(account_name_or_id[0]))
      {
          account = _db.find(fc::variant( account_name_or_id ).as<account_id_type>( 1 ));
      }else
      {
          const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
          auto itr = idx.find(account_name_or_id);
          if (itr != idx.end())
             account = &*itr;
      }

      if (account == nullptr)
         continue;

      if( subscribe )
      {
         FC_ASSERT( std::distance(_subscribed_accounts.begin(), _subscribed_accounts.end()) < 100 );
         _subscribed_accounts.insert( account->uid );
         subscribe_to_item( account->id );
      }

      // fc::mutable_variant_object full_account;
      full_account acnt;
      acnt.account = *account;
      acnt.statistics = _db.get_account_statistics_struct_by_uid(account->uid);//account->statistics(_db);
      auto reg = _db.find_account_by_uid( account->registrar );
      if(reg != nullptr)
         acnt.registrar_name = reg->name;
      auto ref = _db.find_account_by_uid( account->referrer );
      if(ref != nullptr)
         acnt.referrer_name = ref->name;
      auto lref = _db.find_account_by_uid( account->lifetime_referrer );
      if(lref != nullptr)
         acnt.lifetime_referrer_name = lref->name;

      // Add the account's proposals
      const auto& proposal_idx = _db.get_index_type<proposal_index>();
      const auto& pidx = dynamic_cast<const primary_index<proposal_index>&>(proposal_idx);
      const auto& proposals_by_account = pidx.get_secondary_index<graphene::chain::required_approval_index>();
      auto  required_approvals_itr = proposals_by_account._account_to_proposals.find( account->uid );
      if( required_approvals_itr != proposals_by_account._account_to_proposals.end() )
      {
         acnt.proposals.reserve( required_approvals_itr->second.size() );
         for( auto proposal_id : required_approvals_itr->second )
            acnt.proposals.push_back( proposal_id(_db) );
      }


      // Add the account's balances
      auto balance_range = _db.get_index_type<account_balance_index>().indices().get<by_account_asset>().equal_range(boost::make_tuple(account->uid));
      std::for_each(balance_range.first, balance_range.second,
                    [&acnt](const account_balance_object& balance) {
                       acnt.balances.emplace_back(balance);
                    });

      // get assets issued by user
      auto asset_range = _db.get_index_type<asset_index>().indices().get<by_issuer>().equal_range(account->uid);
      std::for_each(asset_range.first, asset_range.second,
                    [&acnt] (const asset_object& asset) {
                       acnt.assets.emplace_back(asset.asset_id);
                    });

      results[account_name_or_id] = acnt;
   }
   return results;
}

std::map<account_uid_type,full_account> database_api::get_full_accounts_by_uid( const vector<account_uid_type>& uids,
                                                                                const full_account_query_options& options )
{
   return my->get_full_accounts_by_uid( uids, options );
}

std::map<account_uid_type,full_account> database_api_impl::get_full_accounts_by_uid( const vector<account_uid_type>& uids,
                                                                                     const full_account_query_options& options )
{
   std::map<account_uid_type, full_account> results;

   for( const account_uid_type uid : uids )
   {
      const account_object* account = _db.find_account_by_uid( uid );
      if (account == nullptr)
         continue;

      const auto& account_stats = _db.get_account_statistics_struct_by_uid( uid );
      full_account acnt;
      if( options.fetch_account_object.valid() && *options.fetch_account_object == true )
         acnt.account = *account;
      if( options.fetch_statistics.valid() && *options.fetch_statistics == true )
         acnt.statistics = account_stats;
      if( options.fetch_csaf_leases_in.valid() && *options.fetch_csaf_leases_in == true )
         acnt.csaf_leases_in = get_csaf_leases_by_to( uid, 0, 100 );
      if( options.fetch_csaf_leases_out.valid() && *options.fetch_csaf_leases_out == true )
         acnt.csaf_leases_out = get_csaf_leases_by_from( uid, 0, 100 );
      if( options.fetch_voter_object.valid() && *options.fetch_voter_object == true && account_stats.is_voter )
         acnt.voter = *_db.find_voter( uid, account_stats.last_voter_sequence );
      // witness
      if( options.fetch_witness_object.valid() && *options.fetch_witness_object == true )
      {
         const witness_object* wit = _db.find_witness_by_uid( uid );
         if( wit != nullptr )
            acnt.witness = *wit;
      }
      if( options.fetch_witness_votes.valid() && *options.fetch_witness_votes == true && account_stats.is_voter )
      {
         auto range = _db.get_index_type<witness_vote_index>().indices().get<by_voter_seq>()
                         .equal_range( std::make_tuple( uid, account_stats.last_voter_sequence ) );
         std::for_each(range.first, range.second,
                    [&acnt] (const witness_vote_object& o) {
                       if( acnt.witness_votes.empty() || acnt.witness_votes.back() != o.witness_uid )
                          acnt.witness_votes.emplace_back( o.witness_uid );
                    });
      }
      // committee member
      if( options.fetch_committee_member_object.valid() && *options.fetch_committee_member_object == true )
      {
         const committee_member_object* com = _db.find_committee_member_by_uid( uid );
         if( com != nullptr )
            acnt.committee_member = *com;
      }
      if( options.fetch_committee_member_votes.valid() && *options.fetch_committee_member_votes == true && account_stats.is_voter )
      {
         auto range = _db.get_index_type<committee_member_vote_index>().indices().get<by_voter_seq>()
                         .equal_range( std::make_tuple( uid, account_stats.last_voter_sequence ) );
         std::for_each(range.first, range.second,
                    [&acnt] (const committee_member_vote_object& o) {
                       if( acnt.committee_member_votes.empty() || acnt.committee_member_votes.back() != o.committee_member_uid )
                          acnt.committee_member_votes.emplace_back( o.committee_member_uid );
                    });
      }
      // platform
      if( options.fetch_platform_object.valid() && *options.fetch_platform_object == true )
      {
         const platform_object* pf = _db.find_platform_by_owner( uid );
         if( pf != nullptr )
            acnt.platform = *pf;
      }
      if( options.fetch_platform_votes.valid() && *options.fetch_platform_votes == true && account_stats.is_voter )
      {
         auto range = _db.get_index_type<platform_vote_index>().indices().get<by_platform_voter_seq>()
                         .equal_range( std::make_tuple( uid, account_stats.last_voter_sequence ) );
         std::for_each(range.first, range.second,
                    [&acnt] (const platform_vote_object& o) {
                       if( acnt.platform_votes.empty() || acnt.platform_votes.back() != o.platform_owner )
                          acnt.platform_votes.emplace_back( o.platform_owner );
                    });
      }
      // get assets issued by user
      if( options.fetch_assets.valid() && *options.fetch_assets == true )
      {
         auto asset_range = _db.get_index_type<asset_index>().indices().get<by_issuer>()
                               .equal_range( account->uid );
         std::for_each(asset_range.first, asset_range.second,
                    [&acnt] (const asset_object& asset_obj) {
                       acnt.assets.emplace_back( asset_obj.asset_id );
                    });
      }
      // Add the account's balances
      if( options.fetch_balances.valid() && *options.fetch_balances == true )
      {
         auto balance_range = _db.get_index_type<account_balance_index>().indices().get<by_account_asset>()
                                 .equal_range( account->uid );
         std::for_each(balance_range.first, balance_range.second,
                    [&acnt](const account_balance_object& balance) {
                       acnt.balances.emplace_back( balance );
                    });
      }

      results[uid] = acnt;
   }
   return results;
}

vector<pledge_balance_object> database_api::get_account_core_asset_pledge(account_uid_type account_uid)const
{
   return my->get_account_core_asset_pledge(account_uid);
}

vector<pledge_balance_object> database_api_impl::get_account_core_asset_pledge(account_uid_type account_uid)const
{
   vector<pledge_balance_object> pledge_objs;
   const _account_statistics_object ant = _db.get_account_statistics_by_uid(account_uid);
   for (auto iter : ant.pledge_balance_ids){
      pledge_objs.emplace_back(_db.get<pledge_balance_object>(iter.second));
   }

   const auto& idx = _db.get_index_type<pledge_mining_index>().indices().get<by_pledge_account>();
   auto itr_mining = idx.lower_bound(account_uid);
   while (itr_mining != idx.end() && itr_mining->pledge_account == account_uid)
   {
      pledge_objs.emplace_back(_db.get<pledge_balance_object>(itr_mining->pledge_id));
      ++itr_mining;
   }

   return pledge_objs;
}

account_statistics_object database_api::get_account_statistics_by_uid(account_uid_type uid)const
{
    return my->get_account_statistics_by_uid(uid);
}

account_statistics_object database_api_impl::get_account_statistics_by_uid(account_uid_type uid)const
{
    account_statistics_object account_stats = _db.get_account_statistics_struct_by_uid(uid);
    return account_stats;
}

std::pair<fc::uint128_t, share_type> database_api::compute_coin_seconds_earned(const account_uid_type uid, const uint64_t window, const fc::time_point_sec now)const
{
   const _account_statistics_object ant = my->_db.get_account_statistics_by_uid(uid);
   auto para = my->_db.get_dynamic_global_properties();
   return ant.compute_coin_seconds_earned(window, now, my->_db, para.enabled_hardfork_version);
}

optional<account_object> database_api::get_account_by_name( string name )const
{
   return my->get_account_by_name( name );
}

optional<account_object> database_api_impl::get_account_by_name( string name )const
{
   const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
   auto itr = idx.find(name);
   if (itr != idx.end())
      return *itr;
   return optional<account_object>();
}

vector<account_uid_type> database_api::get_account_references( account_uid_type uid )const
{
   return my->get_account_references( uid );
}

vector<account_uid_type> database_api_impl::get_account_references( account_uid_type uid )const
{
   const auto& idx = _db.get_index_type<account_index>();
   const auto& aidx = dynamic_cast<const primary_index<account_index>&>(idx);
   const auto& refs = aidx.get_secondary_index<graphene::chain::account_member_index>();
   auto itr = refs.account_to_account_memberships.find(uid);
   vector<account_uid_type> result;

   if( itr != refs.account_to_account_memberships.end() )
   {
      result.reserve( itr->second.size() );
      for( auto item : itr->second ) result.push_back(item);
   }
   return result;
}

vector<optional<account_object>> database_api::lookup_account_names(const vector<string>& account_names)const
{
   return my->lookup_account_names( account_names );
}

vector<optional<account_object>> database_api_impl::lookup_account_names(const vector<string>& account_names)const
{
   const auto& accounts_by_name = _db.get_index_type<account_index>().indices().get<by_name>();
   vector<optional<account_object> > result;
   result.reserve(account_names.size());
   std::transform(account_names.begin(), account_names.end(), std::back_inserter(result),
                  [&accounts_by_name](const string& name) -> optional<account_object> {
      auto itr = accounts_by_name.find(name);
      return itr == accounts_by_name.end()? optional<account_object>() : *itr;
   });
   return result;
}

map<string,account_uid_type> database_api::lookup_accounts_by_name(const string& lower_bound_name, uint32_t limit)const
{
   return my->lookup_accounts_by_name( lower_bound_name, limit );
}

map<string,account_uid_type> database_api_impl::lookup_accounts_by_name(const string& lower_bound_name, uint32_t limit)const
{
   FC_ASSERT( limit <= 1001 );
   const auto& accounts_by_name = _db.get_index_type<account_index>().indices().get<by_name>();
   map<string,account_uid_type> result;

   for( auto itr = accounts_by_name.lower_bound(lower_bound_name);
        limit-- && itr != accounts_by_name.end();
        ++itr )
   {
      result.insert(make_pair(itr->name, itr->get_uid()));
      //if( limit == 1 )
      //   subscribe_to_item( itr->get_id() );
   }

   return result;
}

uint64_t database_api::get_account_auth_platform_count(const account_uid_type platform)const
{
   return my->get_account_auth_platform_count(platform);
}

uint64_t database_api_impl::get_account_auth_platform_count(const account_uid_type platform)const
{
   const auto& idx = _db.get_index_type<account_auth_platform_index>().indices().get<by_platform_account>();
   auto range = idx.equal_range(boost::make_tuple(platform));

   int count = boost::distance(range);

   return count;
}

vector<account_auth_platform_object> database_api::list_account_auth_platform_by_platform(const account_uid_type platform,
                                                                                          const account_uid_type lower_bound_account,
                                                                                          const uint32_t limit)const
{
   return my->list_account_auth_platform_by_platform(platform, lower_bound_account, limit);
}

vector<account_auth_platform_object> database_api_impl::list_account_auth_platform_by_platform(const account_uid_type platform,
                                                                                               const account_uid_type lower_bound_account,
                                                                                               const uint32_t limit)const
{
   FC_ASSERT(limit <= 100);
   vector<account_auth_platform_object> objs;
   const auto& idx = _db.get_index_type<account_auth_platform_index>().indices().get<by_platform_account>();
   auto itr = idx.lower_bound(std::make_tuple(platform, lower_bound_account));
   uint32_t count = 0;
   while (itr != idx.end() && itr->platform == platform && count < limit)
   {
      objs.emplace_back(*itr);
      ++itr;
      ++count;
   }
   return objs;
}

vector<account_auth_platform_object> database_api::list_account_auth_platform_by_account(const account_uid_type account,
                                                                                         const account_uid_type lower_bound_platform,
                                                                                         const uint32_t limit)const
{
   return my->list_account_auth_platform_by_account(account, lower_bound_platform, limit);
}

vector<account_auth_platform_object> database_api_impl::list_account_auth_platform_by_account(const account_uid_type account,
                                                                                              const account_uid_type lower_bound_platform,
                                                                                              const uint32_t limit)const
{
   FC_ASSERT(limit <= 100);
   vector<account_auth_platform_object> objs;
   const auto& idx = _db.get_index_type<account_auth_platform_index>().indices().get<by_account_platform>();
   auto itr = idx.lower_bound(std::make_tuple(account, lower_bound_platform));
   uint32_t count = 0;
   while (itr != idx.end() && itr->account == account && count < limit)
   {
      objs.emplace_back(*itr);
      ++itr;
      ++count;
   }
   return objs;
}


vector<pledge_mining_object> database_api::list_pledge_mining_by_witness(const account_uid_type witness,
   const account_uid_type lower_bound_account,
   const uint32_t limit)const
{
   return my->list_pledge_mining_by_witness(witness, lower_bound_account, limit);
}

vector<pledge_mining_object> database_api_impl::list_pledge_mining_by_witness(const account_uid_type witness,
   const account_uid_type lower_bound_account,
   const uint32_t limit)const
{
   FC_ASSERT(limit <= 100);
   vector<pledge_mining_object> result;
   const auto& idx = _db.get_index_type<pledge_mining_index>().indices().get<by_pledge_witness>();
   auto itr = idx.lower_bound(std::make_tuple(witness, lower_bound_account));
   uint32_t count = 0;
   while (itr != idx.end() && itr->witness == witness && count < limit)
   {
      result.emplace_back(*itr);
      ++itr;
      ++count;
   }
   return result;
}

vector<pledge_mining_object> database_api::list_pledge_mining_by_account(const account_uid_type account,
   const account_uid_type lower_bound_witness,
   const uint32_t limit)const
{
   return my->list_pledge_mining_by_account(account, lower_bound_witness, limit);
}

vector<pledge_mining_object> database_api_impl::list_pledge_mining_by_account(const account_uid_type account,
   const account_uid_type lower_bound_witness,
   const uint32_t limit)const
{
   FC_ASSERT(limit <= 100);
   vector<pledge_mining_object> result;
   const auto& idx = _db.get_index_type<pledge_mining_index>().indices().get<by_pledge_account>();
   auto itr = idx.lower_bound(std::make_tuple(account, lower_bound_witness));
   uint32_t count = 0;
   while (itr != idx.end() && itr->pledge_account == account && count < limit)
   {
      result.emplace_back(*itr);
      ++itr;
      ++count;
   }
   return result;
}

uint64_t database_api::get_account_count()const
{
   return my->get_account_count();
}

uint64_t database_api_impl::get_account_count()const
{
   return _db.get_index_type<account_index>().indices().size();
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// CSAF                                                             //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<csaf_lease_object> database_api::get_csaf_leases_by_from( const account_uid_type from,
                                                                 const account_uid_type lower_bound_to,
                                                                 const uint32_t limit )const
{
   return my->get_csaf_leases_by_from( from, lower_bound_to, limit );
}

vector<csaf_lease_object> database_api_impl::get_csaf_leases_by_from( const account_uid_type from,
                                                                      const account_uid_type lower_bound_to,
                                                                      const uint32_t limit )const
{
   FC_ASSERT( limit <= 1000 );

   vector<csaf_lease_object> result;

   uint32_t count = 0;

   const auto& idx = _db.get_index_type<csaf_lease_index>().indices().get<by_from_to>();
   auto itr = idx.lower_bound( std::make_tuple( from, lower_bound_to ) );

   while( itr != idx.end() && itr->from == from && count < limit )
   {
      result.push_back(*itr);
      ++itr;
      ++count;
   }

   return result;
}

vector<csaf_lease_object> database_api::get_csaf_leases_by_to( const account_uid_type to,
                                                               const account_uid_type lower_bound_from,
                                                               const uint32_t limit )const
{
   return my->get_csaf_leases_by_to( to, lower_bound_from, limit );
}

vector<csaf_lease_object> database_api_impl::get_csaf_leases_by_to( const account_uid_type to,
                                                                    const account_uid_type lower_bound_from,
                                                                    const uint32_t limit )const
{
   FC_ASSERT( limit <= 1000 );

   vector<csaf_lease_object> result;

   uint32_t count = 0;

   const auto& idx = _db.get_index_type<csaf_lease_index>().indices().get<by_to_from>();
   auto itr = idx.lower_bound( std::make_tuple( to, lower_bound_from ) );

   while( itr != idx.end() && itr->to == to && count < limit )
   {
      result.push_back(*itr);
      ++itr;
      ++count;
   }

   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Platforms and posts                                              //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<optional<platform_object>> database_api::get_platforms( const vector<account_uid_type>& account_uids )const
{
    return my->get_platforms( account_uids );
}

vector<optional<platform_object>> database_api_impl::get_platforms(const vector<account_uid_type>& platform_uids)const
{
   vector<optional<platform_object>> result; result.reserve(platform_uids.size());
   std::transform(platform_uids.begin(), platform_uids.end(), std::back_inserter(result),
                  [this](account_uid_type uid) -> optional<platform_object> {
                    if( auto o = _db.find_platform_by_owner( uid ) )
                        return *o;
                    return {};
                });
   return result;
}


fc::optional<platform_object> database_api::get_platform_by_account( account_uid_type account )const
{
    return my->get_platform_by_account( account );
}

fc::optional<platform_object> database_api_impl::get_platform_by_account(account_uid_type account) const
{
   const auto& idx = _db.get_index_type<platform_index>().indices().get<by_valid>();
   auto itr = idx.find( std::make_tuple( true, account ) );
   if( itr != idx.end() )
      return *itr;
   return {};
}


vector<platform_object> database_api::lookup_platforms( const account_uid_type lower_bound_uid,
                                              uint32_t limit, data_sorting_type order_by )const
{
    return my->lookup_platforms( lower_bound_uid, limit, order_by );
}

vector<platform_object> database_api_impl::lookup_platforms( const account_uid_type lower_bound_uid,
                                                            uint32_t limit,
                                                            data_sorting_type order_by )const
{
   FC_ASSERT( limit <= 101 );
   vector<platform_object> result;

   if( order_by == order_by_uid )
   {
      const auto& idx = _db.get_index_type<platform_index>().indices().get<by_valid>();
      auto itr = idx.lower_bound( std::make_tuple( true, lower_bound_uid ) );
      while( itr != idx.end() && limit > 0 ) // assume false < true
      {
         result.push_back( *itr );
         ++itr;
         --limit;
      }
   }
   else
   {
      account_uid_type new_lower_bound_uid = lower_bound_uid;
      const platform_object* lower_bound_obj = _db.find_platform_by_owner( lower_bound_uid );
      uint64_t lower_bound_shares = -1;
      if( lower_bound_obj == nullptr )
         new_lower_bound_uid = 0;
      else
      {
         if( order_by == order_by_votes )
            lower_bound_shares = lower_bound_obj->total_votes;
         else // by pledge
            lower_bound_shares = lower_bound_obj->pledge;
      }

      if( order_by == order_by_votes )
      {
         const auto& idx = _db.get_index_type<platform_index>().indices().get<by_platform_votes>();
         auto itr = idx.lower_bound( std::make_tuple( true, lower_bound_shares, new_lower_bound_uid ) );
         while( itr != idx.end() && limit > 0 ) // assume false < true
         {
            result.push_back( *itr );
            ++itr;
            --limit;
         }
      }
      else // by pledge
      {
         const auto& idx = _db.get_index_type<platform_index>().indices().get<by_platform_pledge>();
         auto itr = idx.lower_bound( std::make_tuple( true, lower_bound_shares, new_lower_bound_uid ) );
         while( itr != idx.end() && limit > 0 ) // assume false < true
         {
            result.push_back( *itr );
            ++itr;
            --limit;
         }
      }
   }

   return result;
}

uint64_t database_api::get_platform_count()const
{
   return my->get_platform_count();
}

uint64_t database_api_impl::get_platform_count()const
{
   return _db.get_index_type< platform_index >().indices().get< by_valid >().count(true);
}

optional<post_object> database_api::get_post(const account_uid_type platform_owner,
                                             const account_uid_type poster_uid,
                                             const post_pid_type post_pid )const
{
   return my->get_post(platform_owner, poster_uid, post_pid);
}

optional<post_object> database_api_impl::get_post(const account_uid_type platform_owner,
                                                  const account_uid_type poster_uid,
                                                  const post_pid_type post_pid )const
{
   if (auto o = _db.find_post_by_platform(platform_owner, poster_uid, post_pid))
   {
      return *o;
   }
   return{};
}

optional<score_object> database_api::get_score(const account_uid_type platform,
                                               const account_uid_type poster_uid,
                                               const post_pid_type post_pid,
                                               const account_uid_type from_account)const
{
   return my->get_score(platform, poster_uid, post_pid, from_account);
}

optional<score_object> database_api_impl::get_score(const account_uid_type platform,
                                                    const account_uid_type poster_uid,
                                                    const post_pid_type    post_pid,
                                                    const account_uid_type from_account)const
{
   if (auto o = _db.find_score(platform, poster_uid, post_pid, from_account))
   {
      return *o;
   }
   return{};
}

vector<score_object> database_api::get_scores_by_uid(const account_uid_type  scorer,
                                                     const uint32_t period,
                                                     const object_id_type lower_bound_score,
                                                     const uint32_t limit)const
{
   return my->get_scores_by_uid(scorer, period, lower_bound_score, limit);
}

vector<score_object> database_api_impl::get_scores_by_uid(const account_uid_type  scorer,
                                                          const uint32_t period,
                                                          const object_id_type lower_bound_score,
                                                          const uint32_t limit)const
{
   FC_ASSERT(limit <= 100);
   vector<score_object> result;
   uint32_t count = 0;

   const auto& sce_idx = _db.get_index_type<score_index>().indices().get<by_from_account_uid>();
   auto itr = sce_idx.lower_bound(std::make_tuple(scorer, period, lower_bound_score));

   while (itr != sce_idx.end() && count < limit &&
      itr->from_account_uid == scorer && itr->period_sequence == period)
   {
      result.push_back(*itr);
      ++itr;
      ++count;
   }

   return result;
}

vector<score_object> database_api::list_scores(const account_uid_type platform,
                                               const account_uid_type poster_uid,
                                               const post_pid_type    post_pid,
                                               const object_id_type   lower_bound_score,
                                               const uint32_t         limit,
                                               const bool             list_cur_period)const
{
   return my->list_scores(platform, poster_uid, post_pid, lower_bound_score, limit, list_cur_period);
}

vector<score_object> database_api_impl::list_scores(const account_uid_type platform,
                                                    const account_uid_type poster_uid,
                                                    const post_pid_type    post_pid,
                                                    const object_id_type   lower_bound_score,
                                                    const uint32_t         limit,
                                                    const bool             list_cur_period)const
{
   FC_ASSERT(limit <= 100);
   vector<score_object> result;
   uint32_t count = 0;

   if (list_cur_period){
      const dynamic_global_property_object& dpo = _db.get_dynamic_global_properties();
      const auto& idx = _db.get_index_type<score_index>().indices().get<by_period_sequence>();
      auto itr_begin = idx.lower_bound(std::make_tuple(platform, poster_uid, post_pid, dpo.current_active_post_sequence, lower_bound_score));

      while (count < limit
         && itr_begin->platform == platform && itr_begin->poster == poster_uid
         && itr_begin->post_pid == post_pid &&itr_begin->period_sequence == dpo.current_active_post_sequence)
      {
         result.push_back(*itr_begin);
         ++count;
         if (itr_begin == idx.begin()) break;
         --itr_begin;
      }
   }
   else{
      const auto& idx = _db.get_index_type<score_index>().indices().get<by_posts_pids>();
      auto itr_begin = idx.lower_bound(std::make_tuple(platform, poster_uid, post_pid, lower_bound_score));
      while (itr_begin != idx.end() && count < limit
         && itr_begin->platform == platform && itr_begin->poster == poster_uid && itr_begin->post_pid == post_pid)
      {
         result.push_back(*itr_begin);
         ++itr_begin;
         ++count;
      }
   }
   return result;
}

optional<license_object> database_api::get_license(const account_uid_type platform, const license_lid_type license_lid)const
{
   return my->get_license(platform, license_lid);
}

optional<license_object> database_api_impl::get_license(const account_uid_type platform,
                                                        const license_lid_type license_lid)const
{
   if (auto o = _db.find_license_by_platform(platform, license_lid))
   {
      return *o;
   }
   return{};
}

vector<license_object> database_api::list_licenses(const account_uid_type platform, const object_id_type lower_bound_license, const uint32_t limit)const
{
   return my->list_licenses(platform, lower_bound_license, limit);
}

vector<license_object> database_api_impl::list_licenses(const account_uid_type platform, const object_id_type lower_bound_license, const uint32_t limit)const
{
   FC_ASSERT(limit <= 100);
   vector<license_object> result;
   uint32_t count = 0;
   const auto& idx = _db.get_index_type<license_index>().indices().get<by_platform>();
   auto itr_begin = idx.lower_bound(std::make_tuple(platform, lower_bound_license));

   while (itr_begin != idx.end() && count < limit && itr_begin->platform == platform)
   {
      result.push_back(*itr_begin);
      ++itr_begin;
      ++count;
   }
   return result;
}

vector<advertising_object> database_api::list_advertisings(const account_uid_type platform, const advertising_aid_type lower_bound_advertising, const uint32_t limit)const
{
   return my->list_advertisings(platform, lower_bound_advertising, limit);
}

vector<advertising_object> database_api_impl::list_advertisings(const account_uid_type platform, const advertising_aid_type lower_bound_advertising, const uint32_t limit)const
{
   FC_ASSERT(limit <= 100);
   vector<advertising_object> result;
   uint32_t count = 0;
   const auto& idx = _db.get_index_type<advertising_index>().indices().get<by_advertising_platform>();
   auto itr_begin = idx.lower_bound(std::make_tuple(platform, lower_bound_advertising));

   while (itr_begin != idx.end() && count < limit && itr_begin->platform == platform)
   {
      result.push_back(*itr_begin);
      ++itr_begin;
      ++count;
   }
   return result;
}

optional<advertising_object> database_api::get_advertising(const account_uid_type platform, const advertising_aid_type advertising_aid)const
{
   return my->get_advertising(platform, advertising_aid);
}

optional<advertising_object> database_api_impl::get_advertising(const account_uid_type platform, const advertising_aid_type advertising_aid)const
{
   if (auto o = _db.find_advertising(platform, advertising_aid))
   {
      return *o;
   }
   return{};
}

vector<advertising_order_object> database_api::list_advertising_orders_by_purchaser(const account_uid_type purchaser, const object_id_type lower_bound_advertising_order, uint32_t limit)const
{
   return my->list_advertising_orders_by_purchaser(purchaser, lower_bound_advertising_order, limit);
}

vector<advertising_order_object> database_api_impl::list_advertising_orders_by_purchaser(const account_uid_type purchaser, const object_id_type lower_bound_advertising_order, uint32_t limit)const
{
   FC_ASSERT(limit <= 100);
   vector<advertising_order_object> result;

   const auto& idx = _db.get_index_type<advertising_order_index>().indices().get<by_advertising_user_id>();
   auto itr = idx.lower_bound(std::make_tuple(purchaser, lower_bound_advertising_order));

   while (itr != idx.end() && itr->user == purchaser && limit--)
   {
      result.push_back(*itr);
      ++itr;
   }
   return result;
}

vector<advertising_order_object> database_api::list_advertising_orders_by_ads_aid(const account_uid_type platform,
                                                                                  const advertising_aid_type id,
                                                                                  const advertising_order_oid_type lower_bound_advertising_order,
                                                                                  uint32_t limit)const
{
   return my->list_advertising_orders_by_ads_aid(platform, id, lower_bound_advertising_order, limit);
}

vector<advertising_order_object> database_api_impl::list_advertising_orders_by_ads_aid(const account_uid_type platform,
                                                                                       const advertising_aid_type id,
                                                                                       const advertising_order_oid_type lower_bound_advertising_order,
                                                                                       uint32_t limit)const
{
   FC_ASSERT(limit <= 100);
   vector<advertising_order_object> result;

   const auto& idx = _db.get_index_type<advertising_order_index>().indices().get<by_advertising_order_oid>();
   auto itr = idx.lower_bound(std::make_tuple(platform, id, lower_bound_advertising_order));
   while (itr != idx.end() && itr->advertising_aid == id && itr->platform == platform && limit--)
   {
      if (!(itr->advertising_order_oid < lower_bound_advertising_order))
         result.push_back(*itr);
      ++itr;
   }
   return result;
}

vector<custom_vote_object> database_api::lookup_custom_votes(const account_uid_type creator, const custom_vote_vid_type lower_bound_custom_vote, uint32_t limit)const
{
    return my->lookup_custom_votes(creator, lower_bound_custom_vote, limit);
}

vector<custom_vote_object> database_api_impl::lookup_custom_votes(const account_uid_type creator, const custom_vote_vid_type lower_bound_custom_vote, uint32_t limit)const
{
   FC_ASSERT(limit <= 100);
   vector<custom_vote_object> result;
   const auto& idx = _db.get_index_type<custom_vote_index>().indices().get<by_creater>();
   auto itr = idx.lower_bound(std::make_tuple(creator, lower_bound_custom_vote));
   while (itr != idx.end() && limit-- && itr->custom_vote_creator == creator)
   {
      result.push_back(*itr);
      ++itr;
   }
   return result;
}

vector<custom_vote_object> database_api::list_custom_votes(optional<custom_vote_id_type> lower_bound_custom_vote_id, optional<bool> is_finished, uint32_t limit)const
{
   return my->list_custom_votes(lower_bound_custom_vote_id, is_finished, limit);
}

vector<custom_vote_object> database_api_impl::list_custom_votes(optional<custom_vote_id_type> lower_bound_custom_vote_id, optional<bool> is_finished, uint32_t limit)const
{
   FC_ASSERT(limit <= 100);
   vector<custom_vote_object> result;
   const auto& idx = _db.get_index_type<custom_vote_index>().indices().get<by_new>();
   auto itr = lower_bound_custom_vote_id.valid() ? idx.lower_bound(*lower_bound_custom_vote_id) : idx.begin();
   if (is_finished.valid()){
      if (*is_finished){
         while (itr != idx.end() && limit--)
         {
            if (itr->vote_expired_time <= _db.head_block_time())
               result.push_back(*itr);
            ++itr;
         }
      }
      else{
         while (itr != idx.end() && limit--)
         {
            if (itr->vote_expired_time > _db.head_block_time())
               result.push_back(*itr);
            ++itr;
         }
      }
   }
   else{
      while (itr != idx.end() && limit--)
      {
         result.push_back(*itr);
         ++itr;
      }
   }
   return result;
}

vector<cast_custom_vote_object> database_api::list_cast_custom_votes_by_id(const account_uid_type creator, 
                                                                           const custom_vote_vid_type vote_vid, 
                                                                           const object_id_type lower_bound_cast_custom_vote, 
                                                                           uint32_t limit)const
{
   return my->list_cast_custom_votes_by_id(creator, vote_vid, lower_bound_cast_custom_vote, limit);
}

vector<cast_custom_vote_object> database_api_impl::list_cast_custom_votes_by_id(const account_uid_type creator, 
                                                                                const custom_vote_vid_type vote_vid, 
                                                                                const object_id_type lower_bound_cast_custom_vote, 
                                                                                uint32_t limit)const
{
   FC_ASSERT(limit <= 100);
   vector<cast_custom_vote_object> result;

   const auto& idx = _db.get_index_type<cast_custom_vote_index>().indices().get<by_custom_vote_vid>();
   auto itr = idx.lower_bound(std::make_tuple(creator, vote_vid, lower_bound_cast_custom_vote));

   while (itr != idx.end() && limit-- &&
      itr->custom_vote_creator == creator &&
      itr->custom_vote_vid == vote_vid)
   {
      result.push_back(*itr);
      ++itr;
   }
   return result;
}

vector<cast_custom_vote_object> database_api::list_cast_custom_votes_by_voter(const account_uid_type voter, const object_id_type lower_bound_cast_custom_vote, uint32_t limit)const
{
   return my->list_cast_custom_votes_by_voter(voter, lower_bound_cast_custom_vote, limit);
}

vector<cast_custom_vote_object> database_api_impl::list_cast_custom_votes_by_voter(const account_uid_type voter, const object_id_type lower_bound_cast_custom_vote, uint32_t limit)const
{
   FC_ASSERT(limit <= 100);
   vector<cast_custom_vote_object> result;

   const auto& idx = _db.get_index_type<cast_custom_vote_index>().indices().get<by_cast_custom_vote_id>();
   auto itr = idx.lower_bound(std::make_tuple(voter, lower_bound_cast_custom_vote));

   while (itr != idx.end() && limit-- && itr->voter == voter)
   {
      result.push_back(*itr);
      ++itr;
   }
   return result;
}

vector<active_post_object> database_api::get_post_profits_detail(const uint32_t         begin_period,
                                                                 const uint32_t         end_period,
                                                                 const account_uid_type platform,
                                                                 const account_uid_type poster,
                                                                 const post_pid_type    post_pid)const
{
   return my->get_post_profits_detail(begin_period, end_period, platform, poster, post_pid);
}

vector<active_post_object> database_api_impl::get_post_profits_detail(const uint32_t         begin_period,
                                                                      const uint32_t         end_period,
                                                                      const account_uid_type platform,
                                                                      const account_uid_type poster,
                                                                      const post_pid_type    post_pid)const
{
   FC_ASSERT(begin_period <= end_period);
   FC_ASSERT(end_period - begin_period <= 100);

   vector<active_post_object> vtr_active_objects;
   const auto& idx = _db.get_index_type<active_post_index>().indices().get<by_post>();
   auto itr_begin = idx.lower_bound(std::make_tuple(platform, poster, post_pid, begin_period));

   while (itr_begin != idx.end() && itr_begin->platform == platform
      && itr_begin->poster == poster && itr_begin->post_pid == post_pid
      && (itr_begin->period_sequence >= begin_period && itr_begin->period_sequence <= end_period))
   {
      vtr_active_objects.push_back(*itr_begin);
      ++itr_begin;
   }
   return vtr_active_objects;
}

vector<Platform_Period_Profit_Detail> database_api::get_platform_profits_detail(const uint32_t         begin_period,
                                                                                const uint32_t         end_period,
                                                                                const account_uid_type platform,
                                                                                const uint32_t         lower_bound_index,
                                                                                uint32_t               limit)const
{
   return my->get_platform_profits_detail(begin_period, end_period, platform, lower_bound_index, limit);
}

vector<Platform_Period_Profit_Detail> database_api_impl::get_platform_profits_detail(const uint32_t         begin_period,
                                                                                     const uint32_t         end_period,
                                                                                     const account_uid_type platform,
                                                                                     const uint32_t         lower_bound_index,
                                                                                     uint32_t               limit)const
{
   FC_ASSERT(begin_period <= end_period);
   FC_ASSERT(end_period - begin_period <= 100);
   FC_ASSERT(limit <= 100);
   uint32_t begin_index = 0;
   vector<Platform_Period_Profit_Detail> vtr_profit_details;
   for (int i = begin_period; i <= end_period; ++i)
   {
      const platform_object& platform_obj = _db.get_platform_by_owner(platform);
      auto iter_profit = platform_obj.period_profits.find(i);

      if (iter_profit != platform_obj.period_profits.end()){
         Platform_Period_Profit_Detail detail;
         detail.cur_period = i;
         detail.platform_account = platform;
         detail.platform_name = platform_obj.name;
         detail.foward_profits = iter_profit->second.foward_profits;
         detail.post_profits = iter_profit->second.post_profits;
         detail.post_profits_by_platform = iter_profit->second.post_profits_by_platform;
         detail.platform_profits = iter_profit->second.platform_profits;
         detail.rewards_profits = iter_profit->second.rewards_profits;

         const auto& idx = _db.get_index_type<active_post_index>().indices().get<by_platforms>();
         auto itr_begin = idx.lower_bound(std::make_tuple(platform, i));
         while (itr_begin != idx.end() && itr_begin->platform == platform && itr_begin->period_sequence == i && limit)
         {
            if (begin_index >= lower_bound_index && itr_begin->is_get_profit()){
               detail.active_objects.push_back(*itr_begin);
               --limit;
            }
            ++itr_begin;
            ++begin_index;
         }
         vtr_profit_details.emplace_back(detail);
      }
   }
   return vtr_profit_details;
}

vector<Poster_Period_Profit_Detail> database_api::get_poster_profits_detail(const uint32_t         begin_period,
                                                                            const uint32_t         end_period,
                                                                            const account_uid_type poster,
                                                                            const uint32_t         lower_bound_index,
                                                                            uint32_t               limit)const
{
   return my->get_poster_profits_detail(begin_period, end_period, poster, lower_bound_index, limit);
}

vector<Poster_Period_Profit_Detail> database_api_impl::get_poster_profits_detail(const uint32_t         begin_period,
                                                                                 const uint32_t         end_period,
                                                                                 const account_uid_type poster,
                                                                                 const uint32_t         lower_bound_index,
                                                                                 uint32_t               limit)const
{
   FC_ASSERT(begin_period <= end_period);
   FC_ASSERT(end_period - begin_period <= 100);
   FC_ASSERT(limit <= 100);
   uint32_t begin_index = 0;
   vector<Poster_Period_Profit_Detail> result;
   const auto& apt_idx = _db.get_index_type<active_post_index>().indices().get<by_poster>();

   uint32_t start = begin_period;
   while (start <= end_period)
   {
      Poster_Period_Profit_Detail ppd;
      ppd.cur_period = start;
      ppd.poster_account = poster;

      auto itr = apt_idx.lower_bound(std::make_tuple(poster, start));
      bool exist = false;

      while (itr != apt_idx.end() && itr->receiptor_details.count(poster)
         && itr->period_sequence == start && itr->poster == poster)
      {
         ppd.total_forward += itr->receiptor_details.at(poster).forward;
         ppd.total_post_award += itr->receiptor_details.at(poster).post_award;
         if (begin_index >= lower_bound_index && limit) {
            ppd.active_objects.push_back(*itr);
            --limit;
         }

         for (const auto& r : itr->receiptor_details.at(poster).rewards)
         {
            if (ppd.total_rewards.count(r.first))
               ppd.total_rewards[r.first] += r.second;
            else
               ppd.total_rewards.emplace(r.first, r.second);
         }
         if (!exist)
            exist = true;
         ++itr;
         ++begin_index;
      }

      if (exist)
         result.push_back(ppd);
      ++start;
   }

   return result;
}

uint64_t database_api::get_posts_count(optional<account_uid_type> platform, optional<account_uid_type> poster)const
{
   return my->get_posts_count(platform, poster);
}

uint64_t database_api_impl::get_posts_count(optional<account_uid_type> platform, optional<account_uid_type> poster)const
{
   const auto& post_idx = _db.get_index_type<post_index>().indices().get<by_post_pid>();
   if (platform.valid()) {
      if (poster.valid())
         return post_idx.count(std::make_tuple(*platform, *poster));
      else
         return post_idx.count(*platform);
   }
   else {
      if (poster.valid())
         FC_ASSERT(false, "platform should be valid when poster is valid");
      else
         return post_idx.size();
   }
}

share_type database_api::get_score_profit(account_uid_type account, uint32_t period)const
{
   return my->get_score_profit(account, period);
}

share_type database_api_impl::get_score_profit(account_uid_type account, uint32_t period)const
{
   const auto& sce_idx = _db.get_index_type<score_index>().indices().get<by_from_account_uid>();
   auto itr = sce_idx.lower_bound(std::make_tuple(account, period));

   share_type amount = 0;
   while (itr != sce_idx.end() && itr->from_account_uid == account && itr->period_sequence == period)
   {
      amount += itr->profits;
      ++itr;
   }

   return amount;
}

vector<post_object> database_api::get_posts_by_platform_poster( const account_uid_type platform_owner,
                                      optional<account_uid_type> poster,
                                      const object_id_type lower_bound_post,
                                      const uint32_t limit )const
{
   return my->get_posts_by_platform_poster(platform_owner, poster, lower_bound_post, limit);
}

vector<post_object> database_api_impl::get_posts_by_platform_poster( const account_uid_type platform_owner,
                                      optional<account_uid_type> poster,
                                      const object_id_type lower_bound_post,
                                      const uint32_t limit )const
{
   FC_ASSERT(limit <= 100);

   vector<post_object> result;
   uint32_t count = 0;

   if (poster.valid()){
      const auto& post_idx = _db.get_index_type<post_index>().indices().get<by_platform_poster>();
      auto itr = post_idx.lower_bound(std::make_tuple(platform_owner, *poster, lower_bound_post));
      while (itr != post_idx.end() && count < limit && itr->platform == platform_owner && itr->poster == *poster)
      {
         result.push_back(*itr);
         ++itr;
         ++count;
      }
   }
   else{
      const auto& post_idx = _db.get_index_type<post_index>().indices().get<by_platform_id>();
      auto itr = post_idx.lower_bound(std::make_tuple(platform_owner, lower_bound_post));
      while (itr != post_idx.end() && count < limit && itr->platform == platform_owner)
      {
         result.push_back(*itr);
         ++itr;
         ++count;
      }
   }
   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Balances                                                         //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<asset> database_api::get_account_balances(account_uid_type uid, const flat_set<asset_aid_type>& assets)const
{
   return my->get_account_balances(uid, assets);
}

vector<asset> database_api_impl::get_account_balances(account_uid_type acnt, const flat_set<asset_aid_type>& assets)const
{
   vector<asset> result;
   if (assets.empty())
   {
      // if the caller passes in an empty list of assets, return balances for all assets the account owns
      const account_balance_index& balance_index = _db.get_index_type<account_balance_index>();
      auto range = balance_index.indices().get<by_account_asset>().equal_range(boost::make_tuple(acnt));
      for (const account_balance_object& balance : boost::make_iterator_range(range.first, range.second))
         result.push_back(asset(balance.get_balance()));
   }
   else
   {
      result.reserve(assets.size());

      std::transform(assets.begin(), assets.end(), std::back_inserter(result),
         [this, acnt](asset_aid_type id) { return _db.get_balance(acnt, id); });
   }

   return result;
}

vector<asset> database_api::get_named_account_balances(const std::string& name, const flat_set<asset_aid_type>& assets)const
{
   return my->get_named_account_balances(name, assets);
}

vector<asset> database_api_impl::get_named_account_balances(const std::string& name, const flat_set<asset_aid_type>& assets) const
{
   const auto& accounts_by_name = _db.get_index_type<account_index>().indices().get<by_name>();
   auto itr = accounts_by_name.find(name);
   FC_ASSERT(itr != accounts_by_name.end());
   return get_account_balances(itr->get_uid(), assets);
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Assets                                                           //
//                                                                  //
//////////////////////////////////////////////////////////////////////
asset_aid_type database_api::get_asset_id_from_string(const std::string& symbol_or_id)const
{
   return my->get_asset_from_string(symbol_or_id)->asset_id;
}

vector<optional<asset_object_with_data>> database_api::get_assets(const vector<asset_aid_type>& asset_ids)const
{
   return my->get_assets(asset_ids);
}

vector<optional<asset_object_with_data>> database_api_impl::get_assets(const vector<asset_aid_type>& asset_ids)const
{
   vector<optional<asset_object_with_data>> result; result.reserve(asset_ids.size());
   std::transform(asset_ids.begin(), asset_ids.end(), std::back_inserter(result),
      [this](asset_aid_type id) -> optional<asset_object_with_data> {

      const auto& idx = _db.get_index_type<asset_index>().indices().get<by_aid>();
      auto itr = idx.find(id);
      if (itr != idx.end())
      {
         subscribe_to_item((*itr).id);
         asset_object_with_data aod(*itr);
         aod.dynamic_asset_data = itr->dynamic_data(_db);
         return aod;
      }

      return{};
   });
   return result;
}

vector<asset_object_with_data> database_api::list_assets(const string& lower_bound_symbol, uint32_t limit)const
{
   return my->list_assets(lower_bound_symbol, limit);
}

vector<asset_object_with_data> database_api_impl::list_assets(const string& lower_bound_symbol, uint32_t limit)const
{
   FC_ASSERT( limit <= 101 );
   const auto& assets_by_symbol = _db.get_index_type<asset_index>().indices().get<by_symbol>();
   vector<asset_object_with_data> result;
   result.reserve(limit);

   auto itr = assets_by_symbol.lower_bound(lower_bound_symbol);

   if( lower_bound_symbol == "" )
      itr = assets_by_symbol.begin();

   while(limit-- && itr != assets_by_symbol.end())
   {
      result.emplace_back(*itr++);
      result.back().dynamic_asset_data = result.back().dynamic_data( _db );
   }

   return result;
}

vector<optional<asset_object_with_data>> database_api::lookup_asset_symbols(const vector<string>& symbols_or_ids)const
{
   return my->lookup_asset_symbols(symbols_or_ids);
}

vector<optional<asset_object_with_data>> database_api_impl::lookup_asset_symbols(const vector<string>& symbols_or_ids)const
{
   const auto& assets_by_symbol = _db.get_index_type<asset_index>().indices().get<by_symbol>();
   vector<optional<asset_object_with_data> > result;
   result.reserve(symbols_or_ids.size());
   std::transform(symbols_or_ids.begin(), symbols_or_ids.end(), std::back_inserter(result),
      [this, &assets_by_symbol](const string& symbol_or_id) -> optional<asset_object_with_data> {
      if (symbol_or_id.empty())
         return{};
      if (symbol_or_id[0] >= '0' && symbol_or_id[0] <= '9')
      {
         auto ptr = _db.find_asset_by_aid(variant(symbol_or_id).as<asset_aid_type>(1));
         if (ptr == nullptr)
            return{};
         asset_object_with_data aod(*ptr);
         aod.dynamic_asset_data = aod.dynamic_data(_db);
         return aod;
      }
      auto itr = assets_by_symbol.find(symbol_or_id);
      if (itr == assets_by_symbol.end())
         return{};
      asset_object_with_data aod(*itr);
      aod.dynamic_asset_data = aod.dynamic_data(_db);
      return aod;
   });
   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Markets / feeds                                                  //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<limit_order_object> database_api::get_limit_orders(std::string a, std::string b, uint32_t limit)const
{
   return my->get_limit_orders(a, b, limit);
}

/**
*  @return the limit orders for both sides of the book for the two assets specified up to limit number on each side.
*/
vector<limit_order_object> database_api_impl::get_limit_orders(const std::string& a, const std::string& b, uint32_t limit)const
{
   FC_ASSERT(limit <= 300);
   const asset_aid_type asset_a_id = get_asset_from_string(a)->asset_id;
   const asset_aid_type asset_b_id = get_asset_from_string(b)->asset_id;

   return get_limit_orders(asset_a_id, asset_b_id, limit);
}

vector<limit_order_object> database_api::get_account_limit_orders(const string& account_name_or_id, const string &base,
   const string &quote, uint32_t limit, optional<limit_order_id_type> ostart_id, optional<price> ostart_price)
{
   return my->get_account_limit_orders(account_name_or_id, base, quote, limit, ostart_id, ostart_price);
}

vector<limit_order_object> database_api_impl::get_account_limit_orders(const string& account_name_or_id, const string &base,
   const string &quote, uint32_t limit, optional<limit_order_id_type> ostart_id, optional<price> ostart_price)
{
   FC_ASSERT(limit <= 101);

   vector<limit_order_object>   results;
   uint32_t                     count = 0;

   const account_object* account = get_account_from_string(account_name_or_id);
   if (account == nullptr)
      return results;

   auto assets = lookup_asset_symbols({ base, quote });
   FC_ASSERT(assets[0], "Invalid base asset symbol: ${s}", ("s", base));
   FC_ASSERT(assets[1], "Invalid quote asset symbol: ${s}", ("s", quote));

   auto base_id = assets[0]->asset_id;
   auto quote_id = assets[1]->asset_id;

   if (ostart_price.valid()) {
      FC_ASSERT(ostart_price->base.asset_id == base_id, "Base asset inconsistent with start price");
      FC_ASSERT(ostart_price->quote.asset_id == quote_id, "Quote asset inconsistent with start price");
   }

   const auto& index_by_account = _db.get_index_type<limit_order_index>().indices().get<by_account>();
   limit_order_multi_index_type::index<by_account>::type::const_iterator lower_itr;
   limit_order_multi_index_type::index<by_account>::type::const_iterator upper_itr;

   // if both order_id and price are invalid, query the first page
   if (!ostart_id.valid() && !ostart_price.valid())
   {
      lower_itr = index_by_account.lower_bound(std::make_tuple(account->uid, price::max(base_id, quote_id)));
   }
   else if (ostart_id.valid())
   {
      // in case of the order been deleted during page querying
      const limit_order_object *p_loo = _db.find(*ostart_id);

      if (!p_loo)
      {
         if (ostart_price.valid())
         {
            lower_itr = index_by_account.lower_bound(std::make_tuple(account->uid, *ostart_price, *ostart_id));
         }
         else
         {
            // start order id been deleted, yet not provided price either
            FC_THROW("Order id invalid (maybe just been canceled?), and start price not provided");
         }
      }
      else
      {
         const limit_order_object &loo = *p_loo;

         // in case of the order not belongs to specified account or market
         FC_ASSERT(loo.sell_price.base.asset_id == base_id, "Order base asset inconsistent");
         FC_ASSERT(loo.sell_price.quote.asset_id == quote_id, "Order quote asset inconsistent with order");
         FC_ASSERT(loo.seller == account->get_uid(), "Order not owned by specified account");

         lower_itr = index_by_account.lower_bound(std::make_tuple(account->uid, loo.sell_price, *ostart_id));
      }
   }
   else
   {
      // if reach here start_price must be valid
      lower_itr = index_by_account.lower_bound(std::make_tuple(account->uid, *ostart_price));
   }

   upper_itr = index_by_account.upper_bound(std::make_tuple(account->uid, price::min(base_id, quote_id)));

   // Add the account's orders
   for (; lower_itr != upper_itr && count < limit; ++lower_itr, ++count)
   {
      const limit_order_object &order = *lower_itr;
      results.emplace_back(order);
   }

   return results;
}

vector<limit_order_object> database_api::get_account_all_limit_orders(const string& account_name_or_id, uint32_t limit, optional<limit_order_id_type> ostart_id)
{
   return my->get_account_all_limit_orders(account_name_or_id, limit, ostart_id);
}

vector<limit_order_object> database_api_impl::get_account_all_limit_orders(const string& account_name_or_id, uint32_t limit, optional<limit_order_id_type> ostart_id)
{
   FC_ASSERT(limit <= 101);

   vector<limit_order_object>   results;
   uint32_t                     count = 0;

   const account_object* account = get_account_from_string(account_name_or_id);
   if (account == nullptr)
      return results;

   const auto& index_by_account = _db.get_index_type<limit_order_index>().indices().get<by_account_id>();
   limit_order_multi_index_type::index<by_account_id>::type::const_iterator lower_itr;

   // if both order_id and price are invalid, query the first page
   if (!ostart_id.valid())
   {
      lower_itr = index_by_account.lower_bound(account->uid);
   }
   else if (ostart_id.valid())
   {
      // in case of the order been deleted during page querying
      const limit_order_object *p_loo = _db.find(*ostart_id);
      if (!p_loo)
      {
         FC_THROW("Order id invalid (maybe just been canceled?)");
      }
      else
      {
         const limit_order_object &loo = *p_loo;
         FC_ASSERT(loo.seller == account->get_uid(), "Order not owned by specified account");
         lower_itr = index_by_account.lower_bound(std::make_tuple(account->uid, *ostart_id));
      }
   }
   for (; lower_itr != index_by_account.end() && count < limit; ++lower_itr, ++count)
   {
      if (lower_itr->seller != account->uid) break;
      const limit_order_object &order = *lower_itr;
      results.emplace_back(order);
   }
   return results;
}

void database_api::subscribe_to_market(std::function<void(const variant&)> callback, const std::string& a, const std::string& b)
{
   my->subscribe_to_market(callback, a, b);
}

void database_api_impl::subscribe_to_market(std::function<void(const variant&)> callback, const std::string& a, const std::string& b)
{
   auto asset_a_id = get_asset_from_string(a)->asset_id;
   auto asset_b_id = get_asset_from_string(b)->asset_id;

   if (asset_a_id > asset_b_id) std::swap(asset_a_id, asset_b_id);
   FC_ASSERT(asset_a_id != asset_b_id);
   _market_subscriptions[std::make_pair(asset_a_id, asset_b_id)] = callback;
}

void database_api::unsubscribe_from_market(const std::string& a, const std::string& b)
{
   my->unsubscribe_from_market(a, b);
}

void database_api_impl::unsubscribe_from_market(const std::string& a, const std::string& b)
{
   auto asset_a_id = get_asset_from_string(a)->asset_id;
   auto asset_b_id = get_asset_from_string(b)->asset_id;

   if (a > b) std::swap(asset_a_id, asset_b_id);
   FC_ASSERT(asset_a_id != asset_b_id);
   _market_subscriptions.erase(std::make_pair(asset_a_id, asset_b_id));
}

string database_api_impl::price_to_string(const price& _price, const asset_object& _base, const asset_object& _quote)
{
   try {
      if (_price.base.asset_id == _base.asset_id && _price.quote.asset_id == _quote.asset_id)
         return graphene::app::price_to_string(_price, _base.precision, _quote.precision);
      else if (_price.base.asset_id == _quote.asset_id && _price.quote.asset_id == _base.asset_id)
         return graphene::app::price_to_string(~_price, _base.precision, _quote.precision);
      else
         FC_ASSERT(!"bad parameters");
   } FC_CAPTURE_AND_RETHROW((_price)(_base)(_quote))
}

market_ticker database_api::get_ticker(const string& base, const string& quote)const
{
   return my->get_ticker(base, quote);
}

market_ticker database_api_impl::get_ticker(const string& base, const string& quote, bool skip_order_book)const
{
   FC_ASSERT(_app_options && _app_options->has_market_history_plugin, "Market history plugin is not enabled.");

   const auto assets = lookup_asset_symbols({ base, quote });

   FC_ASSERT(assets[0], "Invalid base asset symbol: ${s}", ("s", base));
   FC_ASSERT(assets[1], "Invalid quote asset symbol: ${s}", ("s", quote));

   auto base_id = assets[0]->asset_id;
   auto quote_id = assets[1]->asset_id;
   if (base_id > quote_id) std::swap(base_id, quote_id);
   const auto& ticker_idx = _db.get_index_type<graphene::market_history::market_ticker_index>().indices().get<by_market>();
   auto itr = ticker_idx.find(std::make_tuple(base_id, quote_id));
   const fc::time_point_sec now = _db.head_block_time();
   if (itr != ticker_idx.end())
   {
      order_book orders;
      if (!skip_order_book)
      {
         orders = get_order_book(assets[0]->symbol, assets[1]->symbol, 1);
      }
      return market_ticker(*itr, now, *assets[0], *assets[1], orders);
   }
   // if no ticker is found for this market we return an empty ticker
   market_ticker empty_result(now, *assets[0], *assets[1]);
   return empty_result;
}

market_volume database_api::get_24_volume(const string& base, const string& quote)const
{
   return my->get_24_volume(base, quote);
}

market_volume database_api_impl::get_24_volume(const string& base, const string& quote)const
{
   const auto& ticker = get_ticker(base, quote, true);

   market_volume result;
   result.time = ticker.time;
   result.base = ticker.base;
   result.quote = ticker.quote;
   result.base_volume = ticker.base_volume;
   result.quote_volume = ticker.quote_volume;

   return result;
}

order_book database_api::get_order_book(const string& base, const string& quote, unsigned limit)const
{
   return my->get_order_book(base, quote, limit);
}

order_book database_api_impl::get_order_book(const string& base, const string& quote, unsigned limit)const
{
   using boost::multiprecision::uint128_t;
   FC_ASSERT(limit <= 50);

   order_book result;
   result.base = base;
   result.quote = quote;

   auto assets = lookup_asset_symbols({ base, quote });
   FC_ASSERT(assets[0], "Invalid base asset symbol: ${s}", ("s", base));
   FC_ASSERT(assets[1], "Invalid quote asset symbol: ${s}", ("s", quote));

   auto base_id = assets[0]->asset_id;
   auto quote_id = assets[1]->asset_id;
   auto orders = get_limit_orders(base_id, quote_id, limit);

   for (const auto& o : orders)
   {
      if (o.sell_price.base.asset_id == base_id)
      {
         order ord;
         ord.price = price_to_string(o.sell_price, *assets[0], *assets[1]);
         uint128_t res = (uint128_t(o.for_sale.value) * o.sell_price.quote.amount.value) / o.sell_price.base.amount.value;
         ord.quote = assets[1]->amount_to_string(share_type(res.convert_to<int64_t>()));
         ord.base = assets[0]->amount_to_string(o.for_sale);
         result.bids.push_back(ord);
      }
      else
      {
         order ord;
         ord.price = price_to_string(o.sell_price, *assets[0], *assets[1]);
         ord.quote = assets[1]->amount_to_string(o.for_sale);
         uint128_t res = (uint128_t(o.for_sale.value) * o.sell_price.quote.amount.value) / o.sell_price.base.amount.value;
         ord.base = assets[0]->amount_to_string(share_type(res.convert_to<int64_t>()));
         result.asks.push_back(ord);
      }
   }

   return result;
}

vector<market_ticker> database_api::get_top_markets(uint32_t limit)const
{
   return my->get_top_markets(limit);
}

vector<market_ticker> database_api_impl::get_top_markets(uint32_t limit)const
{
   FC_ASSERT(_app_options && _app_options->has_market_history_plugin, "Market history plugin is not enabled.");

   FC_ASSERT(limit <= 100);

   const auto& volume_idx = _db.get_index_type<graphene::market_history::market_ticker_index>().indices().get<by_volume>();
   auto itr = volume_idx.rbegin();
   vector<market_ticker> result;
   result.reserve(limit);
   const fc::time_point_sec now = _db.head_block_time();

   while (itr != volume_idx.rend() && result.size() < limit)
   {
      const asset_object base = _db.get_asset_by_aid(itr->base);
      const asset_object quote = _db.get_asset_by_aid(itr->quote);
      order_book orders;
      orders = get_order_book(base.symbol, quote.symbol, 1);

      result.emplace_back(market_ticker(*itr, now, base, quote, orders));
      ++itr;
   }
   return result;
}

vector<market_trade> database_api::get_trade_history(const string& base,
   const string& quote,
   fc::time_point_sec start,
   fc::time_point_sec stop,
   unsigned limit)const
{
   return my->get_trade_history(base, quote, start, stop, limit);
}

vector<market_trade> database_api_impl::get_trade_history(const string& base,
   const string& quote,
   fc::time_point_sec start,
   fc::time_point_sec stop,
   unsigned limit)const
{
   FC_ASSERT(_app_options && _app_options->has_market_history_plugin, "Market history plugin is not enabled.");

   FC_ASSERT(limit <= 100);

   auto assets = lookup_asset_symbols({ base, quote });
   FC_ASSERT(assets[0], "Invalid base asset symbol: ${s}", ("s", base));
   FC_ASSERT(assets[1], "Invalid quote asset symbol: ${s}", ("s", quote));

   auto base_id = assets[0]->asset_id;
   auto quote_id = assets[1]->asset_id;

   if (base_id > quote_id) std::swap(base_id, quote_id);

   if (start.sec_since_epoch() == 0)
      start = fc::time_point_sec(fc::time_point::now());

   uint32_t count = 0;
   const auto& history_idx = _db.get_index_type<graphene::market_history::history_index>().indices().get<by_market_time>();
   auto itr = history_idx.lower_bound(std::make_tuple(base_id, quote_id, start));
   vector<market_trade> result;

   while (itr != history_idx.end() && count < limit && !(itr->key.base != base_id || itr->key.quote != quote_id || itr->time < stop))
   {
      {
         market_trade trade;

         if (assets[0]->asset_id == itr->op.receives.asset_id)
         {
            trade.amount = assets[1]->amount_to_string(itr->op.pays);
            trade.value = assets[0]->amount_to_string(itr->op.receives);
         }
         else
         {
            trade.amount = assets[1]->amount_to_string(itr->op.receives);
            trade.value = assets[0]->amount_to_string(itr->op.pays);
         }

         trade.date = itr->time;
         trade.price = price_to_string(itr->op.fill_price, *assets[0], *assets[1]);

         if (itr->op.is_maker)
         {
            trade.sequence = -itr->key.sequence;
            trade.side1_account_id = itr->op.account_id;
         }
         else
            trade.side2_account_id = itr->op.account_id;

         auto next_itr = std::next(itr);
         // Trades are usually tracked in each direction, exception: for global settlement only one side is recorded
         if (next_itr != history_idx.end() && next_itr->key.base == base_id && next_itr->key.quote == quote_id
            && next_itr->time == itr->time && next_itr->op.is_maker != itr->op.is_maker)
         {  // next_itr now could be the other direction // FIXME not 100% sure
            if (next_itr->op.is_maker)
            {
               trade.sequence = -next_itr->key.sequence;
               trade.side1_account_id = next_itr->op.account_id;
            }
            else
               trade.side2_account_id = next_itr->op.account_id;
            // skip the other direction
            itr = next_itr;
         }

         result.push_back(trade);
         ++count;
      }

      ++itr;
   }

   return result;
}

vector<market_trade> database_api::get_trade_history_by_sequence(
   const string& base,
   const string& quote,
   int64_t start,
   fc::time_point_sec stop,
   unsigned limit)const
{
   return my->get_trade_history_by_sequence(base, quote, start, stop, limit);
}

vector<market_trade> database_api_impl::get_trade_history_by_sequence(
   const string& base,
   const string& quote,
   int64_t start,
   fc::time_point_sec stop,
   unsigned limit)const
{
   FC_ASSERT(_app_options && _app_options->has_market_history_plugin, "Market history plugin is not enabled.");

   FC_ASSERT(limit <= 100);
   FC_ASSERT(start >= 0);
   int64_t start_seq = -start;

   auto assets = lookup_asset_symbols({ base, quote });
   FC_ASSERT(assets[0], "Invalid base asset symbol: ${s}", ("s", base));
   FC_ASSERT(assets[1], "Invalid quote asset symbol: ${s}", ("s", quote));

   auto base_id = assets[0]->asset_id;
   auto quote_id = assets[1]->asset_id;

   if (base_id > quote_id) std::swap(base_id, quote_id);
   const auto& history_idx = _db.get_index_type<graphene::market_history::history_index>().indices().get<by_key>();
   history_key hkey;
   hkey.base = base_id;
   hkey.quote = quote_id;
   hkey.sequence = start_seq;

   uint32_t count = 0;
   auto itr = history_idx.lower_bound(hkey);
   vector<market_trade> result;

   while (itr != history_idx.end() && count < limit && !(itr->key.base != base_id || itr->key.quote != quote_id || itr->time < stop))
   {
      if (itr->key.sequence == start_seq) // found the key, should skip this and the other direction if found
      {
         auto next_itr = std::next(itr);
         if (next_itr != history_idx.end() && next_itr->key.base == base_id && next_itr->key.quote == quote_id
            && next_itr->time == itr->time && next_itr->op.is_maker != itr->op.is_maker)
         {  // next_itr now could be the other direction // FIXME not 100% sure
            // skip the other direction
            itr = next_itr;
         }
      }
      else
      {
         market_trade trade;

         if (assets[0]->asset_id == itr->op.receives.asset_id)
         {
            trade.amount = assets[1]->amount_to_string(itr->op.pays);
            trade.value = assets[0]->amount_to_string(itr->op.receives);
         }
         else
         {
            trade.amount = assets[1]->amount_to_string(itr->op.receives);
            trade.value = assets[0]->amount_to_string(itr->op.pays);
         }

         trade.date = itr->time;
         trade.price = price_to_string(itr->op.fill_price, *assets[0], *assets[1]);

         if (itr->op.is_maker)
         {
            trade.sequence = -itr->key.sequence;
            trade.side1_account_id = itr->op.account_id;
         }
         else
            trade.side2_account_id = itr->op.account_id;

         auto next_itr = std::next(itr);
         // Trades are usually tracked in each direction, exception: for global settlement only one side is recorded
         if (next_itr != history_idx.end() && next_itr->key.base == base_id && next_itr->key.quote == quote_id
            && next_itr->time == itr->time && next_itr->op.is_maker != itr->op.is_maker)
         {  // next_itr now could be the other direction // FIXME not 100% sure
            if (next_itr->op.is_maker)
            {
               trade.sequence = -next_itr->key.sequence;
               trade.side1_account_id = next_itr->op.account_id;
            }
            else
               trade.side2_account_id = next_itr->op.account_id;
            // skip the other direction
            itr = next_itr;
         }

         result.push_back(trade);
         ++count;
      }

      ++itr;
   }

   return result;
}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Witnesses                                                        //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<optional<witness_object>> database_api::get_witnesses(const vector<account_uid_type>& witness_uids)const
{
   return my->get_witnesses( witness_uids );
}

vector<optional<witness_object>> database_api_impl::get_witnesses(const vector<account_uid_type>& witness_uids)const
{
   vector<optional<witness_object>> result; result.reserve(witness_uids.size());
   std::transform(witness_uids.begin(), witness_uids.end(), std::back_inserter(result),
                  [this](account_uid_type uid) -> optional<witness_object> {
      if( auto o = _db.find_witness_by_uid( uid ) )
         return *o;
      return {};
   });
   return result;
}

fc::optional<witness_object> database_api::get_witness_by_account(account_uid_type account)const
{
   return my->get_witness_by_account( account );
}

fc::optional<witness_object> database_api_impl::get_witness_by_account(account_uid_type account) const
{
   const auto& idx = _db.get_index_type<witness_index>().indices().get<by_valid>();
   auto itr = idx.find( std::make_tuple( true, account ) );
   if( itr != idx.end() )
      return *itr;
   return {};
}

vector<witness_object> database_api::lookup_witnesses(const account_uid_type lower_bound_uid, uint32_t limit,
                                                      data_sorting_type order_by)const
{
   return my->lookup_witnesses( lower_bound_uid, limit, order_by );
}

vector<witness_object> database_api_impl::lookup_witnesses(const account_uid_type lower_bound_uid, uint32_t limit,
                                                           data_sorting_type order_by)const
{
   FC_ASSERT( limit <= 101 );
   vector<witness_object> result;

   if( order_by == order_by_uid )
   {
      const auto& idx = _db.get_index_type<witness_index>().indices().get<by_valid>();
      auto itr = idx.lower_bound( std::make_tuple( true, lower_bound_uid ) );
      while( itr != idx.end() && limit > 0 ) // assume false < true
      {
         result.push_back( *itr );
         ++itr;
         --limit;
      }
   }
   else
   {
      account_uid_type new_lower_bound_uid = lower_bound_uid;
      const witness_object* lower_bound_obj = _db.find_witness_by_uid( lower_bound_uid );
      uint64_t lower_bound_shares = -1;
      if( lower_bound_obj == nullptr )
         new_lower_bound_uid = 0;
      else
      {
         if( order_by == order_by_votes )
            lower_bound_shares = lower_bound_obj->total_votes;
         else // by pledge
            lower_bound_shares = lower_bound_obj->pledge;
      }

      if( order_by == order_by_votes )
      {
         const auto& idx = _db.get_index_type<witness_index>().indices().get<by_votes>();
         auto itr = idx.lower_bound( std::make_tuple( true, lower_bound_shares, new_lower_bound_uid ) );
         while( itr != idx.end() && limit > 0 ) // assume false < true
         {
            result.push_back( *itr );
            ++itr;
            --limit;
         }
      }
      else // by pledge
      {
         const auto& idx = _db.get_index_type<witness_index>().indices().get<by_pledge>();
         auto itr = idx.lower_bound( std::make_tuple( true, lower_bound_shares, new_lower_bound_uid ) );
         while( itr != idx.end() && limit > 0 ) // assume false < true
         {
            result.push_back( *itr );
            ++itr;
            --limit;
         }
      }
   }

   return result;
}

uint64_t database_api::get_witness_count()const
{
   return my->get_witness_count();
}

uint64_t database_api_impl::get_witness_count()const
{
   return _db.get_index_type<witness_index>().indices().get<by_valid>().count( true );
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Committee members and proposals                                  //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<optional<committee_member_object>> database_api::get_committee_members(const vector<account_uid_type>& committee_member_uids)const
{
   return my->get_committee_members( committee_member_uids );
}

vector<optional<committee_member_object>> database_api_impl::get_committee_members(const vector<account_uid_type>& committee_member_uids)const
{
   vector<optional<committee_member_object>> result; result.reserve(committee_member_uids.size());
   std::transform(committee_member_uids.begin(), committee_member_uids.end(), std::back_inserter(result),
                  [this](account_uid_type uid) -> optional<committee_member_object> {
      if( auto o = _db.find_committee_member_by_uid( uid ) )
         return *o;
      return {};
   });
   return result;
}

fc::optional<committee_member_object> database_api::get_committee_member_by_account(account_uid_type account)const
{
   return my->get_committee_member_by_account( account );
}

fc::optional<committee_member_object> database_api_impl::get_committee_member_by_account(account_uid_type account) const
{
   const auto& idx = _db.get_index_type<committee_member_index>().indices().get<by_valid>();
   auto itr = idx.find( std::make_tuple( true, account ) );
   if( itr != idx.end() )
      return *itr;
   return {};
}

vector<committee_member_object> database_api::lookup_committee_members(const account_uid_type lower_bound_uid, uint32_t limit,
                                                                       data_sorting_type order_by)const
{
   return my->lookup_committee_members( lower_bound_uid, limit, order_by );
}

vector<committee_member_object> database_api_impl::lookup_committee_members(const account_uid_type lower_bound_uid, uint32_t limit,
                                                                            data_sorting_type order_by)const
{
   FC_ASSERT( limit <= 101 );
   vector<committee_member_object> result;

   if( order_by == order_by_uid )
   {
      const auto& idx = _db.get_index_type<committee_member_index>().indices().get<by_valid>();
      auto itr = idx.lower_bound( std::make_tuple( true, lower_bound_uid ) );
      while( itr != idx.end() && limit > 0 ) // assume false < true
      {
         result.push_back( *itr );
         ++itr;
         --limit;
      }
   }
   else
   {
      account_uid_type new_lower_bound_uid = lower_bound_uid;
      const committee_member_object* lower_bound_obj = _db.find_committee_member_by_uid( lower_bound_uid );
      uint64_t lower_bound_shares = -1;
      if( lower_bound_obj == nullptr )
         new_lower_bound_uid = 0;
      else
      {
         if( order_by == order_by_votes )
            lower_bound_shares = lower_bound_obj->total_votes;
         else // by pledge
            lower_bound_shares = lower_bound_obj->pledge;
      }

      if( order_by == order_by_votes )
      {
         const auto& idx = _db.get_index_type<committee_member_index>().indices().get<by_votes>();
         auto itr = idx.lower_bound( std::make_tuple( true, lower_bound_shares, new_lower_bound_uid ) );
         while( itr != idx.end() && limit > 0 ) // assume false < true
         {
            result.push_back( *itr );
            ++itr;
            --limit;
         }
      }
      else // by pledge
      {
         const auto& idx = _db.get_index_type<committee_member_index>().indices().get<by_pledge>();
         auto itr = idx.lower_bound( std::make_tuple( true, lower_bound_shares, new_lower_bound_uid ) );
         while( itr != idx.end() && limit > 0 ) // assume false < true
         {
            result.push_back( *itr );
            ++itr;
            --limit;
         }
      }
   }

   return result;
}

uint64_t database_api::get_committee_member_count()const
{
   return my->get_committee_member_count();
}

uint64_t database_api_impl::get_committee_member_count()const
{
   return _db.get_index_type<committee_member_index>().indices().get<by_valid>().count( true );
}

vector<committee_proposal_object> database_api::list_committee_proposals()const
{
   return my->list_committee_proposals();
}

vector<committee_proposal_object> database_api_impl::list_committee_proposals()const
{
   const auto& idx = _db.get_index_type<committee_proposal_index>().indices();
   vector<committee_proposal_object> result;
   result.reserve( idx.size() );

   for( const committee_proposal_object& o : idx )
   {
      result.emplace_back( o );
   }

   return result;
}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Authority / validation                                           //
//                                                                  //
//////////////////////////////////////////////////////////////////////

std::string database_api::get_transaction_hex(const signed_transaction& trx)const
{
   return my->get_transaction_hex( trx );
}

std::string database_api_impl::get_transaction_hex(const signed_transaction& trx)const
{
   return fc::to_hex(fc::raw::pack(trx));
}

std::pair<std::pair<flat_set<public_key_type>,flat_set<public_key_type>>,flat_set<signature_type>> database_api::get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const
{
   return my->get_required_signatures( trx, available_keys );
}

std::pair<std::pair<flat_set<public_key_type>,flat_set<public_key_type>>,flat_set<signature_type>> database_api_impl::get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const
{
   wdump((trx)(available_keys));
   bool enable_hardfork_04 = _db.get_dynamic_global_properties().enabled_hardfork_version >= ENABLE_HEAD_FORK_04;
   auto result = trx.get_required_signatures( _db.get_chain_id(),
                                       available_keys,
                                       [&]( account_uid_type uid ){ return &(_db.get_account_by_uid(uid).owner); },
                                       [&]( account_uid_type uid ){ return &(_db.get_account_by_uid(uid).active); },
                                       [&]( account_uid_type uid ){ return &(_db.get_account_by_uid(uid).secondary); },
                                       enable_hardfork_04,
                                       _db.get_global_properties().parameters.max_authority_depth );
   wdump((std::get<0>(result))(std::get<1>(result))(std::get<2>(result)));
   return std::make_pair( std::make_pair( std::get<0>(result), std::get<1>(result) ), std::get<2>(result) );
}

set<public_key_type> database_api::get_potential_signatures( const signed_transaction& trx )const
{
   return my->get_potential_signatures( trx );
}

set<public_key_type> database_api_impl::get_potential_signatures( const signed_transaction& trx )const
{
   wdump((trx));
   set<public_key_type> result;
   bool enable_hardfork_04 = _db.get_dynamic_global_properties().enabled_hardfork_version >= ENABLE_HEAD_FORK_04;
   trx.get_required_signatures(
      _db.get_chain_id(),
      flat_set<public_key_type>(),
      [&]( account_uid_type uid )
      {
         const auto& auth = _db.get_account_by_uid( uid ).owner;
         for( const auto& k : auth.get_keys() )
            result.insert(k);
         return &auth;
      },
      [&]( account_uid_type uid )
      {
         const auto& auth = _db.get_account_by_uid( uid ).active;
         for( const auto& k : auth.get_keys() )
            result.insert(k);
         return &auth;
      },
      [&]( account_uid_type uid )
      {
         const auto& auth = _db.get_account_by_uid( uid ).secondary;
         for( const auto& k : auth.get_keys() )
            result.insert(k);
         return &auth;
      },
      enable_hardfork_04,
      _db.get_global_properties().parameters.max_authority_depth
   );

   wdump((result));
   return result;
}

bool database_api::verify_authority( const signed_transaction& trx )const
{
   return my->verify_authority( trx );
}

bool database_api_impl::verify_authority( const signed_transaction& trx )const
{
   bool enable_hardfork_04 = _db.get_dynamic_global_properties().enabled_hardfork_version >= ENABLE_HEAD_FORK_04;
   trx.verify_authority( _db.get_chain_id(),
                         [this]( account_uid_type uid ){ return &(_db.get_account_by_uid( uid ).owner); },
                         [this]( account_uid_type uid ){ return &(_db.get_account_by_uid( uid ).active); },
                         [this]( account_uid_type uid ){ return &(_db.get_account_by_uid( uid ).secondary); },
                         enable_hardfork_04,
                         _db.get_global_properties().parameters.max_authority_depth );
   return true;
}

bool database_api::verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& signers )const
{
   return my->verify_account_authority( name_or_id, signers );
}

bool database_api_impl::verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& keys )const
{
   FC_ASSERT( name_or_id.size() > 0);
   const account_object* account = nullptr;
   if (std::isdigit(name_or_id[0]))
      account = _db.find(fc::variant(name_or_id ).as<account_id_type>( 1 ));
   else
   {
      const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
      auto itr = idx.find(name_or_id);
      if (itr != idx.end())
         account = &*itr;
   }
   FC_ASSERT( account, "no such account" );


   /// reuse trx.verify_authority by creating a dummy transfer
   signed_transaction trx;
   transfer_operation op;
   op.from = account->uid;
   trx.operations.emplace_back(op);

   return verify_authority( trx );
}

processed_transaction database_api::validate_transaction( const signed_transaction& trx )const
{
   return my->validate_transaction( trx );
}

processed_transaction database_api_impl::validate_transaction( const signed_transaction& trx )const
{
   return _db.validate_transaction(trx);
}

vector< fc::variant > database_api::get_required_fees( const vector<operation>& ops, asset_id_type id )const
{
   return my->get_required_fees( ops, id );
}

vector< required_fee_data > database_api::get_required_fee_data( const vector<operation>& ops )const
{
   return my->get_required_fee_data( ops );
}

/**
 * Container method for mutually recursive functions used to
 * implement get_required_fees() with potentially nested proposals.
 */
struct get_required_fees_helper
{
   get_required_fees_helper(
      const fee_schedule& _current_fee_schedule,
      const price& _core_exchange_rate,
      uint32_t _max_recursion
      )
      : current_fee_schedule(_current_fee_schedule),
        core_exchange_rate(_core_exchange_rate),
        max_recursion(_max_recursion)
   {}

   fc::variant set_op_fees( operation& op )
   {
      if( op.which() == operation::tag<proposal_create_operation>::value )
      {
         return set_proposal_create_op_fees( op );
      }
      else
      {
         asset fee = current_fee_schedule.set_fee( op, core_exchange_rate );
         fc::variant result;
         fc::to_variant( fee, result, GRAPHENE_MAX_NESTED_OBJECTS );
         return result;
      }
   }

   fc::variant set_proposal_create_op_fees( operation& proposal_create_op )
   {
      proposal_create_operation& op = proposal_create_op.get<proposal_create_operation>();
      std::pair< asset, fc::variants > result;
      for( op_wrapper& prop_op : op.proposed_ops )
      {
         FC_ASSERT( current_recursion < max_recursion );
         ++current_recursion;
         result.second.push_back( set_op_fees( prop_op.op ) );
         --current_recursion;
      }
      // we need to do this on the boxed version, which is why we use
      // two mutually recursive functions instead of a visitor
      result.first = current_fee_schedule.set_fee( proposal_create_op, core_exchange_rate );
      fc::variant vresult;
      fc::to_variant( result, vresult, GRAPHENE_MAX_NESTED_OBJECTS );
      return vresult;
   }

   const fee_schedule& current_fee_schedule;
   const price& core_exchange_rate;
   uint32_t max_recursion;
   uint32_t current_recursion = 0;
};

vector< fc::variant > database_api_impl::get_required_fees( const vector<operation>& ops, asset_id_type id )const
{
   vector< operation > _ops = ops;
   //
   // we copy the ops because we need to mutate an operation to reliably
   // determine its fee, see #435
   //

   vector< fc::variant > result;
   result.reserve(ops.size());
   //const asset_object& a = id(_db);
   const price cer( asset( 1, GRAPHENE_CORE_ASSET_AID ), asset( 1, GRAPHENE_CORE_ASSET_AID ) );
   get_required_fees_helper helper(
      _db.current_fee_schedule(),
      cer,
      GET_REQUIRED_FEES_MAX_RECURSION );
   for( operation& op : _ops )
   {
      result.push_back( helper.set_op_fees( op ) );
   }
   return result;
}


struct fee_payer_uid_visitor
{
   typedef account_uid_type result_type;

   template<typename OpType>
   result_type operator()( const OpType& op )const
   {
      return op.fee_payer_uid();
   }
};

vector< required_fee_data > database_api_impl::get_required_fee_data( const vector<operation>& ops )const
{
   vector< required_fee_data > result;
   result.reserve(ops.size());

   const auto& fs = _db.current_fee_schedule();
   for( const operation& op : ops )
   {
      const auto& fee_pair = fs.calculate_fee_pair( op );
      const auto fee_payer_uid = op.visit( fee_payer_uid_visitor() );
      result.push_back( { fee_payer_uid, fee_pair.first.value, fee_pair.second.value } );
   }
   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Proposed transactions                                            //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<proposal_object> database_api::get_proposed_transactions( account_uid_type uid )const
{
   return my->get_proposed_transactions( uid );
}

/** TODO: add secondary index that will accelerate this process */
vector<proposal_object> database_api_impl::get_proposed_transactions( account_uid_type uid )const
{
   const auto& idx = _db.get_index_type<proposal_index>();
   vector<proposal_object> result;

   idx.inspect_all_objects( [&](const object& obj){
           const proposal_object& p = static_cast<const proposal_object&>(obj);
           if( p.required_secondary_approvals.find( uid ) != p.required_secondary_approvals.end() )
              result.push_back(p);
           else if( p.required_active_approvals.find( uid ) != p.required_active_approvals.end() )
              result.push_back(p);
           else if ( p.required_owner_approvals.find( uid ) != p.required_owner_approvals.end() )
              result.push_back(p);
           else if ( p.available_active_approvals.find( uid ) != p.available_active_approvals.end() )
              result.push_back(p);
           else if( p.available_secondary_approvals.find( uid ) != p.available_secondary_approvals.end() )
              result.push_back(p);
   });
   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Private methods                                                  //
//                                                                  //
//////////////////////////////////////////////////////////////////////

void database_api_impl::broadcast_updates( const vector<variant>& updates )
{
   if( updates.size() && _subscribe_callback ) {
      auto capture_this = shared_from_this();
      fc::async([capture_this,updates](){
          if(capture_this->_subscribe_callback)
            capture_this->_subscribe_callback( fc::variant(updates) );
      });
   }
}


void database_api_impl::on_objects_removed( const vector<object_id_type>& ids, const vector<const object*>& objs, const flat_set<account_uid_type>& impacted_accounts)
{
   handle_object_changed(_notify_remove_create, false, ids, impacted_accounts,
      [objs](object_id_type id) -> const object* {
         auto it = std::find_if(
               objs.begin(), objs.end(),
               [id](const object* o) {return o != nullptr && o->id == id;});

         if (it != objs.end())
            return *it;

         return nullptr;
      }
   );
}

void database_api_impl::on_objects_new(const vector<object_id_type>& ids, const flat_set<account_uid_type>& impacted_accounts)
{
   handle_object_changed(_notify_remove_create, true, ids, impacted_accounts,
      std::bind(&object_database::find_object, &_db, std::placeholders::_1)
   );
}

void database_api_impl::on_objects_changed(const vector<object_id_type>& ids, const flat_set<account_uid_type>& impacted_accounts)
{
   handle_object_changed(false, true, ids, impacted_accounts,
      std::bind(&object_database::find_object, &_db, std::placeholders::_1)
   );
}

void database_api_impl::handle_object_changed(bool force_notify, bool full_object, const vector<object_id_type>& ids, const flat_set<account_uid_type>& impacted_accounts, std::function<const object*(object_id_type id)> find_object)
{
   if( _subscribe_callback )
   {
      vector<variant> updates;

      for(auto id : ids)
      {
         if( force_notify || is_subscribed_to_item(id) || is_impacted_account(impacted_accounts) )
         {
            if( full_object )
            {
               auto obj = find_object(id);
               if( obj )
               {
                  updates.emplace_back( obj->to_variant() );
               }
            }
            else
            {
               updates.emplace_back( fc::variant( id, 1 ) );
            }
         }
      }

      broadcast_updates(updates);
   }

}

/** note: this method cannot yield because it is called in the middle of
 * apply a block.
 */
void database_api_impl::on_applied_block()
{
   if (_block_applied_callback)
   {
      auto capture_this = shared_from_this();
      block_id_type block_id = _db.head_block_id();
      fc::async([this,capture_this,block_id](){
         _block_applied_callback(fc::variant( block_id, 1 ));
      });
   }

   /// we need to ensure the database_api is not deleted for the life of the async operation
   if (_market_subscriptions.size() == 0)
       return;

   const auto& ops = _db.get_applied_operations();
   map< std::pair<asset_aid_type, asset_aid_type>, vector<pair<operation, operation_result>> > subscribed_markets_ops;
   for (const optional< operation_history_object >& o_op : ops)
   {
       if (!o_op.valid())
           continue;
       const operation_history_object& op = *o_op;

       optional< std::pair<asset_aid_type, asset_aid_type> > market;
       switch (op.op.which())
       {
           /*  This is sent via the object_changed callback
           case operation::tag<limit_order_create_operation>::value:
           market = op.op.get<limit_order_create_operation>().get_market();
           break;
           */
       case operation::tag<fill_order_operation>::value:
           market = op.op.get<fill_order_operation>().get_market();
           break;
           /*
           case operation::tag<limit_order_cancel_operation>::value:
           */
       default: break;
       }
       if (market.valid() && _market_subscriptions.count(*market))
           // FIXME this may cause fill_order_operation be pushed before order creation
           subscribed_markets_ops[*market].emplace_back(std::make_pair(op.op, op.result));
   }
   /// we need to ensure the database_api is not deleted for the life of the async operation
   auto capture_this = shared_from_this();
   fc::async([this, capture_this, subscribed_markets_ops](){
       for (auto item : subscribed_markets_ops)
       {
           auto itr = _market_subscriptions.find(item.first);
           if (itr != _market_subscriptions.end())
               itr->second(fc::variant(item.second, GRAPHENE_NET_MAX_NESTED_OBJECTS));
       }
   });
}

} } // graphene::app
