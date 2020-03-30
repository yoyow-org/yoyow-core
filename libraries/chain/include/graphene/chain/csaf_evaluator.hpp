/*
 * Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
 */
#pragma once
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/database.hpp>

namespace graphene { namespace chain {

   class csaf_collect_evaluator : public evaluator<csaf_collect_evaluator>
   {
      public:
         typedef csaf_collect_operation operation_type;

         void_result do_evaluate( const operation_type& o );
         void_result do_apply( const operation_type& o );

         const _account_statistics_object* from_stats = nullptr;
         const _account_statistics_object* to_stats = nullptr;
         fc::uint128_t                    available_coin_seconds;
         fc::uint128_t                    collecting_coin_seconds;
   };

   class csaf_lease_evaluator : public evaluator<csaf_lease_evaluator>
   {
      public:
         typedef csaf_lease_operation operation_type;

         void_result do_evaluate( const operation_type& o );
         object_id_type do_apply( const operation_type& o );

         const _account_statistics_object* from_stats = nullptr;
         const _account_statistics_object* to_stats = nullptr;
         const csaf_lease_object*         current_lease = nullptr;
         share_type                       delta;
   };

} } // graphene::chain
