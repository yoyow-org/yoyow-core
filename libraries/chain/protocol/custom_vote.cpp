/*
 * Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
 */
#include <graphene/chain/protocol/custom_vote.hpp>

#include "../utf8/checked.h"

namespace graphene { namespace chain {

void custom_vote_create_operation::validate()const
{
   //validate_op_fee(fee, "advertising_create ");
   //validate_account_uid(platform, "platform");
}

share_type custom_vote_create_operation::calculate_fee(const fee_parameters_type& k)const
{
   share_type core_fee_required = k.fee;
   auto hash_size = fc::raw::pack_size(description);
   if (hash_size > 65)
      core_fee_required += calculate_data_fee(hash_size, k.price_per_kbyte);
   return core_fee_required;
}

void custom_vote_cast_operation::validate()const
{
   //validate_op_fee(fee, "advertising_update ");
   //validate_account_uid(platform, "platform");
}

share_type custom_vote_cast_operation::calculate_fee(const fee_parameters_type& k)const
{
   share_type core_fee_required = k.fee;
   return core_fee_required;
}

} } // graphene::chain
