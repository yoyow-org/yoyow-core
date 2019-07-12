/*
* Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
*/
#pragma once
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/pledge_mining_object.hpp>

namespace graphene { namespace chain {

   class pledge_mining_update_evaluator : public evaluator < pledge_mining_update_evaluator >
   {
      public:
         typedef pledge_mining_update_operation operation_type;

         void_result do_evaluate(const pledge_mining_update_operation& o);
         void_result do_apply(const pledge_mining_update_operation& o);

         const witness_object* witness_obj = nullptr;
         const pledge_mining_object* pledge_mining_obj = nullptr;
         const account_statistics_object* account_stats = nullptr;
   };

   class pledge_bonus_collect_evaluator : public evaluator < pledge_bonus_collect_evaluator >
   {
   public:
      typedef pledge_bonus_collect_operation operation_type;

      void_result do_evaluate(const pledge_bonus_collect_operation& o);
      void_result do_apply(const pledge_bonus_collect_operation& o);

      const account_statistics_object* account_stats = nullptr;
   };

} } // graphene::chain
