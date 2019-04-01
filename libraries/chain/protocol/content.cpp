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

void platform_create_operation::validate() const
{
   validate_op_fee( fee, "platform creation " );
   validate_account_uid( account, "platform " );
   validate_non_negative_core_asset( pledge, "pledge " );
   validate_platform_string( name, "name " );
   validate_platform_string( url, "url ", GRAPHENE_MAX_URL_LENGTH );
   validate_platform_string( extra_data, "extra_data ", GRAPHENE_MAX_PLATFORM_EXTRA_DATA_LENGTH );
}

share_type platform_create_operation::calculate_fee(const fee_parameters_type& k)const
{
   share_type core_fee_required = k.fee;
   core_fee_required += calculate_data_fee( fc::raw::pack_size(extra_data), k.price_per_kbyte );
   core_fee_required += calculate_data_fee( fc::raw::pack_size(name), k.price_per_kbyte );
   core_fee_required += calculate_data_fee( fc::raw::pack_size(url), k.price_per_kbyte );
   return core_fee_required;
}

void platform_update_operation::validate() const
{
   validate_op_fee( fee, "platform update " );
   validate_account_uid( account, "platform " );
   FC_ASSERT( new_pledge.valid() || new_name.valid() || new_url.valid() || new_extra_data.valid(), "Should change something" );
   if( new_pledge.valid() )
      validate_non_negative_core_asset( *new_pledge, "new pledge" );
   if( new_url.valid() )
      validate_platform_string( *new_url, "new url ", GRAPHENE_MAX_URL_LENGTH );
   if( new_name.valid() )
      validate_platform_string( *new_name, "new name " );
   if( new_extra_data.valid() )
      validate_platform_string( *new_extra_data, "new extra_data ", GRAPHENE_MAX_PLATFORM_EXTRA_DATA_LENGTH );
}

share_type platform_update_operation::calculate_fee(const fee_parameters_type& k)const
{
   share_type core_fee_required = k.fee;
   if( new_extra_data.valid() )
      core_fee_required += calculate_data_fee( fc::raw::pack_size(new_extra_data), k.price_per_kbyte );
   if( new_name.valid() )
      core_fee_required += calculate_data_fee( fc::raw::pack_size(new_name), k.price_per_kbyte );
   if( new_url.valid() )
      core_fee_required += calculate_data_fee( fc::raw::pack_size(new_url), k.price_per_kbyte );
   return core_fee_required;
}

void platform_vote_update_operation::validate()const
{
   validate_op_fee( fee, "platform vote update " );
   validate_account_uid( voter, "voter " );
   auto itr_add = platform_to_add.begin();
   auto itr_remove = platform_to_remove.begin();
   while( itr_add != platform_to_add.end() && itr_remove != platform_to_remove.end() )
   {
      FC_ASSERT( *itr_add != *itr_remove, "Can not add and remove same platform, uid: ${w}", ("w",*itr_add) );
      if( *itr_add < *itr_remove )
         ++itr_add;
      else
         ++itr_remove;
   }
   for( const auto uid : platform_to_add )
      validate_account_uid( uid, "platform " );
   for( const auto uid : platform_to_remove )
      validate_account_uid( uid, "platform " );
}

share_type platform_vote_update_operation::calculate_fee(const fee_parameters_type& k)const
{
   share_type core_fee_required = k.basic_fee;

   auto total_size = platform_to_add.size();
   core_fee_required += ( share_type( k.price_per_platform ) * total_size );

   return core_fee_required;
}

share_type post_operation::calculate_fee( const fee_parameters_type& schedule )const
{
   share_type core_fee_required = schedule.fee;
   auto hash_size = fc::raw::pack_size(hash_value);
   if( hash_size > 65 )
      core_fee_required += calculate_data_fee( hash_size, schedule.price_per_kbyte );
   core_fee_required += calculate_data_fee( fc::raw::pack_size(extra_data), schedule.price_per_kbyte );
   core_fee_required += calculate_data_fee( fc::raw::pack_size(title), schedule.price_per_kbyte );
   core_fee_required += calculate_data_fee( fc::raw::pack_size(body), schedule.price_per_kbyte );
   if (extensions.valid())
   {
       core_fee_required += calculate_data_fee(fc::raw::pack_size(extensions), schedule.price_per_kbyte);
   }
   return core_fee_required;
}


