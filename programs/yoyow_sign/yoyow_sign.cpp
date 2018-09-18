#include "yoyow_sign.h"
#include <fc/io/json.hpp>
//#include <graphene/chain/protocol/fee_schedule.hpp>
//#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/protocol.hpp>
#include <graphene/chain/protocol/transaction.hpp>
#include <graphene/utilities/key_conversion.hpp>

using namespace graphene::chain;
using namespace graphene::utilities;
using namespace fc::ecc;

string signature(string tx, string wif, string chain_id)
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

string generate_key(string brain_key, int sequence_number)
{
   std::string sequence_string = std::to_string(sequence_number);
   fc::sha512 h = fc::sha512::hash(brain_key + " " + sequence_string);
   private_key derived_key = private_key::regenerate(fc::sha256::hash(h));
   return key_to_wif(derived_key);
}

string private_to_public(string wif)
{
   fc::optional< private_key > privkey = wif_to_key( wif );
   if( privkey.valid() ) {
      public_key_type key(privkey->get_public_key());

      return string(key);
   }
   return string("");
}

string base_transaction(string last_irreversible_block_id, string last_irreversible_block_time)
{
   block_id_type block_id(last_irreversible_block_id);
   signed_transaction strx;
   strx.set_reference_block(block_id);
   time_point_sec time = time_point_sec::from_iso_string(last_irreversible_block_time);
   strx.set_expiration( time + fc::seconds(30) );
   return fc::json::to_string(strx);
}