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
#include <graphene/chain/get_config.hpp>

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

class database_api_impl;


class database_api_impl : public std::enable_shared_from_this<database_api_impl>
{
   public:
      database_api_impl( graphene::chain::database& db );
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
      optional<signed_block> get_block(uint32_t block_num)const;
      processed_transaction get_transaction( uint32_t block_num, uint32_t trx_in_block )const;

      // Globals
      chain_property_object get_chain_properties()const;
      global_property_object get_global_properties()const;
      fc::variant_object get_config()const;
      chain_id_type get_chain_id()const;
      dynamic_global_property_object get_dynamic_global_properties()const;

      // Keys
      vector<vector<account_id_type>> get_key_references( vector<public_key_type> key )const;
     bool is_public_key_registered(string public_key) const;

      // Accounts
      vector<optional<account_object>> get_accounts(const vector<account_id_type>& account_ids)const;
      vector<optional<account_object>> get_accounts_by_uid(const vector<account_uid_type>& account_uids)const;
      std::map<string,full_account> get_full_accounts( const vector<string>& names_or_ids, bool subscribe );
      std::map<account_uid_type,full_account> get_full_accounts_by_uid( const vector<account_uid_type>& uids,
                                                                        const full_account_query_options& options );
      optional<account_object> get_account_by_name( string name )const;
      vector<account_id_type> get_account_references( account_id_type account_id )const;
      vector<optional<account_object>> lookup_account_names(const vector<string>& account_names)const;
      map<string,account_uid_type> lookup_accounts_by_name(const string& lower_bound_name, uint32_t limit)const;
      uint64_t get_account_count()const;

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
                                      const optional<account_uid_type> poster,
                                      const std::pair<time_point_sec, time_point_sec> create_time_range,
                                      const uint32_t limit )const;

      // Balances
      vector<asset> get_account_balances(account_uid_type uid, const flat_set<asset_aid_type>& assets)const;
      vector<asset> get_named_account_balances(const std::string& name, const flat_set<asset_aid_type>& assets)const;
      vector<balance_object> get_balance_objects( const vector<address>& addrs )const;
      vector<asset> get_vested_balances( const vector<balance_id_type>& objs )const;
      vector<vesting_balance_object> get_vesting_balances( account_id_type account_id )const;

      // Assets
      vector<optional<asset_object>> get_assets(const vector<asset_aid_type>& asset_ids)const;
      vector<asset_object>           list_assets(const string& lower_bound_symbol, uint32_t limit)const;
      vector<optional<asset_object>> lookup_asset_symbols(const vector<string>& symbols_or_ids)const;

      // Markets / feeds
      vector<limit_order_object>         get_limit_orders(asset_id_type a, asset_id_type b, uint32_t limit)const;
      vector<call_order_object>          get_call_orders(asset_id_type a, uint32_t limit)const;
      vector<force_settlement_object>    get_settle_orders(asset_id_type a, uint32_t limit)const;
      vector<call_order_object>          get_margin_positions( const account_id_type& id )const;
      void subscribe_to_market(std::function<void(const variant&)> callback, asset_aid_type a, asset_aid_type b);
      void unsubscribe_from_market(asset_aid_type a, asset_aid_type b);
      market_ticker                      get_ticker( const string& base, const string& quote )const;
      market_volume                      get_24_volume( const string& base, const string& quote )const;
      order_book                         get_order_book( const string& base, const string& quote, unsigned limit = 50 )const;
      vector<market_trade>               get_trade_history( const string& base, const string& quote, fc::time_point_sec start, fc::time_point_sec stop, unsigned limit = 100 )const;

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

      // Votes
      vector<variant> lookup_vote_ids( const vector<vote_id_type>& votes )const;

      // Authority / validation
      std::string get_transaction_hex(const signed_transaction& trx)const;
      set<public_key_type> get_required_signatures_old( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const;
      std::pair<std::pair<flat_set<public_key_type>,flat_set<public_key_type>>,flat_set<signature_type>> get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const;
      set<public_key_type> get_potential_signatures( const signed_transaction& trx )const;
      set<address> get_potential_address_signatures( const signed_transaction& trx )const;
      bool verify_authority( const signed_transaction& trx )const;
      bool verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& signers )const;
      processed_transaction validate_transaction( const signed_transaction& trx )const;
      vector< fc::variant > get_required_fees( const vector<operation>& ops, asset_id_type id )const;
      vector< std::pair< int64_t, int64_t > > get_required_fee_pairs( const vector<operation>& ops )const;
      vector< required_fee_data > get_required_fee_data( const vector<operation>& ops )const;

      // Proposed transactions
      vector<proposal_object> get_proposed_transactions( account_id_type id )const;

      // Blinded balances
      vector<blinded_balance_object> get_blinded_balances( const flat_set<commitment_type>& commitments )const;

   //private:
      template<typename T>
      void subscribe_to_item( const T& i )const
      {
         auto vec = fc::raw::pack(i);
         if( !_subscribe_callback )
            return;

         if( !is_subscribed_to_item(i) )
         {
            idump((i));
            _subscribe_filter.insert( vec.data(), vec.size() );//(vecconst char*)&i, sizeof(i) );
         }
      }

      template<typename T>
      bool is_subscribed_to_item( const T& i )const
      {
         if( !_subscribe_callback )
            return false;

         return _subscribe_filter.contains( i );
      }

      bool is_impacted_account( const flat_set<account_id_type>& accounts)
      {
         if( !_subscribed_accounts.size() || !accounts.size() )
            return false;

         return std::any_of(accounts.begin(), accounts.end(), [this](const account_id_type& account) {
            return _subscribed_accounts.find(account) != _subscribed_accounts.end();
         });
      }

      template<typename T>
      void enqueue_if_subscribed_to_market(const object* obj, market_queue_type& queue, bool full_object=true)
      {
         const T* order = dynamic_cast<const T*>(obj);
         FC_ASSERT( order != nullptr);

         auto market = order->get_market();
         //std::pair<asset_id_type, asset_id_type> market;
         //market.first = asset_id_type(m.first);
         //market.second = asset_id_type(m.second);

         auto sub = _market_subscriptions.find( market );
         if( sub != _market_subscriptions.end() ) {
            queue[market].emplace_back( full_object ? obj->to_variant() : fc::variant(obj->id) );
         }
      }

      void broadcast_updates( const vector<variant>& updates );
      void broadcast_market_updates( const market_queue_type& queue);
      void handle_object_changed(bool force_notify, bool full_object, const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts, std::function<const object*(object_id_type id)> find_object);

      /** called every time a block is applied to report the objects that were changed */
      void on_objects_new(const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts);
      void on_objects_changed(const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts);
      void on_objects_removed(const vector<object_id_type>& ids, const vector<const object*>& objs, const flat_set<account_id_type>& impacted_accounts);
      void on_applied_block();

      bool _notify_remove_create = false;
      mutable fc::bloom_filter _subscribe_filter;
      std::set<account_id_type> _subscribed_accounts;
      std::function<void(const fc::variant&)> _subscribe_callback;
      std::function<void(const fc::variant&)> _pending_trx_callback;
      std::function<void(const fc::variant&)> _block_applied_callback;

      boost::signals2::scoped_connection                                                                                           _new_connection;
      boost::signals2::scoped_connection                                                                                           _change_connection;
      boost::signals2::scoped_connection                                                                                           _removed_connection;
      boost::signals2::scoped_connection                                                                                           _applied_block_connection;
      boost::signals2::scoped_connection                                                                                           _pending_trx_connection;
      map< pair<asset_aid_type,asset_aid_type>, std::function<void(const variant&)> >      _market_subscriptions;
      graphene::chain::database&                                                                                                            _db;
};

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Constructors                                                     //
//                                                                  //
//////////////////////////////////////////////////////////////////////

