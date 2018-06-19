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
#include <graphene/chain/protocol/operations.hpp>

namespace graphene { namespace chain {

uint64_t base_operation::calculate_data_fee( uint64_t bytes, uint64_t price_per_kbyte )
{
   auto result = (fc::uint128(bytes) * price_per_kbyte) / 1024;
   FC_ASSERT( result <= GRAPHENE_MAX_SHARE_SUPPLY );
   return result.to_uint64();
}

/**
 * @brief Used to validate operations in a polymorphic manner
 */
struct operation_validator
{
   typedef void result_type;
   template<typename T>
   void operator()( const T& v )const { v.validate(); }
};

struct operation_get_required_uid_auth
{
   typedef void result_type;

   flat_set<account_uid_type>& owner_uids;
   flat_set<account_uid_type>& active_uids;
   flat_set<account_uid_type>& secondary_uids;
   vector<authority>&          other;


   operation_get_required_uid_auth( flat_set<account_uid_type>& own,
     flat_set<account_uid_type>& a,
     flat_set<account_uid_type>& s,
     vector<authority>&  oth ):owner_uids(own),active_uids(a),secondary_uids(s),other(oth){}

   template<typename T>
   void do_get_basic(const T& v)const
   {
      v.get_required_owner_uid_authorities( owner_uids );
      v.get_required_active_uid_authorities( active_uids );
      v.get_required_secondary_uid_authorities( secondary_uids );
      v.get_required_authorities( other );
   }

   template<typename T>
   void do_get_with_extended_fee(const T& v)const
   {
      do_get_basic( v );

      // fee can be paid with any authority of the payer.
      // TODO: 1. review the order to minimize number of signatures. can also check before add
      //       2. review the depth,
      //       3. less important authority can only pay less fee
      const auto fee_payer_uid = v.fee_payer_uid();
      const auto& fo = v.fee.options;
      // fee from balance should be paid with active auth or owner auth
      if( v.fee.total.amount == 0 )
      {
         // don't need additional authority for zero fee operations.
      }
      else if( !fo.valid() || ( fo->value.from_balance.valid() && fo->value.from_balance->amount != 0 ) )
      {
         // no fee option, or
         // special fee option with a non-zero from_balance value,
         other.push_back( authority( 1,
                             authority::account_uid_auth_type( fee_payer_uid, authority::owner_auth ), 1,
                             authority::account_uid_auth_type( fee_payer_uid, authority::active_auth ), 1
                        ) );
      }
      else
      {
         other.push_back( authority( 1,
                             authority::account_uid_auth_type( fee_payer_uid, authority::owner_auth ), 1,
                             authority::account_uid_auth_type( fee_payer_uid, authority::active_auth ), 1,
                             authority::account_uid_auth_type( fee_payer_uid, authority::secondary_auth ), 1
                        ) );
      }
   }

   BOOST_TTI_HAS_MEMBER_DATA(fee)

   template<typename T>
   void do_get(const T& v)const
   {
      do_get_basic( v );

      // fee can be paid with any authority of the payer.
      // TODO: 1. review the order to minimize number of signatures. can also check before add
      //       2. review the depth,
      //       3. less important authority can only pay less fee
      const auto fee_payer_uid = v.fee_payer_uid();
      authority fee_payer_auth( 1, authority::account_uid_auth_type( fee_payer_uid, authority::owner_auth ), 1,
                                   authority::account_uid_auth_type( fee_payer_uid, authority::active_auth ), 1,
                                   authority::account_uid_auth_type( fee_payer_uid, authority::secondary_auth ), 1 );
      other.push_back( fee_payer_auth );
   }

   template<typename T>
   void operator()( const T& v, std::true_type )const
   {
      do_get_with_extended_fee(v);
   }

   template<typename T>
   void operator()( const T& v, std::false_type )const
   {
      FC_ASSERT( false, "not implemented." );
   }

   template<typename T>
   void operator()( const T& v )const
   {
      const bool b = has_member_data_fee< T, fee_type >::value;
      operator()( v, std::integral_constant<bool, b>() );
   }
};

void operation_validate( const operation& op )
{
   op.visit( operation_validator() );
}

void operation_get_required_uid_authorities( const operation& op,
                                         flat_set<account_uid_type>& owner_uids,
                                         flat_set<account_uid_type>& active_uids,
                                         flat_set<account_uid_type>& secondary_uids,
                                         vector<authority>&  other )
{
   op.visit( operation_get_required_uid_auth( owner_uids, active_uids, secondary_uids, other ) );
}

void validate_account_uid( const account_uid_type uid, const string& object_name )
{
   FC_ASSERT( is_valid_account_uid( uid ), "${o}account uid ${u} is not valid.", ("o", object_name)("u", uid) );
}
void validate_asset_id( const asset& a, asset_aid_type aid, const string& object_name )
{
   FC_ASSERT( a.asset_id == aid, "asset_id of ${o} should be ${aid}.", ("o", object_name)("aid", aid) );
}
void validate_core_asset_id( const asset& a, const string& object_name )
{
   validate_asset_id( a, GRAPHENE_CORE_ASSET_AID, object_name );
}
void validate_non_core_asset_id( const asset& a, const string& object_name )
{
   FC_ASSERT( a.asset_id != GRAPHENE_CORE_ASSET_AID, "asset_id of ${o} should not be 0.", ("o", object_name) );
}
void validate_op_fee( const asset& fee, const string& op_name )
{
   validate_non_negative_core_asset( fee, op_name + "fee" );
}
void validate_op_fee( const fee_type& fee, const string& op_name )
{
   validate_op_fee( fee.total, op_name + "total ");
   if( fee.options.valid() )
   {
      const auto& fov = fee.options->value;
      asset total; // 0 CORE
      if( fov.from_balance.valid() )
      {
         validate_op_fee( *fov.from_balance, op_name + "from_balance " );
         total += *fov.from_balance;
      }
      if( fov.from_prepaid.valid() )
      {
         validate_op_fee( *fov.from_prepaid, op_name + "from_prepaid " );
         total += *fov.from_prepaid;
      }
      if( fov.from_csaf.valid() )
      {
         validate_op_fee( *fov.from_csaf, op_name + "from_csaf " );
         total += *fov.from_csaf;
      }
      FC_ASSERT( total.amount == fee.total.amount, "${o}total fee should be equal to sum of fees in options.", ("o", op_name) );
   }
}
void validate_percentage( uint16_t percent, const string& object_name )
{
   FC_ASSERT( percent <= GRAPHENE_100_PERCENT, "${o} should not exceed 100%.", ("o", object_name) );
}
void validate_positive_amount( share_type amount, const string& object_name )
{
   FC_ASSERT( amount > 0, "${o} should be positive.", ("o", object_name) );
}
void validate_non_negative_amount( share_type amount, const string& object_name )
{
   FC_ASSERT( amount >= 0, "${o} should not be negative.", ("o", object_name) );
}
void validate_positive_core_asset( const asset& a, const string& object_name )
{
   validate_core_asset_id( a, object_name );
   validate_positive_amount( a.amount, object_name + " amount" );
}
void validate_positive_asset( const asset& a, const string& object_name )
{
   validate_positive_amount( a.amount, object_name + " amount" );
}
void validate_non_negative_core_asset( const asset& a, const string& object_name )
{
   validate_core_asset_id( a, object_name );
   validate_non_negative_amount( a.amount, object_name + " amount" );
}
void validate_non_negative_asset( const asset& a, const string& object_name )
{
   validate_non_negative_amount( a.amount, object_name + " amount" );
}

} } // namespace graphene::chain
