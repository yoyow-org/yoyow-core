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
#pragma once
#include <graphene/chain/protocol/base.hpp>

namespace graphene { namespace chain {

   struct post_options
   {
      extensions_type        extensions;

      void validate()const;
   };

   /**
    * @ingroup operations
    *
    * @brief Post an article or a reply
    *
    *  Fees are paid by the "poster" account
    *
    *  @return n/a
    */
   struct post_operation : public base_operation
   {
      struct fee_parameters_type {
         uint64_t fee       = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint32_t price_per_kbyte = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
         extensions_type   extensions;
      };

      asset                        fee;

      platform_pid_type            platform = 0;
      account_uid_type             poster = 0;
      post_pid_type                post_pid = 0;
      optional<account_uid_type>   parent_poster;
      optional<post_pid_type>      parent_post_pid;

      post_options                 options;

      string                       hash_value;
      string                       extra_data = "{}"; ///< category, tags and etc
      string                       title;
      string                       body;

      extensions_type              extensions;

      account_uid_type fee_payer_uid()const { return poster; }
      void            validate()const;
      share_type      calculate_fee(const fee_parameters_type& k)const;
      void get_required_secondary_uid_authorities( flat_set<account_uid_type>& a )const
      {
         a.insert( poster );
      }
   };

}} // graphene::chain

FC_REFLECT(graphene::chain::post_options, (extensions) )

FC_REFLECT( graphene::chain::post_operation::fee_parameters_type, (fee)(price_per_kbyte)(extensions) )

FC_REFLECT( graphene::chain::post_operation,
            (fee)
            (platform)(poster)(post_pid)(parent_poster)(parent_post_pid)
            (options)
            (hash_value)(extra_data)(title)(body)
            (extensions) )

