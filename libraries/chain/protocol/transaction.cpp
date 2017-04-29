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
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <fc/io/raw.hpp>
#include <fc/bitutil.hpp>
#include <fc/smart_ref_impl.hpp>
#include <algorithm>

namespace graphene { namespace chain {

digest_type processed_transaction::merkle_digest()const
{
   digest_type::encoder enc;
   fc::raw::pack( enc, *this );
   return enc.result();
}

digest_type transaction::digest()const
{
   digest_type::encoder enc;
   fc::raw::pack( enc, *this );
   return enc.result();
}

digest_type transaction::sig_digest( const chain_id_type& chain_id )const
{
   digest_type::encoder enc;
   fc::raw::pack( enc, chain_id );
   fc::raw::pack( enc, *this );
   return enc.result();
}

void transaction::validate() const
{
   FC_ASSERT( operations.size() > 0, "A transaction must have at least one operation", ("trx",*this) );
   for( const auto& op : operations )
      operation_validate(op);
}

graphene::chain::transaction_id_type graphene::chain::transaction::id() const
{
   auto h = digest();
   transaction_id_type result;
   memcpy(result._hash, h._hash, std::min(sizeof(result), sizeof(h)));
   return result;
}

const signature_type& graphene::chain::signed_transaction::sign(const private_key_type& key, const chain_id_type& chain_id)
{
   digest_type h = sig_digest( chain_id );
   signatures.push_back(key.sign_compact(h));
   return signatures.back();
}

signature_type graphene::chain::signed_transaction::sign(const private_key_type& key, const chain_id_type& chain_id)const
{
   digest_type::encoder enc;
   fc::raw::pack( enc, chain_id );
   fc::raw::pack( enc, *this );
   return key.sign_compact(enc.result());
}

void transaction::set_expiration( fc::time_point_sec expiration_time )
{
    expiration = expiration_time;
}

void transaction::set_reference_block( const block_id_type& reference_block )
{
   ref_block_num = fc::endian_reverse_u32(reference_block._hash[0]);
   ref_block_prefix = reference_block._hash[1];
}

void transaction::get_required_uid_authorities( flat_set<account_uid_type>& owner_uids,
                                                flat_set<account_uid_type>& active_uids,
                                                flat_set<account_uid_type>& secondary_uids,
                                                vector<authority>& other )const
{
   for( const auto& op : operations )
      operation_get_required_uid_authorities( op, owner_uids, active_uids, secondary_uids, other );
}

void transaction::get_required_authorities( flat_set<account_id_type>& active, flat_set<account_id_type>& owner, vector<authority>& other )const
{
   for( const auto& op : operations )
      operation_get_required_authorities( op, active, owner, other );
}


const std::function<const authority*(account_id_type)> null_by_id = []( account_id_type id ){ return nullptr; };
const std::function<const authority*(account_uid_type)> null_by_uid = []( account_uid_type uid ){ return nullptr; };
const flat_set<public_key_type> empty_public_key_set = flat_set<public_key_type>();

struct sign_state
{
      /** returns true if we have a signature for this key or can
       * produce a signature for this key, else returns false.
       */
      bool signed_by( const public_key_type& k )
      {
         auto itr = provided_signatures.find(k);
         if( itr == provided_signatures.end() )
         {
            auto pk = available_keys.find(k);
            if( pk != available_keys.end() )
            {
               used_keys.insert(k);
               return provided_signatures[k] = true;
            }
            return false;
         }
         return itr->second = true;
      }

      optional<map<address,public_key_type>> available_address_sigs;
      optional<map<address,public_key_type>> provided_address_sigs;