database_api::database_api( graphene::chain::database& db )
   : my( new database_api_impl( db ) ) {}

database_api::~database_api() {}

database_api_impl::database_api_impl( graphene::chain::database& db ):_db(db)
{
   wlog("creating database api ${x}", ("x",int64_t(this)) );
   _new_connection = _db.new_objects.connect([this](const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts) {
                                on_objects_new(ids, impacted_accounts);
                                });
   _change_connection = _db.changed_objects.connect([this](const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts) {
                                on_objects_changed(ids, impacted_accounts);
                                });
   _removed_connection = _db.removed_objects.connect([this](const vector<object_id_type>& ids, const vector<const object*>& objs, const flat_set<account_id_type>& impacted_accounts) {
                                on_objects_removed(ids, objs, impacted_accounts);
                                });
   _applied_block_connection = _db.applied_block.connect([this](const signed_block&){ on_applied_block(); });

   _pending_trx_connection = _db.on_pending_transaction.connect([this](const signed_transaction& trx ){
                         if( _pending_trx_callback ) _pending_trx_callback( fc::variant(trx) );
                      });
}

database_api_impl::~database_api_impl()
{
   elog("freeing database api ${x}", ("x",int64_t(this)) );
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
   _market_subscriptions.clear();
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

optional<signed_block> database_api::get_block(uint32_t block_num)const
{
   return my->get_block( block_num );
}

optional<signed_block> database_api_impl::get_block(uint32_t block_num)const
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

vector<vector<account_id_type>> database_api::get_key_references( vector<public_key_type> key )const
{
   return my->get_key_references( key );
}

/**
 *  @return all accounts that referr to the key or account id in their owner or active authorities.
 */
vector<vector<account_id_type>> database_api_impl::get_key_references( vector<public_key_type> keys )const
{
   wdump( (keys) );
   vector< vector<account_id_type> > final_result;
   final_result.reserve(keys.size());

   for( auto& key : keys )
   {

      address a1( pts_address(key, false, 56) );
      address a2( pts_address(key, true, 56) );
      address a3( pts_address(key, false, 0)  );
      address a4( pts_address(key, true, 0)  );
      address a5( key );

      subscribe_to_item( key );
      subscribe_to_item( a1 );
      subscribe_to_item( a2 );
      subscribe_to_item( a3 );
      subscribe_to_item( a4 );
      subscribe_to_item( a5 );

      const auto& idx = _db.get_index_type<account_index>();
      const auto& aidx = dynamic_cast<const primary_index<account_index>&>(idx);
      const auto& refs = aidx.get_secondary_index<graphene::chain::account_member_index>();
      auto itr = refs.account_to_key_memberships.find(key);
      vector<account_id_type> result;

      for( auto& a : {a1,a2,a3,a4,a5} )
      {
          auto itr = refs.account_to_address_memberships.find(a);
          if( itr != refs.account_to_address_memberships.end() )
          {
             result.reserve( itr->second.size() );
             for( auto item : itr->second )
             {
                wdump((a)(item)(item(_db).name));
                result.push_back(item);
             }
          }
      }

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
   idump((names_or_ids));
   std::map<std::string, full_account> results;

   for (const std::string& account_name_or_id : names_or_ids)
   {
      const account_object* account = nullptr;
      if (std::isdigit(account_name_or_id[0]))
         account = _db.find(fc::variant(account_name_or_id).as<account_id_type>());
      else
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
         _subscribed_accounts.insert( account->get_id() );
         subscribe_to_item( account->id );
      }

      // fc::mutable_variant_object full_account;
      full_account acnt;
      acnt.account = *account;
      acnt.statistics = account->statistics(_db);
      acnt.registrar_name = account->registrar(_db).name;
      acnt.referrer_name = account->referrer(_db).name;
      acnt.lifetime_referrer_name = account->lifetime_referrer(_db).name;
      //TODO review
      //acnt.votes = lookup_vote_ids( vector<vote_id_type>(account->options.votes.begin(),account->options.votes.end()) );

      // Add the account itself, its statistics object, cashback balance, and referral account names
      /*
      full_account("account", *account)("statistics", account->statistics(_db))
            ("registrar_name", account->registrar(_db).name)("referrer_name", account->referrer(_db).name)
            ("lifetime_referrer_name", account->lifetime_referrer(_db).name);
            */
      if (account->cashback_vb)
      {
         acnt.cashback_balance = account->cashback_balance(_db);
      }
      // Add the account's proposals
      const auto& proposal_idx = _db.get_index_type<proposal_index>();
      const auto& pidx = dynamic_cast<const primary_index<proposal_index>&>(proposal_idx);
      const auto& proposals_by_account = pidx.get_secondary_index<graphene::chain::required_approval_index>();
      auto  required_approvals_itr = proposals_by_account._account_to_proposals.find( account->id );
      if( required_approvals_itr != proposals_by_account._account_to_proposals.end() )
      {
         acnt.proposals.reserve( required_approvals_itr->second.size() );
         for( auto proposal_id : required_approvals_itr->second )
            acnt.proposals.push_back( proposal_id(_db) );
      }


      // Add the account's balances
      auto balance_range = _db.get_index_type<account_balance_index>().indices().get<by_account_asset>().equal_range(boost::make_tuple(account->uid));
      //vector<account_balance_object> balances;
      std::for_each(balance_range.first, balance_range.second,
                    [&acnt](const account_balance_object& balance) {
                       acnt.balances.emplace_back(balance);
                    });

      // Add the account's vesting balances
      auto vesting_range = _db.get_index_type<vesting_balance_index>().indices().get<by_account>().equal_range(account->id);
      std::for_each(vesting_range.first, vesting_range.second,
                    [&acnt](const vesting_balance_object& balance) {
                       acnt.vesting_balances.emplace_back(balance);
                    });

      // Add the account's orders
      auto order_range = _db.get_index_type<limit_order_index>().indices().get<by_account>().equal_range(account->id);
      std::for_each(order_range.first, order_range.second,
                    [&acnt] (const limit_order_object& order) {
                       acnt.limit_orders.emplace_back(order);
                    });
      auto call_range = _db.get_index_type<call_order_index>().indices().get<by_account>().equal_range(account->id);
      std::for_each(call_range.first, call_range.second,
                    [&acnt] (const call_order_object& call) {
                       acnt.call_orders.emplace_back(call);
                    });
      auto settle_range = _db.get_index_type<force_settlement_index>().indices().get<by_account>().equal_range(account->id);
      std::for_each(settle_range.first, settle_range.second,
                    [&acnt] (const force_settlement_object& settle) {
                       acnt.settle_orders.emplace_back(settle);
                    });

      // get assets issued by user
      auto asset_range = _db.get_index_type<asset_index>().indices().get<by_issuer>().equal_range(account->id);
      std::for_each(asset_range.first, asset_range.second,
                    [&acnt] (const asset_object& asset) {
                       acnt.assets.emplace_back(asset.id);
                    });

      // get withdraws permissions
      auto withdraw_range = _db.get_index_type<withdraw_permission_index>().indices().get<by_from>().equal_range(account->id);
      std::for_each(withdraw_range.first, withdraw_range.second,
                    [&acnt] (const withdraw_permission_object& withdraw) {
                       acnt.withdraws.emplace_back(withdraw);
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

      // fc::mutable_variant_object full_account;
      auto& account_stats = _db.get_account_statistics_by_uid( uid );
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
                       acnt.witness_votes.emplace_back( o.witness_uid );
                    });
      }
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
                       acnt.committee_member_votes.emplace_back( o.committee_member_uid );
                    });
      }

      results[uid] = acnt;
   }
   return results;
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

