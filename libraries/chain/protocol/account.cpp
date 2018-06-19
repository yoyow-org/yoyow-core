/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <graphene/chain/protocol/account.hpp>

#include "../utf8/checked.h"

namespace graphene { namespace chain {

void validate_account_name( const string& name, const string& object_name = "" )
{
   // valid name should:
   // 1. be encoded in UTF-8
   // 2. not be shorter than GRAPHENE_MIN_ACCOUNT_NAME_LENGTH (in term of UTF-8)
   // 3. not be longer than GRAPHENE_MAX_ACCOUNT_NAME_LENGTH (in term of UTF-8)
   // 4. contains only common Chinese chars and parentheses, Latin letters (lower case), numbers, underline
   // 5. not start with a number
   // 6. not start with a underline, not end with a underline

   bool name_is_utf8 = true;
   bool name_too_short = false;
   bool name_too_long = false;
   bool name_contains_invalid_char = false;
   bool name_start_with_number = false;
   bool name_start_with_underline = false;
   bool name_end_with_underline = false;

   uint32_t len = 0;
   uint32_t last_char = 0;
   auto itr = name.begin();
   while( itr != name.end() )
   {
      try
      {
         last_char = utf8::next( itr, name.end() );
         ++len;
         if( len > GRAPHENE_MAX_ACCOUNT_NAME_LENGTH )
         {
            name_too_long = true;
            break;
         }
         if( len == 1 )
         {
            if( last_char == '_')
            {
               name_start_with_underline = true;
               break;
            }
            if( last_char >= '0' && last_char <= '9' )
            {
               name_start_with_number = true;
               break;
            }
         }
         if( last_char != '_' &&
             !( last_char >= '0' && last_char <= '9' ) &&
             !( last_char >= 'a' && last_char <= 'z' ) &&
             !( last_char >= 0x4E00 && last_char <= 0x9FA5 ) &&
             !( last_char == 0xFF08 || last_char == 0xFF09 ) )
         {
            name_contains_invalid_char = true;
            break;
         }
      }
      catch( utf8::invalid_utf8 e )
      {
         name_is_utf8 = false;
         break;
      }
   }

   FC_ASSERT( name_is_utf8, "${o}account name should be in UTF-8", ("o", object_name) );

   FC_ASSERT( !name_start_with_number, "${o}account name should not start with a number", ("o", object_name) );
   FC_ASSERT( !name_start_with_underline, "${o}account name should not start with an underline", ("o", object_name) );

   FC_ASSERT( !name_contains_invalid_char, "${o}account name contains invalid character", ("o", object_name) );

   if( len > 0 && itr == name.end() && name_is_utf8 && last_char == '_' )
      name_end_with_underline = true;
   FC_ASSERT( !name_end_with_underline, "${o}account name should not end with an underline", ("o", object_name) );

   if( len < GRAPHENE_MIN_ACCOUNT_NAME_LENGTH )
      name_too_short = true;
   FC_ASSERT( !name_too_short, "${o}account name is too short", ("o", object_name)("length", len) );
   FC_ASSERT( !name_too_long, "${o}account name is too long", ("o", object_name)("length", len) );
}

void validate_new_authority( const authority& au, const string& object_name = "" )
{
   FC_ASSERT( au.num_auths() != 0, "${o}authority should contain something", ("o", object_name) );
   string uid_check_obj_name = object_name + "authority ";
   for( const auto& a : au.account_uid_auths )
      validate_account_uid( a.first.uid, uid_check_obj_name );
   FC_ASSERT( !au.is_impossible(), "cannot use an imposible ${o}authority threshold", ("o", object_name) );
}

void is_special_account( const account_uid_type uid, const string& object_name = "" )
{
   bool is_special_account = ( uid == GRAPHENE_COMMITTEE_ACCOUNT_UID ||
                               uid == GRAPHENE_WITNESS_ACCOUNT_UID ||
                               uid == GRAPHENE_RELAXED_COMMITTEE_ACCOUNT_UID ||
                               uid == GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID ||
                               uid == GRAPHENE_NULL_ACCOUNT_UID ||
                               uid == GRAPHENE_TEMP_ACCOUNT_UID );
   FC_ASSERT( !is_special_account, "Can not update this account ${uid}: ${o}", ("uid", uid)("o", object_name) );
}

/**
 * Names must comply with the following grammar (RFC 1035):
 * <domain> ::= <subdomain> | " "
 * <subdomain> ::= <label> | <subdomain> "." <label>
 * <label> ::= <letter> [ [ <ldh-str> ] <let-dig> ]
 * <ldh-str> ::= <let-dig-hyp> | <let-dig-hyp> <ldh-str>
 * <let-dig-hyp> ::= <let-dig> | "-"
 * <let-dig> ::= <letter> | <digit>
 *
 * Which is equivalent to the following:
 *
 * <domain> ::= <subdomain> | " "
 * <subdomain> ::= <label> ("." <label>)*
 * <label> ::= <letter> [ [ <let-dig-hyp>+ ] <let-dig> ]
 * <let-dig-hyp> ::= <let-dig> | "-"
 * <let-dig> ::= <letter> | <digit>
 *
 * I.e. a valid name consists of a dot-separated sequence
 * of one or more labels consisting of the following rules:
 *
 * - Each label is three characters or more
 * - Each label begins with a letter
 * - Each label ends with a letter or digit
 * - Each label contains only letters, digits or hyphens
 *
 * In addition we require the following:
 *
 * - All letters are lowercase
 * - Length is between (inclusive) GRAPHENE_MIN_ACCOUNT_NAME_LENGTH and GRAPHENE_MAX_ACCOUNT_NAME_LENGTH
 */
bool is_valid_name( const string& name )
{ try {
    const size_t len = name.size();

    if( len < GRAPHENE_MIN_ACCOUNT_NAME_LENGTH )
    {
          ilog( ".");
        return false;
    }

    if( len > GRAPHENE_MAX_ACCOUNT_NAME_LENGTH )
    {
          ilog( ".");
        return false;
    }

    size_t begin = 0;
    while( true )
    {
       size_t end = name.find_first_of( '.', begin );
       if( end == std::string::npos )
          end = len;
       if( (end - begin) < GRAPHENE_MIN_ACCOUNT_NAME_LENGTH )
       {
          idump( (name) (end)(len)(begin)(GRAPHENE_MAX_ACCOUNT_NAME_LENGTH) );
          return false;
       }
       switch( name[begin] )
       {
          case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h':
          case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
          case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x':
          case 'y': case 'z':
             break;
          default:
          ilog( ".");
             return false;
       }
       switch( name[end-1] )
       {
          case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h':
          case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
          case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x':
          case 'y': case 'z':
          case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
          case '8': case '9':
             break;
          default:
          ilog( ".");
             return false;
       }
       for( size_t i=begin+1; i<end-1; i++ )
       {
          switch( name[i] )
          {
             case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h':
             case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
             case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x':
             case 'y': case 'z':
             case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
             case '8': case '9':
             case '-':
                break;
             default:
          ilog( ".");
                return false;
          }
       }
       if( end == len )
          break;
       begin = end+1;
    }
    return true;
} FC_CAPTURE_AND_RETHROW( (name) ) }

bool is_cheap_name( const string& n )
{
   bool v = false;
   for( auto c : n )
   {
      if( c >= '0' && c <= '9' ) return true;
      if( c == '.' || c == '-' || c == '/' ) return true;
      switch( c )
      {
         case 'a':
         case 'e':
         case 'i':
         case 'o':
         case 'u':
         case 'y':
            v = true;
      }
   }
   if( !v )
      return true;
   return false;
}

void account_options::validate() const
{
   validate_account_uid( voting_account, "voting_account " );
   //extensions.validate();
}

void account_reg_info::validate() const
{
   validate_account_uid( registrar, "registrar " );
   validate_account_uid( referrer, "referrer " );
   validate_percentage( registrar_percent, "registrar_percent" );
   validate_percentage( referrer_percent,  "referrer_percent"  );
   validate_percentage( registrar_percent + referrer_percent, "registrar_percent plus referrer_percent" );
   validate_percentage( buyout_percent, "buyout_percent" );
   // assets should be core asset
   validate_core_asset_id( allowance_per_article, "allowance_per_article" );
   validate_core_asset_id( max_share_per_article, "max_share_per_article" );
   validate_core_asset_id( max_share_total,       "max_share_total"       );
   // ==== checks below are not needed
   // allowance_per_article should be >= 0
   // max_share_per_article should be >= 0
   // max_share_total should be >= 0

   //extensions.validate();
}

share_type account_create_operation::calculate_fee( const fee_parameters_type& k )const
{
   share_type core_fee_required = k.basic_fee;

   //if( !is_cheap_name(name) )
   //   core_fee_required = k.premium_fee;

   // Authorities and vote lists can be arbitrarily large, so charge a data fee for big ones
   //auto data_size = fc::raw::pack_size(owner) + fc::raw::pack_size(active) + fc::raw::pack_size(secondary);
   //auto data_fee =  calculate_data_fee( data_size, k.price_per_kbyte );
   //core_fee_required += data_fee;

   uint32_t total_auths = owner.num_auths() + active.num_auths() + secondary.num_auths();
   core_fee_required += ( share_type( k.price_per_auth ) * total_auths );

   return core_fee_required;
}


void account_create_operation::validate()const
{
   validate_op_fee( fee, "account creation " );
   validate_account_uid( uid, "new " );
   validate_account_name( name, "new " );
   validate_new_authority( owner, "new owner " );
   validate_new_authority( active, "new active " );
   validate_new_authority( secondary, "new secondary " );
   reg_info.validate();
}

void account_manage_operation::validate()const
{
   validate_op_fee( fee, "account manage " );
   validate_account_uid( executor, "executor " );
   validate_account_uid( account, "target " );
   const auto& o = options.value;
   bool has_option = o.can_post.valid() || o.can_reply.valid() || o.can_rate.valid();
   FC_ASSERT( has_option, "Should update something" );
}


void account_update_key_operation::validate()const
{
   validate_op_fee( fee, "account key update " );
   validate_account_uid( fee_paying_account, "fee paying " );
   validate_account_uid( uid, "target " );
   FC_ASSERT( old_key != new_key, "Should update something" );
   FC_ASSERT( update_active || update_secondary, "Should update something" );
}

share_type account_update_auth_operation::calculate_fee( const fee_parameters_type& k )const
{
   share_type core_fee_required = k.fee;

   // Authorities can be arbitrarily large, so charge a data fee for big ones
   uint32_t total_auths = 0;
   if( owner.valid() )
      total_auths += owner->num_auths();
   if( active.valid() )
      total_auths += active->num_auths();
   if( secondary.valid() )
      total_auths += secondary->num_auths();
   if( total_auths > 0 )
      core_fee_required += ( share_type( k.price_per_auth ) * total_auths );

   return core_fee_required;
}

void account_update_auth_operation::validate()const
{
   validate_op_fee( fee, "account authority update " );
   validate_account_uid( uid, "target " );
   is_special_account( uid );
   FC_ASSERT( owner.valid() || active.valid() || secondary.valid() || memo_key.valid(), "Should update something" );
   if( owner.valid() )
      validate_new_authority( *owner, "new owner " );
   if( active.valid() )
      validate_new_authority( *active, "new active " );
   if( secondary.valid() )
      validate_new_authority( *secondary, "new secondary " );
}

void account_auth_platform_operation::validate()const
{
   validate_op_fee( fee, "account authority platform " );
   validate_account_uid( uid, "target " );
   is_special_account( uid );
   validate_account_uid( platform, "platform " );
}

void account_cancel_auth_platform_operation::validate()const
{
   validate_op_fee( fee, "cancel account authority platform " );
   validate_account_uid( uid, "target " );
   is_special_account( uid );
   validate_account_uid( platform, "platform " );
}

void account_update_proxy_operation::validate() const
{
   validate_op_fee( fee, "account voting proxy update " );
   validate_account_uid( voter, "voter " );
   validate_account_uid( proxy, "proxy " );
   FC_ASSERT( voter != proxy, "voter and proxy should not be same" );
}

} } // graphene::chain