void post_operation::validate()const
{
   validate_op_fee( fee, "post " );
   validate_account_uid( poster, "poster " );
   validate_account_uid( platform, "platform" );
   FC_ASSERT( post_pid > uint64_t( 0 ), "post_pid must be greater than 0 ");
   bool flag = origin_poster.valid() == origin_post_pid.valid();
   bool flag2 = origin_poster.valid() == origin_platform.valid();
   FC_ASSERT( flag && flag2, "origin poster and origin post pid and origin platform should be all presented or all not " );
   if( origin_poster.valid() )
      validate_account_uid( *origin_poster, "origin poster " );
   if( origin_platform.valid() )
      validate_account_uid( *origin_platform, "origin platform " );
   if( origin_post_pid.valid() )
      FC_ASSERT( *origin_post_pid > uint64_t( 0 ), "origin_post_pid must be greater than 0 ");

   if (extensions.valid())
   {
       const post_operation::ext& ext = extensions->value;
       FC_ASSERT(ext.post_type.valid(), "post_operation`s extension post_type shouldn`t be null. ");
       FC_ASSERT(*(ext.post_type) >= Post_Type::Post_Type_Post && *(ext.post_type) < Post_Type::Post_Type_Default, 
                 "post_operation`s extension post_type is invalid");
       if (*(ext.post_type) == Post_Type::Post_Type_Comment
           || *(ext.post_type) == Post_Type::Post_Type_forward
           || *(ext.post_type) == Post_Type::Post_Type_forward_And_Modify)
           FC_ASSERT(origin_post_pid.valid() && origin_poster.valid() && origin_platform.valid(),
           "${type} post operation must include origin_post_pid, origin_poster and origin_platform", ("type", *(ext.post_type)));

       if (ext.receiptors.valid())
       {
           const map<account_uid_type, Recerptor_Parameter>& receiptor = *(ext.receiptors);
           FC_ASSERT(receiptor.size() >= 2 && receiptor.size() <= 5, "receiptors` size must be >= 2 and <= 5");
           auto itor = receiptor.find(platform);
           FC_ASSERT(itor != receiptor.end(), "platform must be included by receiptors");
           FC_ASSERT(itor->second.cur_ratio == GRAPHENE_DEFAULT_PLATFORM_RECERPTS_RATIO, "platform`s ratio must be 25%");
           auto itor_poster = receiptor.find(poster);
           FC_ASSERT(itor_poster != receiptor.end(), "poster must be included by receiptors");
           FC_ASSERT(itor_poster->second.cur_ratio >= GRAPHENE_DEFAULT_POSTER_MIN_RECERPTS_RATIO, "poster`s ratio must be >= 25%");
           uint32_t total = 0;
           for (auto iter : receiptor)
           {
               iter.second.validate();
               total += iter.second.cur_ratio;
           }
           FC_ASSERT(total == GRAPHENE_100_PERCENT, "The sum of receiptors` ratio must be 100%");
       }
       FC_ASSERT(ext.license_lid.valid(), "post operation`s license_lid is invalid.");
       if (ext.forward_price.valid())
           FC_ASSERT(*(ext.forward_price) > share_type(0), "buyout price should more than 0. ");
   }
}

void post_update_operation::validate()const
{
   validate_op_fee( fee, "post update " );
   validate_account_uid( poster, "poster " );
   validate_account_uid( platform, "platform " );
   FC_ASSERT( post_pid > uint64_t( 0 ), "post_pid must be greater than 0 ");
   FC_ASSERT(hash_value.valid() || extra_data.valid() || title.valid() || body.valid() || extensions.valid(),
             "Should change something");

   if (extensions.valid())
   {
       const post_update_operation::ext& ext = extensions->value;
       FC_ASSERT(ext.forward_price.valid() || ext.license_lid.valid() || ext.permission_flags.valid() || (ext.receiptor.valid() && ( ext.to_buyout.valid() ||
           ext.buyout_ratio.valid() || ext.buyout_price.valid() || ext.buyout_expiration.valid())),
           "Should change something");
       if (ext.receiptor.valid())
       {
           FC_ASSERT(ext.receiptor != platform, "The platform can`t change receiptor ratio");
       }
       if (ext.buyout_ratio.valid())
           FC_ASSERT(*ext.buyout_ratio <= (GRAPHENE_100_PERCENT - GRAPHENE_DEFAULT_PLATFORM_RECERPTS_RATIO), "buyout_ratio is more than max. ");
       if (ext.buyout_price.valid())
           FC_ASSERT(*(ext.buyout_price) > share_type(0), "buyout price should more than 0. ");
   }
}

