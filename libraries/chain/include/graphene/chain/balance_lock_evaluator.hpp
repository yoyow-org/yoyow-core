/*
* Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
*/
#pragma once
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/chain/protocol/balance_lock.hpp>
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/protocol/types.hpp>

namespace graphene { namespace chain {

class balance_lock_update_evaluator : public evaluator < balance_lock_update_evaluator >
{
public:
   typedef balance_lock_update_operation operation_type;

   void_result do_evaluate(const operation_type& op);
   void_result do_apply(const operation_type& op);

   const account_statistics_object* account_stats = nullptr; 
   const pledge_balance_object* pledge_balance_obj = nullptr;
};

}} // graphene::chain