      bool signed_by( const address& a ) {
         if( !available_address_sigs ) {
            available_address_sigs = std::map<address,public_key_type>();
            provided_address_sigs = std::map<address,public_key_type>();
            for( auto& item : available_keys ) {
             (*available_address_sigs)[ address(pts_address(item, false, 56) ) ] = item;
             (*available_address_sigs)[ address(pts_address(item, true, 56) ) ] = item;
             (*available_address_sigs)[ address(pts_address(item, false, 0) ) ] = item;
             (*available_address_sigs)[ address(pts_address(item, true, 0) ) ] = item;
             (*available_address_sigs)[ address(item) ] = item;
            }
            for( auto& item : provided_signatures ) {
             (*provided_address_sigs)[ address(pts_address(item.first, false, 56) ) ] = item.first;
             (*provided_address_sigs)[ address(pts_address(item.first, true, 56) ) ] = item.first;
             (*provided_address_sigs)[ address(pts_address(item.first, false, 0) ) ] = item.first;
             (*provided_address_sigs)[ address(pts_address(item.first, true, 0) ) ] = item.first;
             (*provided_address_sigs)[ address(item.first) ] = item.first;
            }
         }
         auto itr = provided_address_sigs->find(a);
         if( itr == provided_address_sigs->end() )
         {
            auto aitr = available_address_sigs->find(a);
            if( aitr != available_address_sigs->end() ) {
               auto pk = available_keys.find(aitr->second);
               if( pk != available_keys.end() )
                  return provided_signatures[aitr->second] = true;
               return false;
            }
         }
         return provided_signatures[itr->second] = true;
      }

      std::tuple<bool,bool,flat_set<public_key_type>> check_authority( account_id_type id )
      {
         if( approved_by.find(id) != approved_by.end() )
            return std::make_tuple( true, true, empty_public_key_set );
         return check_authority( get_active(id) );
      }

      std::tuple<bool,bool,flat_set<public_key_type>> check_authority( const authority::account_uid_auth_type& uid_auth )
      {
         if( approved_by_uid_auth.find( uid_auth ) != approved_by_uid_auth.end() )
            return std::make_tuple( true, true, empty_public_key_set );
         if( uid_auth.auth_type == authority::secondary_auth )
            return check_authority( get_secondary_by_uid( uid_auth.uid ) );
         else if( uid_auth.auth_type == authority::active_auth )
            return check_authority( get_active_by_uid( uid_auth.uid ) );
         else // if( uid_auth.auth_type == authority::owner_auth )
            return check_authority( get_owner_by_uid( uid_auth.uid ) );
      }

      /**
       *  Checks to see if we have signatures of the active authorites of
       *  the accounts specified in authority or the keys specified.
       *  If the result is false,
       *     returns if it's possible to satisfy the authority as the second item in the tuple,
       *     and the missed keys as the third item in the tuple
       */
      std::tuple<bool,bool,flat_set<public_key_type>> check_authority( const authority* au, uint32_t depth = 0 )
      {
         if( au == nullptr )
         {
            flat_set<public_key_type> s;
            s.insert( public_key_type() );
            return std::make_tuple( false, false, s );
         }

         const authority& auth = *au;

         uint32_t total_weight = 0;
         uint32_t total_possible_weight = 0;
         flat_set<public_key_type> missed_keys;
         for( const auto& k : auth.key_auths )
         {
            const public_key_type& kf = k.first;
            if( kf != public_key_type() )
               total_possible_weight += k.second;
            if( signed_by( kf ) )
            {
               total_weight += k.second;
               if( total_weight >= auth.weight_threshold )
                  return std::make_tuple( true, true, empty_public_key_set );
            }
            else
            {
               if( kf != public_key_type() )
                  missed_keys.insert( kf );
            }
         }

         for( const auto& k : auth.address_auths )
         {
            total_possible_weight += k.second;
            if( signed_by( k.first ) )
            {
               total_weight += k.second;
               if( total_weight >= auth.weight_threshold )
                  return std::make_tuple( true, true, empty_public_key_set );
            }
         }

         for( const auto& a : auth.account_auths )
         {
            if( approved_by.find(a.first) == approved_by.end() )
            {
               if( depth == max_recursion )
                  continue;
               const auto& result = check_authority( get_active( a.first ), depth+1 );
               if( std::get<0>( result ) )
               {
                  total_possible_weight += a.second;
                  approved_by.insert( a.first );
                  total_weight += a.second;
                  if( total_weight >= auth.weight_threshold )
                     return std::make_tuple( true, true, empty_public_key_set );
               }
               else
               {
                  if( std::get<1>( result ) )
                  {
                     total_possible_weight += a.second;
                     for( const auto& k : std::get<2>( result ) )
                        missed_keys.insert( k );
                  }
               }
            }
            else
            {
               total_possible_weight += a.second;
               total_weight += a.second;
               if( total_weight >= auth.weight_threshold )
                  return std::make_tuple( true, true, empty_public_key_set );
            }
         }

         for( const auto& a : auth.account_uid_auths )
         {
            if( approved_by_uid_auth.find( a.first ) == approved_by_uid_auth.end() )
            {
               if( depth == max_recursion )
                  continue;
               std::tuple<bool, bool, flat_set<public_key_type>> result;
               if( a.first.auth_type == authority::secondary_auth )
                  result = check_authority( get_secondary_by_uid( a.first.uid ), depth+1 );
               else if( a.first.auth_type == authority::active_auth )
                  result = check_authority( get_active_by_uid( a.first.uid ), depth+1 );
               else // if( a.first.auth_type == authority::owner_auth )
                  result = check_authority( get_owner_by_uid( a.first.uid ), depth+1 );
               if( std::get<0>( result ) )
               {
                  total_possible_weight += a.second;
                  approved_by_uid_auth.insert( a.first );
                  total_weight += a.second;
                  if( total_weight >= auth.weight_threshold )
                     return std::make_tuple( true, true, empty_public_key_set );
               }
               else
               {
                  if( std::get<1>( result ) )
                  {
                     total_possible_weight += a.second;
                     for( const auto& k : std::get<2>( result ) )
                        missed_keys.insert( k );
                  }
               }
            }
            else
            {
               total_possible_weight += a.second;
               total_weight += a.second;
               if( total_weight >= auth.weight_threshold )
                  return std::make_tuple( true, true, empty_public_key_set );
            }
         }

         if( total_possible_weight >= auth.weight_threshold )
            return std::make_tuple( total_weight >= auth.weight_threshold, true, missed_keys );
         else
         {
            flat_set<public_key_type> s;
            s.insert( public_key_type() );
            return std::make_tuple( false, false, s );
         }
      }