share_type post_update_operation::calculate_fee( const fee_parameters_type& schedule )const
{
   share_type core_fee_required = schedule.fee;
   if( hash_value.valid() )
   {
      auto hash_size = fc::raw::pack_size(*hash_value);
      if( hash_size > 65 )
         core_fee_required += calculate_data_fee( hash_size, schedule.price_per_kbyte );
   }
   if( extra_data.valid() )
      core_fee_required += calculate_data_fee( fc::raw::pack_size(extra_data), schedule.price_per_kbyte );
   if( title.valid() )
      core_fee_required += calculate_data_fee( fc::raw::pack_size(title), schedule.price_per_kbyte );
   if( body.valid() )
      core_fee_required += calculate_data_fee( fc::raw::pack_size(body), schedule.price_per_kbyte );
   if (extensions.valid())
   {
       core_fee_required += calculate_data_fee(fc::raw::pack_size(extensions), schedule.price_per_kbyte);
   }
   return core_fee_required;
}

void score_create_operation::validate()const
{
	validate_op_fee(fee, "score ");
	validate_account_uid(poster, "poster ");
	validate_account_uid(platform, "platform");
	validate_account_uid(from_account_uid, "from account ");
	FC_ASSERT(post_pid > uint64_t(0), "post_pid must be greater than 0 ");
	FC_ASSERT((score >= -5) && (score <= 5), "The score_create_operation`s score over range");
    FC_ASSERT(csaf > 0, "The score_create_operation`s member points must more than 0.");
    FC_ASSERT(!extensions.valid(), "extension is currently not allowed");
}

share_type score_create_operation::calculate_fee(const fee_parameters_type& k)const
{
	share_type core_fee_required = k.fee;
	return core_fee_required;
}

void reward_operation::validate()const
{
	validate_op_fee(fee, "reward ");
	validate_account_uid(poster, "poster ");
	validate_account_uid(platform, "platform");
	validate_account_uid(from_account_uid, "from account ");
	FC_ASSERT(post_pid > uint64_t(0), "post_pid must be greater than 0 ");
	FC_ASSERT(amount.amount > share_type(0), "amount must be greater than 0 ");
    FC_ASSERT(!extensions.valid(), "extension is currently not allowed");
}

share_type reward_operation::calculate_fee(const fee_parameters_type& k)const
{
	share_type core_fee_required = k.fee;
	return core_fee_required;
}

void reward_proxy_operation::validate()const
{
    validate_op_fee(fee, "reward_proxy ");
    validate_account_uid(poster, "poster ");
    validate_account_uid(platform, "platform");
    validate_account_uid(from_account_uid, "from account ");
    FC_ASSERT(post_pid > uint64_t(0), "post_pid must be greater than 0 ");
    FC_ASSERT(amount > share_type(0), "amount must be greater than 0 ");
    FC_ASSERT(!extensions.valid(), "extension is currently not allowed");
}

share_type reward_proxy_operation::calculate_fee(const fee_parameters_type& k)const
{
    share_type core_fee_required = k.fee;
    return core_fee_required;
}

void buyout_operation::validate()const
{
	validate_op_fee(fee, "buyout ");
	validate_account_uid(poster, "poster ");
	validate_account_uid(platform, "platform");
	validate_account_uid(from_account_uid, "from account ");
	FC_ASSERT(post_pid > uint64_t(0), "post_pid must be greater than 0 ");
	FC_ASSERT(from_account_uid != platform, "from_account shouldn`t be platform");
	FC_ASSERT(receiptor_account_uid != platform, "platform shouldn`t sell out its receipt ratio");
	validate_account_uid(receiptor_account_uid, "from account ");
    FC_ASSERT(!extensions.valid(), "extension is currently not allowed");
}

share_type buyout_operation::calculate_fee(const fee_parameters_type& k)const
{
	share_type core_fee_required = k.fee;
	return core_fee_required;
}

void license_create_operation::validate()const
{
    validate_op_fee(fee, "license_create ");
    validate_account_uid(platform, "platform");
    FC_ASSERT(license_lid > uint64_t(0), "license_lid must be greater than 0 ");
    FC_ASSERT(!extensions.valid(), "extension is currently not allowed");
}

share_type license_create_operation::calculate_fee(const fee_parameters_type& k)const
{
    share_type core_fee_required = k.fee;
    auto hash_size = fc::raw::pack_size(hash_value);
    if (hash_size > 65)
        core_fee_required += calculate_data_fee(hash_size, k.price_per_kbyte);
    core_fee_required += calculate_data_fee(fc::raw::pack_size(extra_data), k.price_per_kbyte);
    core_fee_required += calculate_data_fee(fc::raw::pack_size(title), k.price_per_kbyte);
    core_fee_required += calculate_data_fee(fc::raw::pack_size(body), k.price_per_kbyte);
    return core_fee_required;
}

} } // graphene::chain
