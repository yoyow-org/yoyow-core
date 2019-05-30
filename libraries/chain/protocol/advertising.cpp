/*
 * Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
 */
#include <graphene/chain/protocol/advertising.hpp>

#include "../utf8/checked.h"

namespace graphene { namespace chain {

void advertising_create_operation::validate()const
{
   validate_op_fee(fee, "advertising_create ");
   validate_account_uid(platform, "platform");
   FC_ASSERT(unit_time > 0, "unit time must be grater than 0");
   FC_ASSERT(unit_price > 0, "unit price must be grater than 0");
   FC_ASSERT(!extensions.valid(), "extension is currently not allowed");
}

share_type advertising_create_operation::calculate_fee(const fee_parameters_type& k)const
{
   share_type core_fee_required = k.fee;
   auto data_size = fc::raw::pack_size(description);
   core_fee_required += calculate_data_fee(data_size, k.price_per_kbyte);
   return core_fee_required;
}

void advertising_update_operation::validate()const
{
   validate_op_fee(fee, "advertising_update ");
   validate_account_uid(platform, "platform");
   FC_ASSERT(description.valid() || unit_price.valid() || unit_time.valid() || on_sell.valid(), "must update some parameter");
   FC_ASSERT(advertising_aid > 0, "advertising_aid should more than 0. ");
   if (unit_price.valid()){
      FC_ASSERT(*unit_price > 0, "unit price must be greater than 0");
   }
   if (unit_time.valid()){
      FC_ASSERT(*unit_time > 0, "unit time must be greater than 0");
   }
   FC_ASSERT(!extensions.valid(), "extension is currently not allowed");
}

share_type advertising_update_operation::calculate_fee(const fee_parameters_type& k)const
{
   share_type core_fee_required = k.fee;
   if (description.valid())
   {
      auto data_size = fc::raw::pack_size(*description);
      core_fee_required += calculate_data_fee(data_size, k.price_per_kbyte);
   }
   return core_fee_required;
}

void advertising_buy_operation::validate()const
{
   validate_op_fee(fee, "advertising_buy ");
   validate_account_uid(platform, "platform");
   validate_account_uid(from_account, "from_account");
   FC_ASSERT(platform != from_account, "platform can`t buy own advertising. ");
   FC_ASSERT(buy_number > 0, "buy number must greater than 0. ");
   FC_ASSERT(advertising_aid > 0, "advertising_aid should more than 0. ");
   FC_ASSERT(!extensions.valid(), "extension is currently not allowed");
}

share_type advertising_buy_operation::calculate_fee(const fee_parameters_type& k)const
{
   share_type core_fee_required = k.fee;
   if (memo)
      core_fee_required += calculate_data_fee(fc::raw::pack_size(memo), k.price_per_kbyte);
   auto size = fc::raw::pack_size(extra_data);
   core_fee_required += calculate_data_fee(size, k.price_per_kbyte);
   return core_fee_required;
}

void advertising_confirm_operation::validate()const
{
   validate_op_fee(fee, "advertising_confirm ");
   validate_account_uid(platform, "platform");
   FC_ASSERT(advertising_aid > 0, "advertising_aid should more than 0. ");
   FC_ASSERT(!extensions.valid(), "extension is currently not allowed");
}

share_type advertising_confirm_operation::calculate_fee(const fee_parameters_type& k)const
{
   share_type core_fee_required = k.fee;
   return core_fee_required;
}

void advertising_ransom_operation::validate()const
{
   validate_op_fee(fee, "advertising_ransom ");
   validate_account_uid(platform, "platform");
   validate_account_uid(from_account, "from_account");
   FC_ASSERT(advertising_aid > 0, "advertising_aid should more than 0. ");
   FC_ASSERT(!extensions.valid(), "extension is currently not allowed");
}

share_type advertising_ransom_operation::calculate_fee(const fee_parameters_type& k)const
{
   share_type core_fee_required = k.fee;
   return core_fee_required;
}

} } // graphene::chain