      const flat_set<public_key_type>& get_used_keys()
      {
         return used_keys;
      }

      vector<public_key_type> get_unused_signature_keys()
      {
         vector<public_key_type> remove_sigs;
         for( const auto& sig : provided_signatures )
            if( !sig.second ) remove_sigs.push_back( sig.first );

         return remove_sigs;
      }

      bool remove_unused_signatures()
      {
         vector<public_key_type> remove_sigs;
         for( const auto& sig : provided_signatures )
            if( !sig.second ) remove_sigs.push_back( sig.first );

         for( auto& sig : remove_sigs )
            provided_signatures.erase(sig);

         return remove_sigs.size() != 0;
      }

      sign_state( const flat_map<public_key_type,signature_type>& sigs,
                  const std::function<const authority*(account_id_type)>& a,
                  const flat_set<public_key_type>& keys = empty_public_key_set )
      : get_active(a),
        get_owner_by_uid(null_by_uid),get_active_by_uid(null_by_uid),get_secondary_by_uid(null_by_uid),
        available_keys(keys)
      {
         for( const auto& key_sig : sigs )
            provided_signatures[ key_sig.first ] = false;
         approved_by.insert( GRAPHENE_TEMP_ACCOUNT  );
      }

      sign_state( const flat_map<public_key_type,signature_type>& sigs,
                  const std::function<const authority*(account_uid_type)>& o,
                  const std::function<const authority*(account_uid_type)>& a,
                  const std::function<const authority*(account_uid_type)>& s,
                  const flat_set<public_key_type>& keys = empty_public_key_set )
      : get_active(null_by_id),
        get_owner_by_uid(o),get_active_by_uid(a),get_secondary_by_uid(s),
        available_keys(keys)
      {
         for( const auto& key_sig : sigs )
            provided_signatures[ key_sig.first ] = false;
         approved_by_uid_auth.emplace( GRAPHENE_TEMP_ACCOUNT_UID, authority::owner_auth );
         approved_by_uid_auth.emplace( GRAPHENE_TEMP_ACCOUNT_UID, authority::active_auth );
         approved_by_uid_auth.emplace( GRAPHENE_TEMP_ACCOUNT_UID, authority::secondary_auth );
      }

      const std::function<const authority*(account_id_type)>& get_active;

      const std::function<const authority*(account_uid_type)>& get_owner_by_uid;
      const std::function<const authority*(account_uid_type)>& get_active_by_uid;
      const std::function<const authority*(account_uid_type)>& get_secondary_by_uid;

      const flat_set<public_key_type>&                        available_keys;
      flat_set<public_key_type>                               used_keys;

