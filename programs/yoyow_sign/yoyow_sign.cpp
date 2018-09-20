#include "yoyow_sign.h"
#include <fc/io/json.hpp>
//#include <graphene/chain/protocol/fee_schedule.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/protocol.hpp>
#include <graphene/chain/protocol/transaction.hpp>
#include <graphene/utilities/key_conversion.hpp>

using namespace graphene::chain;
using namespace graphene::utilities;
using namespace fc::ecc;

string signature(const string& tx, const string& wif, const string& chain_id)
{
   fc::optional< private_key > privkey = wif_to_key( wif );
   if( privkey.valid() ) {
      fc::optional<signed_transaction> trx = fc::json::from_string( tx ).as<signed_transaction>(GRAPHENE_MAX_NESTED_OBJECTS);
      if( trx.valid() ) {
         trx->sign( *privkey, chain_id_type( chain_id ));
      return fc::json::to_string(trx);
      }
   }
   return string("");
}

string generate_key(const string& brain_key, int sequence_number)
{
   std::string sequence_string = std::to_string(sequence_number);
   fc::sha512 h = fc::sha512::hash(brain_key + " " + sequence_string);
   private_key derived_key = private_key::regenerate(fc::sha256::hash(h));
   return key_to_wif(derived_key);
}

string private_to_public(const string& wif)
{
   fc::optional< private_key > privkey = wif_to_key( wif );
   if( privkey.valid() ) {
      public_key_type key(privkey->get_public_key());

      return string(key);
   }
   return string("");
}

void set_operation_fees( signed_transaction& tx, const fee_schedule& s  )
{
   for( auto& op : tx.operations )
      s.set_fee_with_csaf(op);
}

string base_transaction(const string& last_irreversible_block_id, 
                        const string& last_irreversible_block_time, 
                        const int64_t expiration )
{
   block_id_type block_id(last_irreversible_block_id);
   signed_transaction strx;
   strx.set_reference_block(block_id);
   time_point_sec time = time_point_sec::from_iso_string(last_irreversible_block_time);
   strx.set_expiration( time + fc::seconds(expiration) );
   return fc::json::to_string(strx);
}

string generate_transaction(const string& last_irreversible_block_id, 
                            const string& last_irreversible_block_time,
                            const string& from, 
                            const string& to, 
                            const string& amount, 
                            const string& memo,
                            const string& from_memo_private_wif,
                            const string& to_memo_public_key,
                            const string& current_fees_json,
                            const int64_t expiration,
                            const u_int64_t asset_id
                            )
{
   block_id_type block_id(last_irreversible_block_id);
   time_point_sec time = time_point_sec::from_iso_string(last_irreversible_block_time);
   fc::optional<account_uid_type> u_from = fc::variant(from).as<account_uid_type>(1);
   fc::optional<account_uid_type> u_to = fc::variant(to).as<account_uid_type>(1);
   fc::optional<share_type> a_amount = fc::variant(amount).as<share_type>(1);
   fc::optional<fee_schedule> fees = fc::json::from_string(current_fees_json).as<fee_schedule>( GRAPHENE_MAX_NESTED_OBJECTS );
   
   if( u_from.valid() && u_to.valid() && a_amount.valid() && fees.valid() ) {
      transfer_operation xfer_op;

      xfer_op.from = *u_from;
      xfer_op.to = *u_to;
      xfer_op.amount = asset(*a_amount, asset_id);

      if( memo.size() )
      {
         fc::optional< private_key > from_memo_key = wif_to_key( from_memo_private_wif );
         if( from_memo_key.valid() ) {
            public_key_type to_memo_key(to_memo_public_key);
            xfer_op.memo = memo_data();
            xfer_op.memo->from = from_memo_key->get_public_key();
            xfer_op.memo->to = to_memo_key;
            xfer_op.memo->set_message(*from_memo_key, to_memo_key, memo);
         }
      }

      signed_transaction tx;
      tx.set_reference_block(block_id);
      tx.set_expiration( time + fc::seconds(expiration) );
      tx.operations.push_back(xfer_op);
      set_operation_fees( tx, *fees);
      tx.validate();
      return fc::json::to_string(tx);
   }
   return string("");
}

string decrypt_memo(const string& memo_json, const string& memo_private_wif )
{
   string m_str;
   fc::optional<memo_data> memo = fc::json::from_string( memo_json ).as<memo_data>(GRAPHENE_MAX_NESTED_OBJECTS);
   fc::optional< private_key > memokey = wif_to_key( memo_private_wif );

   if( memo.valid() && memokey.valid() ) {
      m_str = memo->get_message( *memokey, memo->from );
   }
   return m_str;
}