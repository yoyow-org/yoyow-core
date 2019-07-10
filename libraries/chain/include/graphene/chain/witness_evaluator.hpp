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
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/witness_object.hpp>

namespace graphene { namespace chain {

   class witness_create_evaluator : public evaluator<witness_create_evaluator>
   {
      public:
         typedef witness_create_operation operation_type;

         void_result do_evaluate( const witness_create_operation& o );
         object_id_type do_apply( const witness_create_operation& o );

         const account_statistics_object* account_stats = nullptr;
         const account_object* account_obj = nullptr;
   };

   class witness_update_evaluator : public evaluator<witness_update_evaluator>
   {
      public:
         typedef witness_update_operation operation_type;

         void_result do_evaluate( const witness_update_operation& o );
         void_result do_apply( const witness_update_operation& o );

         const account_statistics_object* account_stats = nullptr;
         const witness_object* witness_obj = nullptr;
   };

   class witness_vote_update_evaluator : public evaluator<witness_vote_update_evaluator>
   {
      public:
         typedef witness_vote_update_operation operation_type;

         void_result do_evaluate( const witness_vote_update_operation& o );
         void_result do_apply( const witness_vote_update_operation& o );

         const account_statistics_object* account_stats = nullptr;
         const voter_object* voter_obj = nullptr;
         const voter_object* invalid_voter_obj = nullptr;
         const voter_object* invalid_current_proxy_voter_obj = nullptr;
         vector<const witness_object*> witnesses_to_add;
         vector<const witness_object*> witnesses_to_remove;
         vector<const witness_vote_object*> witness_votes_to_remove;
         vector<const witness_vote_object*> invalid_witness_votes_to_remove;
   };

   class witness_collect_pay_evaluator : public evaluator<witness_collect_pay_evaluator>
   {
      public:
         typedef witness_collect_pay_operation operation_type;

         void_result do_evaluate( const witness_collect_pay_operation& o );
         void_result do_apply( const witness_collect_pay_operation& o );

         const account_statistics_object* account_stats = nullptr;
   };

   class witness_report_evaluator : public evaluator<witness_report_evaluator>
   {
      public:
         typedef witness_report_operation operation_type;

         void_result do_evaluate( const witness_report_operation& o );
         void_result do_apply( const witness_report_operation& o );

         const account_statistics_object* account_stats = nullptr;
         uint32_t reporting_block_num = 0;
   };

   class account_pledge_update_evaluator : public evaluator < account_pledge_update_evaluator >
   {
      public:
         typedef account_pledge_update_operation operation_type;

         void_result do_evaluate(const account_pledge_update_operation& o);
         void_result do_apply(const account_pledge_update_operation& o);

         const witness_object* witness_obj = nullptr;
         const witness_pledge_object* witness_pledge_obj = nullptr;
         const account_statistics_object* account_stats = nullptr;
   };

} } // graphene::chain
