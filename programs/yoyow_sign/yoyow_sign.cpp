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