/*
 * Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
 */
#pragma once
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/chain/protocol/advertising.hpp>
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/database.hpp>

namespace graphene { namespace chain {

   class advertising_create_evaluator : public evaluator < advertising_create_evaluator >
   {
   public:
       typedef advertising_create_operation operation_type;

       void_result do_evaluate(const operation_type& op);
       object_id_type do_apply(const operation_type& op);
   };

   class advertising_update_evaluator : public evaluator < advertising_update_evaluator >
   {
   public:
       typedef advertising_update_operation operation_type;

       void_result do_evaluate(const operation_type& op);
       void_result do_apply(const operation_type& op);

       const advertising_object* advertising_obj = nullptr;
   };

   class advertising_buy_evaluator : public evaluator < advertising_buy_evaluator >
   {
   public:
       typedef advertising_buy_operation operation_type;

       void_result do_evaluate(const operation_type& op);
       asset do_apply(const operation_type& op);

       share_type necessary_balance = 0;
       const advertising_object* advertising_obj = nullptr;
   };

   class advertising_confirm_evaluator : public evaluator < advertising_confirm_evaluator >
   {
   public:
       typedef advertising_confirm_operation operation_type;

       void_result do_evaluate(const operation_type& op);
       advertising_confirm_result do_apply(const operation_type& op);

       const advertising_order_object* advertising_order_obj = nullptr;
   };

   class advertising_ransom_evaluator : public evaluator < advertising_ransom_evaluator >
   {
   public:
       typedef advertising_ransom_operation operation_type;

       void_result do_evaluate(const operation_type& op);
       void_result do_apply(const operation_type& op);

       const advertising_order_object* advertising_order_obj = nullptr;
   };
} } // graphene::chain