vector<account_id_type> database_api::get_account_references( account_id_type account_id )const
{
   return my->get_account_references( account_id );
}

vector<account_id_type> database_api_impl::get_account_references( account_id_type account_id )const
{
   const auto& idx = _db.get_index_type<account_index>();
   const auto& aidx = dynamic_cast<const primary_index<account_index>&>(idx);
   const auto& refs = aidx.get_secondary_index<graphene::chain::account_member_index>();
   auto itr = refs.account_to_account_memberships.find(account_id);
   vector<account_id_type> result;

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
   FC_ASSERT( limit <= 100 );
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
   return _db.get_index_type< platform_index >().indices().get< by_valid >().count( true );
}

optional<post_object> database_api::get_post( const account_uid_type platform_owner,
                                              const account_uid_type poster_uid,
                                              const post_pid_type post_pid )const
{
   return my->get_post( platform_owner, poster_uid, post_pid );
}

optional<post_object> database_api_impl::get_post( const account_uid_type platform_owner,
                                                   const account_uid_type poster_uid,
                                                   const post_pid_type post_pid )const
{
   if( auto o = _db.find_post_by_platform( platform_owner, poster_uid, post_pid ) )
   {
      return *o;
   }
   return {};
}

vector<post_object> database_api::get_posts_by_platform_poster( const account_uid_type platform_owner,
                                      const optional<account_uid_type> poster,
                                      const std::pair<time_point_sec, time_point_sec> create_time_range,
                                      const uint32_t limit )const
{
   return my->get_posts_by_platform_poster( platform_owner, poster, create_time_range, limit );
}

