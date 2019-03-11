/*
* Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
*/
#pragma once
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/chain/protocol/custom_vote.hpp>
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/database.hpp>

namespace graphene { namespace chain {

class custom_vote_create_evaluator : public evaluator < custom_vote_create_evaluator >
{
public:
   typedef custom_vote_create_operation operation_type;

   void_result do_evaluate(const operation_type& op);
   object_id_type do_apply(const operation_type& op);
};

class custom_vote_cast_evaluator : public evaluator < custom_vote_cast_evaluator >
{
public:
   typedef custom_vote_cast_operation operation_type;

   void_result do_evaluate(const operation_type& op);
   object_id_type do_apply(const operation_type& op);
};
}} // graphene::chain
