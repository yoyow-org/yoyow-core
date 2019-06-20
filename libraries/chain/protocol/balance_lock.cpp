/*
 * Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
 */
#include <graphene/chain/protocol/balance_lock.hpp>

#include "../utf8/checked.h"

namespace graphene { namespace chain {

void balance_lock_update_operation::validate()const
{
   validate_op_fee(fee, "balance_lock_update");
   validate_account_uid(account, "lock balance account");
   validate_non_negative_amount(new_lock_balance, " new lock balance amount");

   //FC_ASSERT(!extensions.valid(), "extension is currently not allowed");
}

} } // graphene::chain
