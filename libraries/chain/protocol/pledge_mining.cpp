/*
* Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
*/
#include <graphene/chain/protocol/pledge_mining.hpp>

namespace graphene { namespace chain {

void pledge_mining_update_operation::validate() const
{
   validate_op_fee(fee, "account pledge update");
   validate_account_uid(pledge_account, "pledge account ");
   validate_account_uid(witness, "witness");
   validate_non_negative_amount(new_pledge, " new pledge amount");
   FC_ASSERT(pledge_account != witness, "witness pledge mining to oneself is not allowed");
   FC_ASSERT(!extensions.valid(), "extension is currently not allowed");
}

void pledge_bonus_collect_operation::validate() const
{
   validate_op_fee(fee, "pledge bonus collecting ");
   validate_account_uid(account, "pledge account ");
   validate_positive_core_asset(bonus, "bonus");
   FC_ASSERT(!extensions.valid(), "extension is currently not allowed");
}

} } // graphene::chain