      flat_map<public_key_type,bool>             provided_signatures; // the bool means "is_used"
      flat_set<account_id_type>                  approved_by;
      flat_set<authority::account_uid_auth_type> approved_by_uid_auth;
      uint32_t                                   max_recursion = GRAPHENE_MAX_SIG_CHECK_DEPTH;
};


void verify_authority( const vector<operation>& ops, const flat_map<public_key_type,signature_type>& sigs,
                       const std::function<const authority*(account_id_type)>& get_active,
                       const std::function<const authority*(account_id_type)>& get_owner,
                       uint32_t max_recursion_depth,
                       bool  allow_committe,
                       const flat_set<account_id_type>& active_aprovals,
                       const flat_set<account_id_type>& owner_approvals )
{ try {
   flat_set<account_id_type> required_active;
   flat_set<account_id_type> required_owner;
   vector<authority> other;

   for( const auto& op : ops )
      operation_get_required_authorities( op, required_active, required_owner, other );

   if( !allow_committe )
      GRAPHENE_ASSERT( required_active.find(GRAPHENE_COMMITTEE_ACCOUNT) == required_active.end(),
                       invalid_committee_approval, "Committee account may only propose transactions" );

   sign_state s(sigs,get_active);
   s.max_recursion = max_recursion_depth;
   for( auto& id : active_aprovals )
      s.approved_by.insert( id );
   for( auto& id : owner_approvals )
      s.approved_by.insert( id );

   for( const auto& auth : other )
   {
      GRAPHENE_ASSERT( std::get<0>( s.check_authority(&auth) ), tx_missing_other_auth, "Missing Authority", ("auth",auth)("sigs",sigs) );
   }

   // fetch all of the top level authorities
   for( auto id : required_active )
   {
      GRAPHENE_ASSERT( std::get<0>( s.check_authority(id) ) ||
                       std::get<0>( s.check_authority(get_owner(id)) ),
                       tx_missing_active_auth, "Missing Active Authority ${id}", ("id",id)("auth",*get_active(id))("owner",*get_owner(id)) );
   }

   for( auto id : required_owner )
   {
      GRAPHENE_ASSERT( owner_approvals.find(id) != owner_approvals.end() ||
                       std::get<0>( s.check_authority(get_owner(id)) ),
                       tx_missing_owner_auth, "Missing Owner Authority ${id}", ("id",id)("auth",*get_owner(id)) );
   }

   GRAPHENE_ASSERT(
      !s.remove_unused_signatures(),
      tx_irrelevant_sig,
      "Unnecessary signature(s) detected"
      );
} FC_CAPTURE_AND_RETHROW( (ops)(sigs) ) }

void verify_authority( const vector<operation>& ops, const flat_map<public_key_type,signature_type>& sigs,
                          const std::function<const authority*(account_uid_type)>& get_owner_by_uid,
                          const std::function<const authority*(account_uid_type)>& get_active_by_uid,
                          const std::function<const authority*(account_uid_type)>& get_secondary_by_uid,
                          uint32_t max_recursion_depth,
                          bool allow_committe,
                          const flat_set<account_uid_type>& owner_uid_approvals,
                          const flat_set<account_uid_type>& active_uid_approvals,
                          const flat_set<account_uid_type>& secondary_uid_approvals)
{ try {
   flat_set<account_uid_type> required_owner_uids;
   flat_set<account_uid_type> required_active_uids;
   flat_set<account_uid_type> required_secondary_uids;
   vector<authority> other;

   for( const auto& op : ops )
      operation_get_required_uid_authorities( op, required_owner_uids, required_active_uids, required_secondary_uids, other );

   if( !allow_committe )
      GRAPHENE_ASSERT( required_active_uids.find(GRAPHENE_COMMITTEE_ACCOUNT_UID) == required_active_uids.end(),
                       invalid_committee_approval, "Committee account may only propose transactions" );

   // don't remove duplicates: if a transaction requires all the authorities, satisfy it
   /*
   for( const auto& uid : required_owner_uids )
   {
      required_active_uids.erase( uid );
      required_secondary_uids.erase( uid );
   }
   for( const auto& uid : required_active_uids )
   {
      required_secondary_uids.erase( uid );
   }
   */

   sign_state s( sigs, get_owner_by_uid, get_active_by_uid, get_secondary_by_uid );
   s.max_recursion = max_recursion_depth;
   for( auto& uid : owner_uid_approvals )
      s.approved_by_uid_auth.emplace( uid, authority::owner_auth );
   for( auto& uid : active_uid_approvals )
      s.approved_by_uid_auth.emplace( uid, authority::active_auth );
   for( auto& uid : secondary_uid_approvals )
      s.approved_by_uid_auth.emplace( uid, authority::secondary_auth );

   // TODO: review order to minimize number of signatures
   //       when changing the order, make sure to change get_required_signatures(...) as well, and set a hard fork

   // fetch all of the top level authorities
   for( auto uid : required_owner_uids )
   {
      GRAPHENE_ASSERT( std::get<0>( s.check_authority( authority::account_uid_auth_type( uid, authority::owner_auth ) ) ),
                       tx_missing_owner_auth,
                       "Missing Owner Authority ${uid}",
                       ( "uid", uid ) ( "owner", *get_owner_by_uid( uid ) )
                     );
   }

   // Can't use owner key to sign a transaction that requires active key
   for( auto uid : required_active_uids )
   {
      GRAPHENE_ASSERT( std::get<0>( s.check_authority( authority::account_uid_auth_type( uid, authority::active_auth ) ) ),
                       tx_missing_active_auth,
                       "Missing Active Authority ${uid}",
                       ( "uid", uid ) ( "active", *get_active_by_uid( uid ) )
                     );
   }

   // Can't use owner or active key to sign a transaction that requires secondary key
   for( auto uid : required_secondary_uids )
   {
      GRAPHENE_ASSERT( std::get<0>( s.check_authority( authority::account_uid_auth_type( uid, authority::secondary_auth ) ) ),
                       tx_missing_secondary_auth,
                       "Missing Secondary Authority ${uid}",
                       ( "uid", uid ) ( "secondary", *get_secondary_by_uid( uid ) )
                     );
   }

   for( const auto& auth : other )
   {
      GRAPHENE_ASSERT( std::get<0>( s.check_authority(&auth) ), tx_missing_other_auth, "Missing Authority", ("auth",auth)("sigs",sigs) );
   }

   GRAPHENE_ASSERT(
      !s.remove_unused_signatures(),
      tx_irrelevant_sig,
      "Unnecessary signature(s) detected"
      );
} FC_CAPTURE_AND_RETHROW( (ops)(sigs) ) }


flat_map<public_key_type,signature_type> signed_transaction::get_signature_keys( const chain_id_type& chain_id )const
{ try {
   auto d = sig_digest( chain_id );
   flat_map<public_key_type,signature_type> result;
   for( const auto&  sig : signatures )
   {
      const auto& key = fc::ecc::public_key(sig,d);
      GRAPHENE_ASSERT(
         result.find( key ) == result.end(),
         tx_duplicate_sig,
         "Duplicate Signature detected" );
      result[key] = sig;
   }
   return result;
} FC_CAPTURE_AND_RETHROW() }


std::tuple<flat_set<public_key_type>,flat_set<public_key_type>,flat_set<signature_type>> signed_transaction::get_required_signatures(
   const chain_id_type& chain_id,
   const flat_set<public_key_type>& available_keys,
   const std::function<const authority*(account_uid_type)>& get_owner_by_uid,
   const std::function<const authority*(account_uid_type)>& get_active_by_uid,
   const std::function<const authority*(account_uid_type)>& get_secondary_by_uid,
   uint32_t max_recursion_depth )const
{
   flat_set<account_uid_type> required_owner_uids;
   flat_set<account_uid_type> required_active_uids;
   flat_set<account_uid_type> required_secondary_uids;
   vector<authority> other;
   vector<authority> required;

   get_required_uid_authorities( required_owner_uids, required_active_uids, required_secondary_uids, other );

   required.reserve( required_owner_uids.size() + required_active_uids.size() + required_secondary_uids.size() + other.size() );
   // the order should be same as in verify_authority(...), changing the order requires a hard fork
   for( auto& uid : required_owner_uids )
      required.push_back( *get_owner_by_uid( uid ) );
   for( auto& uid : required_active_uids )
      required.push_back( *get_active_by_uid( uid ) );
   for( auto& uid : required_secondary_uids )
      required.push_back( *get_secondary_by_uid( uid ) );
   for( auto& auth : other )
      required.push_back( auth );


   auto key_sigs = get_signature_keys( chain_id );
   sign_state s( key_sigs, get_owner_by_uid, get_active_by_uid, get_secondary_by_uid, available_keys );
   s.max_recursion = max_recursion_depth;

   bool check_ok = true;
   flat_set<public_key_type> missed_keys;

   for( const auto& auth : required )
   {
      const auto& result = s.check_authority( &auth );
      check_ok = check_ok && std::get<0>( result );
      if( std::get<1>( result ) ) //possible
      {
         for( const auto& k : std::get<2>( result ) )
            missed_keys.insert( k );
      }
      else
      {
         missed_keys = std::get<2>( result );
         break;
      }
   }

   const auto& used_keys = s.get_used_keys();
   const auto& unused_sig_keys = s.get_unused_signature_keys();
   flat_set<signature_type> unused_sigs;
   for( const auto& key : unused_sig_keys )
      unused_sigs.insert( key_sigs[key] );

   return std::make_tuple( used_keys, missed_keys, unused_sigs );
}

set<public_key_type> signed_transaction::get_required_signatures(
   const chain_id_type& chain_id,
   const flat_set<public_key_type>& available_keys,
   const std::function<const authority*(account_id_type)>& get_active,
   const std::function<const authority*(account_id_type)>& get_owner,
   uint32_t max_recursion_depth )const
{
   flat_set<account_id_type> required_active;
   flat_set<account_id_type> required_owner;
   vector<authority> other;
   get_required_authorities( required_active, required_owner, other );


   sign_state s(get_signature_keys( chain_id ),get_active,available_keys);
   s.max_recursion = max_recursion_depth;

   for( const auto& auth : other )
      s.check_authority(&auth);
   for( auto& owner : required_owner )
      s.check_authority( get_owner( owner ) );
   for( auto& active : required_active )
      s.check_authority( active  );

   s.remove_unused_signatures();

   set<public_key_type> result;

   for( auto& provided_sig : s.provided_signatures )
      if( available_keys.find( provided_sig.first ) != available_keys.end() )
         result.insert( provided_sig.first );

   return result;
}

/*
set<public_key_type> signed_transaction::minimize_required_signatures(
   const chain_id_type& chain_id,
   const flat_set<public_key_type>& available_keys,
   const std::function<const authority*(account_id_type)>& get_active,
   const std::function<const authority*(account_id_type)>& get_owner,
   uint32_t max_recursion
   ) const
{
   set< public_key_type > s = get_required_signatures( chain_id, available_keys, get_active, get_owner, max_recursion );
   flat_set< public_key_type > result( s.begin(), s.end() );

   for( const public_key_type& k : s )
   {
      result.erase( k );
      try
      {
         graphene::chain::verify_authority( operations, result, get_active, get_owner, max_recursion );
         continue;  // element stays erased if verify_authority is ok
      }
      catch( const tx_missing_owner_auth& e ) {}
      catch( const tx_missing_active_auth& e ) {}
      catch( const tx_missing_other_auth& e ) {}
      result.insert( k );
   }
   return set<public_key_type>( result.begin(), result.end() );
}
*/

void signed_transaction::verify_authority(
   const chain_id_type& chain_id,
   const std::function<const authority*(account_id_type)>& get_active,
   const std::function<const authority*(account_id_type)>& get_owner,
   uint32_t max_recursion )const
{ try {
   graphene::chain::verify_authority( operations, get_signature_keys( chain_id ), get_active, get_owner, max_recursion );
} FC_CAPTURE_AND_RETHROW( (*this) ) }

void signed_transaction::verify_authority(
   const chain_id_type& chain_id,
   const std::function<const authority*(account_uid_type)>& get_owner_by_uid,
   const std::function<const authority*(account_uid_type)>& get_active_by_uid,
   const std::function<const authority*(account_uid_type)>& get_secondary_by_uid,
   uint32_t max_recursion )const
{ try {
   graphene::chain::verify_authority( operations,
                                      get_signature_keys( chain_id ),
                                      get_owner_by_uid,
                                      get_active_by_uid,
                                      get_secondary_by_uid,
                                      max_recursion );
} FC_CAPTURE_AND_RETHROW( (*this) ) }

} } // graphene::chain
