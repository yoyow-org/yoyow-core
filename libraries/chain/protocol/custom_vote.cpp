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

} } // graphene::chain
