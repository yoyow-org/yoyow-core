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
#include <graphene/chain/content_evaluator.hpp>
#include <graphene/chain/content_object.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>

namespace graphene { namespace chain {

void_result post_evaluator::do_evaluate( const post_operation& op )
{ try {

   const database& d = db();

   d.get_platform_by_pid( op.platform ); // make sure pid exists
   poster_account  = &d.get_account_by_uid( op.poster );
   content         = d.find_content_by_cid( op.platform, op.poster, op.cid );

   FC_ASSERT( poster_account->can_post, "poster ${uid} is not allowed to post.", ("uid",op.poster) );

   if( content == nullptr ) // new content
   {
      if( op.parent_cid.valid() ) // is reply
         parent_content = &d.get_content_by_cid( op.platform, *op.parent_poster, *op.parent_cid );
      else
         parent_content = nullptr;
   }
   else
   {
      FC_ASSERT( !op.parent_cid.valid(), "should not specify parent when editing." );
   }

   return void_result();

}  FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type post_evaluator::do_apply( const post_operation& o )
{ try {
   database& d = db();

   if( content == nullptr ) // new content
   {
      const auto& new_content_object = d.create<content_object>( [&]( content_object& obj )
      {
         obj.platform         = o.platform;
         obj.poster           = o.poster;
         obj.cid              = o.cid;
         obj.parent_poster    = o.parent_poster;
         obj.parent_cid       = o.parent_cid;
         obj.options          = o.options;
         obj.hash_value       = o.hash_value;
         obj.extra_data       = o.extra_data;
         obj.title            = o.title;
         obj.body             = o.body;
         obj.create_time      = d.head_block_time();
         obj.last_update_time = d.head_block_time();
      } );
      return new_content_object.id;
   }
   else // edit
   {
      d.modify<content_object>( *content, [&]( content_object& obj )
      {
         //obj.options          = o.options;
         obj.hash_value       = o.hash_value;
         obj.extra_data       = o.extra_data;
         obj.title            = o.title;
         obj.body             = o.body;
         obj.last_update_time = d.head_block_time();
      } );
      return content->id;
   }
} FC_CAPTURE_AND_RETHROW( (o) ) }


} } // graphene::chain
