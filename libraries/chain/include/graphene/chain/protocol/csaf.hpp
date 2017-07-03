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

   /**
    * @ingroup operations
    *
    * @brief Collect coin-seconds-as-fee to an account
    *
    *  Fees are paid by the "from" account
    *
    *  @return n/a
    */
   struct csaf_collect_operation : public base_operation
   {
      struct fee_parameters_type {
         uint64_t fee              = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee     = 0;
         uint16_t min_rf_percent   = 0;
         extensions_type   extensions;
      };

      fee_type                     fee;

      account_uid_type             from = 0;
      account_uid_type             to = 0;
      asset                        amount;

      extensions_type              extensions;

      account_uid_type fee_payer_uid()const { return from; }
      void            validate()const;
      //share_type      calculate_fee(const fee_parameters_type& k)const;
      void get_required_active_uid_authorities( flat_set<account_uid_type>& a )const
      {
         if( from != to )
            a.insert( from );
      }
      void get_required_secondary_uid_authorities( flat_set<account_uid_type>& a )const
      {
         if( from == to )
            a.insert( from );
      }
   };

   /**
    * @ingroup operations
    *
    * @brief Lease or stop leasing coin-seconds-as-fee to an account
    *
    *  Fees are paid by the "from" account
    *
    *  @return n/a
    */
   struct csaf_lease_operation : public base_operation
   {
      struct fee_parameters_type {
         uint64_t fee              = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee     = 0;
         uint16_t min_rf_percent   = 0;
         extensions_type   extensions;
      };

      fee_type                     fee;

      account_uid_type             from = 0;
      account_uid_type             to = 0;
      asset                        amount;
      time_point_sec               expiration;

      extensions_type              extensions;

      account_uid_type fee_payer_uid()const { return from; }
      void            validate()const;
      //share_type      calculate_fee(const fee_parameters_type& k)const;
      void get_required_active_uid_authorities( flat_set<account_uid_type>& a )const
      {
         a.insert( from );
      }
   };

}} // graphene::chain

FC_REFLECT( graphene::chain::csaf_collect_operation::fee_parameters_type, (fee)(min_real_fee)(min_rf_percent)(extensions) )
FC_REFLECT( graphene::chain::csaf_lease_operation::fee_parameters_type, (fee)(min_real_fee)(min_rf_percent)(extensions) )

FC_REFLECT( graphene::chain::csaf_collect_operation,
            (fee)
            (from)(to)(amount)
            (extensions) )
FC_REFLECT( graphene::chain::csaf_lease_operation,
            (fee)
            (from)(to)(amount)(expiration)
            (extensions) )

