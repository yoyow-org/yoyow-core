/*
 * Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
 */
#include <graphene/chain/protocol/csaf.hpp>

namespace graphene { namespace chain {

void csaf_collect_operation::validate()const
{
   validate_op_fee( fee, "csaf collect operation " );
   validate_account_uid( from, "from " );
   validate_account_uid( to, "to " );
   validate_positive_core_asset( amount, "csaf collect amount" );
   FC_ASSERT( time.sec_since_epoch() % 60 == 0, "time should be rounded down to nearest minute" );
}

void csaf_lease_operation::validate()const
{
   validate_op_fee( fee, "csaf lease operation " );
   validate_account_uid( from, "from " );
   validate_account_uid( to, "to " );
   validate_non_negative_core_asset( amount, "csaf lease amount" );
   FC_ASSERT( from != to, "can not lease to self." );
}



} } // graphene::chain
