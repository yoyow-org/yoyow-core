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
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/transaction_evaluation_state.hpp>
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/chain/protocol/transaction.hpp>

namespace graphene { namespace chain {

   class database;
   struct signed_transaction;
   class generic_evaluator;
   class transaction_evaluation_state;

   class generic_evaluator
   {
   public:
      virtual ~generic_evaluator(){}

      virtual int get_type()const = 0;
      virtual operation_result start_evaluate(transaction_evaluation_state& eval_state, const operation& op, bool apply);

      /**
       * @note derived classes should ASSUME that the default validation that is
       * indepenent of chain state should be performed by op.validate() and should
       * not perform these extra checks.
       */
      virtual operation_result evaluate(const operation& op) = 0;
      virtual operation_result apply(const operation& op) = 0;

      database& db()const;

      void set_signed_information(const signed_information& s){ sigs = s; }

	  void set_billed_cpu_time_us(const uint32_t& s){ _billed_cpu_time_us = s; }
      //void check_required_authorities(const operation& op);
   protected:
      /**
       * @brief Fetch objects relevant to fee payer and set pointer members
       * @param account_id Account which is paying the fee
       * @param fee The fee being paid. Must be in CORE asset.
       *
       * This method verifies that the fee is valid and sets the object pointer members and the fee fields. It should
       * be called during do_evaluate.
       *
       */
      void prepare_fee(account_id_type account_id, asset fee);
      void prepare_fee(account_uid_type account_uid, asset fee);
      void prepare_fee(asset fee); // be called after fee_paying_account initialized
      void prepare_fee(account_uid_type account_uid, const fee_type& fee);
      void prepare_fee(const fee_type& fee); // be called after fee_paying_account initialized

      /**
       * Process fee_options.
       */
      void process_fee_options();

      /**
       * Calculate fee for an operation.
       */
      share_type calculate_fee_for_operation(const operation& op) const;

      /**
       * Calculate fee pair for an operation.
       * @return A pair, the first is total fee required, the second is minimum non-csaf fee required
       */
      std::pair<share_type,share_type> calculate_fee_pair_for_operation(const operation& op) const;

      // the next two functions are helpers that allow template functions declared in this 
      // header to call db() without including database.hpp, which would
      // cause a circular dependency
      void db_adjust_balance(const account_id_type& fee_payer, asset fee_from_account);
      void db_adjust_balance(const account_uid_type& fee_payer, asset fee_from_account);

      // helper functions for error report
      string db_to_pretty_string( const asset& a )const;
      string db_to_pretty_core_string( const share_type amount )const;

      asset                            fee_from_account;
      share_type                       total_fee_paid;
      share_type                       from_balance;
      share_type                       from_prepaid;
      share_type                       from_csaf;
      const account_object*            fee_paying_account = nullptr;
      const _account_statistics_object* fee_paying_account_statistics = nullptr;
      transaction_evaluation_state*    trx_state;
      signed_information               sigs;
	  uint32_t			   			   _billed_cpu_time_us;
   };

   class op_evaluator
   {
   public:
      virtual ~op_evaluator(){}
      virtual operation_result evaluate(transaction_evaluation_state& eval_state, const operation& op, bool apply, const signed_information& sigs,const uint32_t& billed_cpu_time_us) = 0;
   };

   template<typename T>
   class op_evaluator_impl : public op_evaluator
   {
   public:
      virtual operation_result evaluate(transaction_evaluation_state& eval_state, const operation& op, bool apply = true, const signed_information& sigs = signed_information(),const uint32_t& billed_cpu_time_us=0) override
      {
         T eval;
         eval.set_signed_information(sigs);
		 eval.set_billed_cpu_time_us(billed_cpu_time_us);
         return eval.start_evaluate(eval_state, op, apply);
      }
   };

   template<typename DerivedEvaluator>
   class evaluator : public generic_evaluator
   {
   public:
      virtual int get_type()const override { return operation::tag<typename DerivedEvaluator::operation_type>::value; }

      virtual operation_result evaluate(const operation& o) final override
      {
         auto* eval = static_cast<DerivedEvaluator*>(this);
         const auto& op = o.get<typename DerivedEvaluator::operation_type>();

         prepare_fee(op.fee_payer_uid(), op.fee);
         if( !trx_state->skip_fee_schedule_check )
         {
            std::pair<share_type,share_type> required_fee_pair = calculate_fee_pair_for_operation(op);
            GRAPHENE_ASSERT( total_fee_paid >= required_fee_pair.first,
                       insufficient_fee,
                       "Insufficient Total Fee Paid: need ${r}, provided ${p}",
                       ("p",db_to_pretty_core_string(total_fee_paid))
                       ("r",db_to_pretty_core_string(required_fee_pair.first)) );
            GRAPHENE_ASSERT( from_balance + from_prepaid >= required_fee_pair.second,
                       insufficient_fee,
                       "Insufficient Real Fee Paid: need ${r}, provided ${fb} from balance and ${fp} from prepaid",
                       ("fb",db_to_pretty_core_string(from_balance))
                       ("fp",db_to_pretty_core_string(from_prepaid))
                       ("r" ,db_to_pretty_core_string(required_fee_pair.second)) );
         }

         return eval->do_evaluate(op);
      }

      virtual operation_result apply(const operation& o) final override
      {
         auto* eval = static_cast<DerivedEvaluator*>(this);
         const auto& op = o.get<typename DerivedEvaluator::operation_type>();

         auto result = eval->do_apply(op);

         if( fee_from_account.amount > 0 )
            db_adjust_balance(op.fee_payer_uid(), -fee_from_account);

         process_fee_options();

         return result;
      }
   };
} }
