/*
 * Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
 */
#include <graphene/chain/protocol/content.hpp>

#include "../utf8/checked.h"

namespace graphene { namespace chain {

void validate_platform_string( const string& str, const string& object_name = "", const int maxlen = GRAPHENE_MAX_PLATFORM_NAME_LENGTH )
{
   bool str_is_utf8 = true;
   bool str_too_long = false;
   uint32_t len = 0;
   uint32_t last_char = 0;

   auto itr = str.begin();
   while( itr != str.end() )
   {
      try
      {
         last_char = utf8::next( itr, str.end() );
         ++len;
         if( len > maxlen )
         {
            str_too_long = true;
            break;
         }
      }
      catch( utf8::invalid_utf8 e )
      {
         str_is_utf8 = false;
         break;
      }
   }

   FC_ASSERT( str_is_utf8, "platform ${o}should be in UTF-8", ("o", object_name) );
   FC_ASSERT( !str_too_long, "platform ${o}is too long", ("o", object_name)("length", len) );
}

void advertising_create_operation::validate()const
{
    validate_op_fee(fee, "advertising_create ");
    validate_account_uid(platform, "platform");
    FC_ASSERT(unit_time > 0, "unit time must be grater then 0");
    FC_ASSERT(unit_price > 0, "unit price must be grater then 0");
}

share_type advertising_create_operation::calculate_fee(const fee_parameters_type& k)const
{
    share_type core_fee_required = k.fee;
    auto hash_size = fc::raw::pack_size(description);
    if (hash_size > 65)
        core_fee_required += calculate_data_fee(hash_size, k.price_per_kbyte);
    return core_fee_required;
}

void advertising_update_operation::validate()const
{
    validate_op_fee(fee, "advertising_update ");
    validate_account_uid(platform, "platform");
    FC_ASSERT(description.valid() || unit_price.valid() || unit_time.valid() || on_sell.valid(), "must update some parameter");
    if (unit_price.valid()){
        FC_ASSERT(*unit_price > 0, "unit price must be greater then 0");
    }
    if (unit_time.valid()){
        FC_ASSERT(*unit_time > 0, "unit time must be greater then 0");
    }
}

share_type advertising_update_operation::calculate_fee(const fee_parameters_type& k)const
{
    share_type core_fee_required = k.fee;
    if (description.valid())
    {
        auto hash_size = fc::raw::pack_size(*description);
        if (hash_size > 65)
            core_fee_required += calculate_data_fee(hash_size, k.price_per_kbyte);
    }
    return core_fee_required;
}

void advertising_buy_operation::validate()const
{
    validate_op_fee(fee, "advertising_buy ");
    validate_account_uid(platform, "platform");
    validate_account_uid(from_account, "from_account");
    FC_ASSERT(platform != from_account, "platform can`t buy own advertising. ");
    FC_ASSERT(buy_number > 0, "buy number must greater then 0. ");
}

share_type advertising_buy_operation::calculate_fee(const fee_parameters_type& k)const
{
    share_type core_fee_required = k.fee;
    return core_fee_required;
}

void advertising_confirm_operation::validate()const
{
    validate_op_fee(fee, "advertising_comfirm ");
    validate_account_uid(platform, "platform");
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
    FC_ASSERT(order_sequence > 0, "the order sequence must be more then 0. ");
}

share_type advertising_ransom_operation::calculate_fee(const fee_parameters_type& k)const
{
    share_type core_fee_required = k.fee;
    return core_fee_required;
}

} } // graphene::chain