vector<post_object> database_api_impl::get_posts_by_platform_poster( const account_uid_type platform_owner,
                                      const optional<account_uid_type> poster,
                                      const std::pair<time_point_sec, time_point_sec> create_time_range,
                                      const uint32_t limit )const
{
   FC_ASSERT( limit <= 100 );

   vector<post_object> result;

   const time_point_sec max_time = std::max( create_time_range.first, create_time_range.second );
   const time_point_sec min_time = std::min( create_time_range.first, create_time_range.second );

   uint32_t count = 0;

   if( poster.valid() )
   {
      const auto& post_idx = _db.get_index_type<post_index>().indices().get<by_platform_poster_create_time>();

      // index is latest first, query range is ( earliest, latest ]
      auto itr = post_idx.lower_bound( std::make_tuple( platform_owner, *poster, max_time ) );
      auto itr_end = post_idx.lower_bound( std::make_tuple( platform_owner, *poster, min_time ) );

      while( itr != itr_end && count < limit )
      {
         result.push_back(*itr);
         ++itr;
         ++count;
      }
   }
   else
   {
      const auto& post_idx = _db.get_index_type<post_index>().indices().get<by_platform_create_time>();

      // index is latest first, query range is ( earliest, latest ]
      auto itr = post_idx.lower_bound( std::make_tuple( platform_owner, max_time ) );
      auto itr_end = post_idx.lower_bound( std::make_tuple( platform_owner, min_time ) );

      while( itr != itr_end && count < limit )
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
   return my->get_account_balances( uid, assets );
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
   return my->get_named_account_balances( name, assets );
}

vector<asset> database_api_impl::get_named_account_balances(const std::string& name, const flat_set<asset_aid_type>& assets) const
{
   const auto& accounts_by_name = _db.get_index_type<account_index>().indices().get<by_name>();
   auto itr = accounts_by_name.find(name);
   FC_ASSERT( itr != accounts_by_name.end() );
   return get_account_balances(itr->get_uid(), assets);
}

vector<balance_object> database_api::get_balance_objects( const vector<address>& addrs )const
{
   return my->get_balance_objects( addrs );
}

vector<balance_object> database_api_impl::get_balance_objects( const vector<address>& addrs )const
{
   try
   {
      const auto& bal_idx = _db.get_index_type<balance_index>();
      const auto& by_owner_idx = bal_idx.indices().get<by_owner>();

      vector<balance_object> result;

      for( const auto& owner : addrs )
      {
         subscribe_to_item( owner );
         auto itr = by_owner_idx.lower_bound( boost::make_tuple( owner, asset_id_type(0) ) );
         while( itr != by_owner_idx.end() && itr->owner == owner )
         {
            result.push_back( *itr );
            ++itr;
         }
      }
      return result;
   }
   FC_CAPTURE_AND_RETHROW( (addrs) )
}

vector<asset> database_api::get_vested_balances( const vector<balance_id_type>& objs )const
{
   return my->get_vested_balances( objs );
}

vector<asset> database_api_impl::get_vested_balances( const vector<balance_id_type>& objs )const
{
   try
   {
      vector<asset> result;
      result.reserve( objs.size() );
      auto now = _db.head_block_time();
      for( auto obj : objs )
         result.push_back( obj(_db).available( now ) );
      return result;
   } FC_CAPTURE_AND_RETHROW( (objs) )
}

vector<vesting_balance_object> database_api::get_vesting_balances( account_id_type account_id )const
{
   return my->get_vesting_balances( account_id );
}

vector<vesting_balance_object> database_api_impl::get_vesting_balances( account_id_type account_id )const
{
   try
   {
      vector<vesting_balance_object> result;
      auto vesting_range = _db.get_index_type<vesting_balance_index>().indices().get<by_account>().equal_range(account_id);
      std::for_each(vesting_range.first, vesting_range.second,
                    [&result](const vesting_balance_object& balance) {
                       result.emplace_back(balance);
                    });
      return result;
   }
   FC_CAPTURE_AND_RETHROW( (account_id) );
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Assets                                                           //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<optional<asset_object>> database_api::get_assets(const vector<asset_aid_type>& asset_ids)const
{
   return my->get_assets( asset_ids );
}

vector<optional<asset_object>> database_api_impl::get_assets(const vector<asset_aid_type>& asset_ids)const
{
   vector<optional<asset_object>> result; result.reserve(asset_ids.size());
   std::transform(asset_ids.begin(), asset_ids.end(), std::back_inserter(result),
                  [this](asset_aid_type id) -> optional<asset_object> {

      const auto& idx = _db.get_index_type<asset_index>().indices().get<by_aid>();
      auto itr = idx.find( id );
      if( itr != idx.end() )
      {
         subscribe_to_item( (*itr).id );
         return *itr;
      }
         
      return {};
   });
   return result;
}

vector<asset_object> database_api::list_assets(const string& lower_bound_symbol, uint32_t limit)const
{
   return my->list_assets( lower_bound_symbol, limit );
}

vector<asset_object> database_api_impl::list_assets(const string& lower_bound_symbol, uint32_t limit)const
{
   FC_ASSERT( limit <= 100 );
   const auto& assets_by_symbol = _db.get_index_type<asset_index>().indices().get<by_symbol>();
   vector<asset_object> result;
   result.reserve(limit);

   auto itr = assets_by_symbol.lower_bound(lower_bound_symbol);

   if( lower_bound_symbol == "" )
      itr = assets_by_symbol.begin();

   while(limit-- && itr != assets_by_symbol.end())
      result.emplace_back(*itr++);

   return result;
}

vector<optional<asset_object>> database_api::lookup_asset_symbols(const vector<string>& symbols_or_ids)const
{
   return my->lookup_asset_symbols( symbols_or_ids );
}

vector<optional<asset_object>> database_api_impl::lookup_asset_symbols(const vector<string>& symbols_or_ids)const
{
   const auto& assets_by_symbol = _db.get_index_type<asset_index>().indices().get<by_symbol>();
   vector<optional<asset_object> > result;
   result.reserve(symbols_or_ids.size());
   std::transform(symbols_or_ids.begin(), symbols_or_ids.end(), std::back_inserter(result),
                  [this, &assets_by_symbol](const string& symbol_or_id) -> optional<asset_object> {
      if( !symbol_or_id.empty() && std::isdigit(symbol_or_id[0]) )
      {
         auto ptr = _db.find(variant(symbol_or_id).as<asset_id_type>());
         return ptr == nullptr? optional<asset_object>() : *ptr;
      }
      auto itr = assets_by_symbol.find(symbol_or_id);
      return itr == assets_by_symbol.end()? optional<asset_object>() : *itr;
   });
   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Markets / feeds                                                  //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<limit_order_object> database_api::get_limit_orders(asset_id_type a, asset_id_type b, uint32_t limit)const
{
   return my->get_limit_orders( a, b, limit );
}

/**
 *  @return the limit orders for both sides of the book for the two assets specified up to limit number on each side.
 */
vector<limit_order_object> database_api_impl::get_limit_orders(asset_id_type a, asset_id_type b, uint32_t limit)const
{
   const auto& limit_order_idx = _db.get_index_type<limit_order_index>();
   const auto& limit_price_idx = limit_order_idx.indices().get<by_price>();

   vector<limit_order_object> result;

   uint32_t count = 0;
   auto limit_itr = limit_price_idx.lower_bound(price::max(a,b));
   auto limit_end = limit_price_idx.upper_bound(price::min(a,b));
   while(limit_itr != limit_end && count < limit)
   {
      result.push_back(*limit_itr);
      ++limit_itr;
      ++count;
   }
   count = 0;
   limit_itr = limit_price_idx.lower_bound(price::max(b,a));
   limit_end = limit_price_idx.upper_bound(price::min(b,a));
   while(limit_itr != limit_end && count < limit)
   {
      result.push_back(*limit_itr);
      ++limit_itr;
      ++count;
   }

   return result;
}

vector<call_order_object> database_api::get_call_orders(asset_id_type a, uint32_t limit)const
{
   return my->get_call_orders( a, limit );
}

vector<call_order_object> database_api_impl::get_call_orders(asset_id_type a, uint32_t limit)const
{
   const auto& call_index = _db.get_index_type<call_order_index>().indices().get<by_price>();
   const asset_object& mia = _db.get(a);
   price index_price = price::min(mia.bitasset_data(_db).options.short_backing_asset, mia.get_id());

   return vector<call_order_object>(call_index.lower_bound(index_price.min()),
                                    call_index.lower_bound(index_price.max()));
}

vector<force_settlement_object> database_api::get_settle_orders(asset_id_type a, uint32_t limit)const
{
   return my->get_settle_orders( a, limit );
}

vector<force_settlement_object> database_api_impl::get_settle_orders(asset_id_type a, uint32_t limit)const
{
   const auto& settle_index = _db.get_index_type<force_settlement_index>().indices().get<by_expiration>();
   const asset_object& mia = _db.get(a);
   return vector<force_settlement_object>(settle_index.lower_bound(mia.get_id()),
                                          settle_index.upper_bound(mia.get_id()));
}

vector<call_order_object> database_api::get_margin_positions( const account_id_type& id )const
{
   return my->get_margin_positions( id );
}

vector<call_order_object> database_api_impl::get_margin_positions( const account_id_type& id )const
{
   try
   {
      const auto& idx = _db.get_index_type<call_order_index>();
      const auto& aidx = idx.indices().get<by_account>();
      auto start = aidx.lower_bound( boost::make_tuple( id, asset_id_type(0) ) );
      auto end = aidx.lower_bound( boost::make_tuple( id+1, asset_id_type(0) ) );
      vector<call_order_object> result;
      while( start != end )
      {
         result.push_back(*start);
         ++start;
      }
      return result;
   } FC_CAPTURE_AND_RETHROW( (id) )
}

void database_api::subscribe_to_market(std::function<void(const variant&)> callback, asset_aid_type a, asset_aid_type b)
{
   my->subscribe_to_market( callback, a, b );
}

void database_api_impl::subscribe_to_market(std::function<void(const variant&)> callback, asset_aid_type a, asset_aid_type b)
{
   if(a > b) std::swap(a,b);
   FC_ASSERT(a != b);
   _market_subscriptions[ std::make_pair(a,b) ] = callback;
}

void database_api::unsubscribe_from_market(asset_aid_type a, asset_aid_type b)
{
   my->unsubscribe_from_market( a, b );
}

void database_api_impl::unsubscribe_from_market(asset_aid_type a, asset_aid_type b)
{
   if(a > b) std::swap(a,b);
   FC_ASSERT(a != b);
   _market_subscriptions.erase(std::make_pair(a,b));
}

market_ticker database_api::get_ticker( const string& base, const string& quote )const
{
    return my->get_ticker( base, quote );
}

market_ticker database_api_impl::get_ticker( const string& base, const string& quote )const
{
    const auto assets = lookup_asset_symbols( {base, quote} );
    FC_ASSERT( assets[0], "Invalid base asset symbol: ${s}", ("s",base) );
    FC_ASSERT( assets[1], "Invalid quote asset symbol: ${s}", ("s",quote) );

    market_ticker result;
    result.base = base;
    result.quote = quote;
    result.latest = 0;
    result.lowest_ask = 0;
    result.highest_bid = 0;
    result.percent_change = 0;
    result.base_volume = 0;
    result.quote_volume = 0;

    try {
        const fc::time_point_sec now = fc::time_point::now();
        const fc::time_point_sec yesterday = fc::time_point_sec( now.sec_since_epoch() - 86400 );
        const auto batch_size = 100;

        vector<market_trade> trades = get_trade_history( base, quote, now, yesterday, batch_size );
        if( !trades.empty() )
        {
            result.latest = trades[0].price;

            while( !trades.empty() )
            {
                for( const market_trade& t: trades )
                {
                    result.base_volume += t.value;
                    result.quote_volume += t.amount;
                }

                trades = get_trade_history( base, quote, trades.back().date, yesterday, batch_size );
            }

            const auto last_trade_yesterday = get_trade_history( base, quote, yesterday, fc::time_point_sec(), 1 );
            if( !last_trade_yesterday.empty() )
            {
                const auto price_yesterday = last_trade_yesterday[0].price;
                result.percent_change = ( (result.latest / price_yesterday) - 1 ) * 100;
            }
        }
        else
        {
            const auto last_trade = get_trade_history( base, quote, now, fc::time_point_sec(), 1 );
            if( !last_trade.empty() )
                result.latest = last_trade[0].price;
        }

        const auto orders = get_order_book( base, quote, 1 );
        if( !orders.asks.empty() ) result.lowest_ask = orders.asks[0].price;
        if( !orders.bids.empty() ) result.highest_bid = orders.bids[0].price;
    } FC_CAPTURE_AND_RETHROW( (base)(quote) )

    return result;
}

market_volume database_api::get_24_volume( const string& base, const string& quote )const
{
    return my->get_24_volume( base, quote );
}

market_volume database_api_impl::get_24_volume( const string& base, const string& quote )const
{
    const auto ticker = get_ticker( base, quote );

    market_volume result;
    result.base = ticker.base;
    result.quote = ticker.quote;
    result.base_volume = ticker.base_volume;
    result.quote_volume = ticker.quote_volume;

    return result;
}

order_book database_api::get_order_book( const string& base, const string& quote, unsigned limit )const
{
   return my->get_order_book( base, quote, limit);
}

order_book database_api_impl::get_order_book( const string& base, const string& quote, unsigned limit )const
{
   using boost::multiprecision::uint128_t;
   FC_ASSERT( limit <= 50 );

   order_book result;
   result.base = base;
   result.quote = quote;

   auto assets = lookup_asset_symbols( {base, quote} );
   FC_ASSERT( assets[0], "Invalid base asset symbol: ${s}", ("s",base) );
   FC_ASSERT( assets[1], "Invalid quote asset symbol: ${s}", ("s",quote) );

   auto base_id = assets[0]->id;
   auto quote_id = assets[1]->id;
   auto orders = get_limit_orders( base_id, quote_id, limit );


   auto asset_to_real = [&]( const asset& a, int p ) { return double(a.amount.value)/pow( 10, p ); };
   auto price_to_real = [&]( const price& p )
   {
      if( asset_id_type(p.base.asset_id) == base_id )
         return asset_to_real( p.base, assets[0]->precision ) / asset_to_real( p.quote, assets[1]->precision );
      else
         return asset_to_real( p.quote, assets[0]->precision ) / asset_to_real( p.base, assets[1]->precision );
   };

   for( const auto& o : orders )
   {
      if( asset_id_type(o.sell_price.base.asset_id) == base_id )
      {
         order ord;
         ord.price = price_to_real( o.sell_price );
         ord.quote = asset_to_real( share_type( ( uint128_t( o.for_sale.value ) * o.sell_price.quote.amount.value ) / o.sell_price.base.amount.value ), assets[1]->precision );
         ord.base = asset_to_real( o.for_sale, assets[0]->precision );
         result.bids.push_back( ord );
      }
      else
      {
         order ord;
         ord.price = price_to_real( o.sell_price );
         ord.quote = asset_to_real( o.for_sale, assets[1]->precision );
         ord.base = asset_to_real( share_type( ( uint64_t( o.for_sale.value ) * o.sell_price.quote.amount.value ) / o.sell_price.base.amount.value ), assets[0]->precision );
         result.asks.push_back( ord );
      }
   }

   return result;
}

vector<market_trade> database_api::get_trade_history( const string& base,
                                                      const string& quote,
                                                      fc::time_point_sec start,
                                                      fc::time_point_sec stop,
                                                      unsigned limit )const
{
   return my->get_trade_history( base, quote, start, stop, limit );
}

vector<market_trade> database_api_impl::get_trade_history( const string& base,
                                                           const string& quote,
                                                           fc::time_point_sec start,
                                                           fc::time_point_sec stop,
                                                           unsigned limit )const
{
   FC_ASSERT( limit <= 100 );

   auto assets = lookup_asset_symbols( {base, quote} );
   FC_ASSERT( assets[0], "Invalid base asset symbol: ${s}", ("s",base) );
   FC_ASSERT( assets[1], "Invalid quote asset symbol: ${s}", ("s",quote) );

   auto base_id = assets[0]->id;
   auto quote_id = assets[1]->id;

   if( base_id > quote_id ) std::swap( base_id, quote_id );
   const auto& history_idx = _db.get_index_type<graphene::market_history::history_index>().indices().get<by_key>();
   history_key hkey;
   hkey.base = base_id;
   hkey.quote = quote_id;
   hkey.sequence = std::numeric_limits<int64_t>::min();

   auto price_to_real = [&]( const share_type a, int p ) { return double( a.value ) / pow( 10, p ); };

   if ( start.sec_since_epoch() == 0 )
      start = fc::time_point_sec( fc::time_point::now() );

   uint32_t count = 0;
   auto itr = history_idx.lower_bound( hkey );
   vector<market_trade> result;

   while( itr != history_idx.end() && count < limit && !( itr->key.base != base_id || itr->key.quote != quote_id || itr->time < stop ) )
   {
      if( itr->time < start )
      {
         market_trade trade;

         if( assets[0]->id == asset_id_type(itr->op.receives.asset_id) )
         {
            trade.amount = price_to_real( itr->op.pays.amount, assets[1]->precision );
            trade.value = price_to_real( itr->op.receives.amount, assets[0]->precision );
         }
         else
         {
            trade.amount = price_to_real( itr->op.receives.amount, assets[1]->precision );
            trade.value = price_to_real( itr->op.pays.amount, assets[0]->precision );
         }

         trade.date = itr->time;
         trade.price = trade.value / trade.amount;

         result.push_back( trade );
         ++count;
      }

      // Trades are tracked in each direction.
      ++itr;
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

// Workers
vector<worker_object> database_api::get_workers_by_account(account_id_type account)const
{
    const auto& idx = my->_db.get_index_type<worker_index>().indices().get<by_account>();
    auto itr = idx.find(account);
    vector<worker_object> result;

    if( itr != idx.end() && itr->worker_account == account )
    {
       result.emplace_back( *itr );
       ++itr;
    }

    return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Votes                                                            //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<variant> database_api::lookup_vote_ids( const vector<vote_id_type>& votes )const
{
   return my->lookup_vote_ids( votes );
}

vector<variant> database_api_impl::lookup_vote_ids( const vector<vote_id_type>& votes )const
{
   FC_ASSERT( votes.size() < 1000, "Only 1000 votes can be queried at a time" );

   //const auto& witness_idx = _db.get_index_type<witness_index>().indices().get<by_vote_id>();
   //const auto& committee_idx = _db.get_index_type<committee_member_index>().indices().get<by_vote_id>();
   const auto& for_worker_idx = _db.get_index_type<worker_index>().indices().get<by_vote_for>();
   const auto& against_worker_idx = _db.get_index_type<worker_index>().indices().get<by_vote_against>();

   vector<variant> result;
   result.reserve( votes.size() );
   for( auto id : votes )
   {
      switch( id.type() )
      {
         case vote_id_type::committee:
         {
            // TODO review
            //auto itr = committee_idx.find( id );
            //if( itr != committee_idx.end() )
            //   result.emplace_back( variant( *itr ) );
            //else
            //   result.emplace_back( variant() );
            break;
         }
         case vote_id_type::witness:
         {
            // TODO review
            //auto itr = witness_idx.find( id );
            //if( itr != witness_idx.end() )
            //   result.emplace_back( variant( *itr ) );
            //else
            //   result.emplace_back( variant() );
            break;
         }
         case vote_id_type::worker:
         {
            auto itr = for_worker_idx.find( id );
            if( itr != for_worker_idx.end() ) {
               result.emplace_back( variant( *itr ) );
            }
            else {
               auto itr = against_worker_idx.find( id );
               if( itr != against_worker_idx.end() ) {
                  result.emplace_back( variant( *itr ) );
               }
               else {
                  result.emplace_back( variant() );
               }
            }
            break;
         }
         case vote_id_type::VOTE_TYPE_COUNT: break; // supress unused enum value warnings
      }
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
   auto result = trx.get_required_signatures( _db.get_chain_id(),
                                       available_keys,
                                       [&]( account_uid_type uid ){ return &(_db.get_account_by_uid(uid).owner); },
                                       [&]( account_uid_type uid ){ return &(_db.get_account_by_uid(uid).active); },
                                       [&]( account_uid_type uid ){ return &(_db.get_account_by_uid(uid).secondary); },
                                       _db.get_global_properties().parameters.max_authority_depth );
   wdump((std::get<0>(result))(std::get<1>(result))(std::get<2>(result)));
   return std::make_pair( std::make_pair( std::get<0>(result), std::get<1>(result) ), std::get<2>(result) );
}

set<public_key_type> database_api::get_required_signatures_old( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const
{
   return my->get_required_signatures_old( trx, available_keys );
}

set<public_key_type> database_api_impl::get_required_signatures_old( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const
{
   wdump((trx)(available_keys));
   auto result = trx.get_required_signatures( _db.get_chain_id(),
                                       available_keys,
                                       [&]( account_id_type id ){ return &id(_db).active; },
                                       [&]( account_id_type id ){ return &id(_db).owner; },
                                       _db.get_global_properties().parameters.max_authority_depth );
   wdump((result));
   return result;
}

set<public_key_type> database_api::get_potential_signatures( const signed_transaction& trx )const
{
   return my->get_potential_signatures( trx );
}
set<address> database_api::get_potential_address_signatures( const signed_transaction& trx )const
{
   return my->get_potential_address_signatures( trx );
}

set<public_key_type> database_api_impl::get_potential_signatures( const signed_transaction& trx )const
{
   wdump((trx));
   set<public_key_type> result;
   trx.get_required_signatures(
      _db.get_chain_id(),
      flat_set<public_key_type>(),
      [&]( account_id_type id )
      {
         const auto& auth = id(_db).active;
         for( const auto& k : auth.get_keys() )
            result.insert(k);
         return &auth;
      },
      [&]( account_id_type id )
      {
         const auto& auth = id(_db).owner;
         for( const auto& k : auth.get_keys() )
            result.insert(k);
         return &auth;
      },
      _db.get_global_properties().parameters.max_authority_depth
   );

   wdump((result));
   return result;
}

set<address> database_api_impl::get_potential_address_signatures( const signed_transaction& trx )const
{
   set<address> result;
   trx.get_required_signatures(
      _db.get_chain_id(),
      flat_set<public_key_type>(),
      [&]( account_id_type id )
      {
         const auto& auth = id(_db).active;
         for( const auto& k : auth.get_addresses() )
            result.insert(k);
         return &auth;
      },
      [&]( account_id_type id )
      {
         const auto& auth = id(_db).owner;
         for( const auto& k : auth.get_addresses() )
            result.insert(k);
         return &auth;
      },
      _db.get_global_properties().parameters.max_authority_depth
   );
   return result;
}

bool database_api::verify_authority( const signed_transaction& trx )const
{
   return my->verify_authority( trx );
}

bool database_api_impl::verify_authority( const signed_transaction& trx )const
{
   trx.verify_authority( _db.get_chain_id(),
                         [&]( account_id_type id ){ return &id(_db).active; },
                         [&]( account_id_type id ){ return &id(_db).owner; },
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
      account = _db.find(fc::variant(name_or_id).as<account_id_type>());
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

vector< std::pair< int64_t, int64_t > > database_api::get_required_fee_pairs( const vector<operation>& ops )const
{
   return my->get_required_fee_pairs( ops );
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
         fc::to_variant( fee, result );
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
      fc::to_variant( result, vresult );
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
   const asset_object& a = id(_db);
   get_required_fees_helper helper(
      _db.current_fee_schedule(),
      a.options.core_exchange_rate,
      GET_REQUIRED_FEES_MAX_RECURSION );
   for( operation& op : _ops )
   {
      result.push_back( helper.set_op_fees( op ) );
   }
   return result;
}

vector< std::pair< int64_t, int64_t > > database_api_impl::get_required_fee_pairs( const vector<operation>& ops )const
{
   vector< std::pair< int64_t, int64_t > > result;
   result.reserve(ops.size());

   const auto& fs = _db.current_fee_schedule();
   for( const operation& op : ops )
   {
      const auto& fee_pair = fs.calculate_fee_pair( op );
      result.push_back( std::make_pair( fee_pair.first.value, fee_pair.second.value ) );
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

vector<proposal_object> database_api::get_proposed_transactions( account_id_type id )const
{
   return my->get_proposed_transactions( id );
}

/** TODO: add secondary index that will accelerate this process */
vector<proposal_object> database_api_impl::get_proposed_transactions( account_id_type id )const
{
   const auto& idx = _db.get_index_type<proposal_index>();
   vector<proposal_object> result;

   idx.inspect_all_objects( [&](const object& obj){
           const proposal_object& p = static_cast<const proposal_object&>(obj);
           if( p.required_active_approvals.find( id ) != p.required_active_approvals.end() )
              result.push_back(p);
           else if ( p.required_owner_approvals.find( id ) != p.required_owner_approvals.end() )
              result.push_back(p);
           else if ( p.available_active_approvals.find( id ) != p.available_active_approvals.end() )
              result.push_back(p);
   });
   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Blinded balances                                                 //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<blinded_balance_object> database_api::get_blinded_balances( const flat_set<commitment_type>& commitments )const
{
   return my->get_blinded_balances( commitments );
}

vector<blinded_balance_object> database_api_impl::get_blinded_balances( const flat_set<commitment_type>& commitments )const
{
   vector<blinded_balance_object> result; result.reserve(commitments.size());
   const auto& bal_idx = _db.get_index_type<blinded_balance_index>();
   const auto& by_commitment_idx = bal_idx.indices().get<by_commitment>();
   for( const auto& c : commitments )
   {
      auto itr = by_commitment_idx.find( c );
      if( itr != by_commitment_idx.end() )
         result.push_back( *itr );
   }
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

void database_api_impl::broadcast_market_updates( const market_queue_type& queue)
{
   if( queue.size() )
   {
      auto capture_this = shared_from_this();
      fc::async([capture_this, this, queue](){
          for( const auto& item : queue )
          {
            auto sub = _market_subscriptions.find(item.first);
            if( sub != _market_subscriptions.end() )
                sub->second( fc::variant(item.second ) );
          }
      });
   }
}

void database_api_impl::on_objects_removed( const vector<object_id_type>& ids, const vector<const object*>& objs, const flat_set<account_id_type>& impacted_accounts)
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

void database_api_impl::on_objects_new(const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts)
{
   handle_object_changed(_notify_remove_create, true, ids, impacted_accounts,
      std::bind(&object_database::find_object, &_db, std::placeholders::_1)
   );
}

void database_api_impl::on_objects_changed(const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts)
{
   handle_object_changed(false, true, ids, impacted_accounts,
      std::bind(&object_database::find_object, &_db, std::placeholders::_1)
   );
}

void database_api_impl::handle_object_changed(bool force_notify, bool full_object, const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts, std::function<const object*(object_id_type id)> find_object)
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
               updates.emplace_back( id );
            }
         }
      }

      broadcast_updates(updates);
   }

   if( _market_subscriptions.size() )
   {
      market_queue_type broadcast_queue;

      for(auto id : ids)
      {
         if( id.is<call_order_object>() )
         {
            enqueue_if_subscribed_to_market<call_order_object>( find_object(id), broadcast_queue, full_object );
         }
         else if( id.is<limit_order_object>() )
         {
            enqueue_if_subscribed_to_market<limit_order_object>( find_object(id), broadcast_queue, full_object );
         }
      }

      broadcast_market_updates(broadcast_queue);
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
         _block_applied_callback(fc::variant(block_id));
      });
   }

   if(_market_subscriptions.size() == 0)
      return;

   const auto& ops = _db.get_applied_operations();
   map< std::pair<asset_aid_type,asset_aid_type>, vector<pair<operation, operation_result>> > subscribed_markets_ops;
   for(const optional< operation_history_object >& o_op : ops)
   {
      if( !o_op.valid() )
         continue;
      const operation_history_object& op = *o_op;

      std::pair<asset_aid_type,asset_aid_type> market;
      switch(op.op.which())
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
      if(_market_subscriptions.count(market))
         subscribed_markets_ops[market].push_back(std::make_pair(op.op, op.result));
   }
   /// we need to ensure the database_api is not deleted for the life of the async operation
   auto capture_this = shared_from_this();
   fc::async([this,capture_this,subscribed_markets_ops](){
      for(auto item : subscribed_markets_ops)
      {
         auto itr = _market_subscriptions.find(item.first);
         if(itr != _market_subscriptions.end())
            itr->second(fc::variant(item.second));
      }
   });
}

} } // graphene::app
