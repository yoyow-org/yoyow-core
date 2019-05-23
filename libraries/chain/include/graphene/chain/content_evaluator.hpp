/*
 * Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
 */
#pragma once
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/database.hpp>

namespace graphene { namespace chain {

class platform_create_evaluator : public evaluator<platform_create_evaluator>
   {
      public:
         typedef platform_create_operation operation_type;

         void_result do_evaluate( const platform_create_operation& op );
         object_id_type do_apply( const platform_create_operation& op );

         const account_statistics_object* account_stats = nullptr;
         const account_object* account_obj = nullptr;
   };

   class platform_update_evaluator : public evaluator<platform_update_evaluator>
   {
      public:
         typedef platform_update_operation operation_type;

         void_result do_evaluate( const platform_update_operation& o );
         void_result do_apply( const platform_update_operation& o );

         const account_statistics_object* account_stats = nullptr;
         const platform_object* platform_obj = nullptr;
   };

   class platform_vote_update_evaluator : public evaluator<platform_vote_update_evaluator>
   {
      public:
         typedef platform_vote_update_operation operation_type;

         void_result do_evaluate( const platform_vote_update_operation& o );
         void_result do_apply( const platform_vote_update_operation& o );

         const account_statistics_object* account_stats = nullptr;
         const voter_object* voter_obj = nullptr;
         const voter_object* invalid_voter_obj = nullptr;
         const voter_object* invalid_current_proxy_voter_obj = nullptr;
         vector<const platform_object*> platform_to_add;
         vector<const platform_object*> platform_to_remove;
         vector<const platform_vote_object*> platform_votes_to_remove;
         vector<const platform_vote_object*> invalid_platform_votes_to_remove;
   };

   class post_evaluator : public evaluator<post_evaluator>
   {
      public:
         typedef post_operation operation_type;

         void_result do_evaluate( const operation_type& o );
         object_id_type do_apply( const operation_type& o );

         const account_statistics_object* account_stats = nullptr;
         const post_operation::ext* ext_para = nullptr;
         const account_auth_platform_object* auth_object = nullptr;
         const post_object* origin_post = nullptr;
   };

   class post_update_evaluator : public evaluator<post_update_evaluator>
   {
      public:
         typedef post_update_operation operation_type;

         void_result do_evaluate( const operation_type& o );
         object_id_type do_apply( const operation_type& o );

         //const account_object* poster_account;
         const post_object*    post = nullptr;
         const post_update_operation::ext* ext_para = nullptr;
         const account_auth_platform_object* auth_object = nullptr;
   };

   class score_create_evaluator : public evaluator<score_create_evaluator>
   {
   public:
	   typedef score_create_operation operation_type;

	   void_result do_evaluate(const operation_type& o);
	   object_id_type do_apply(const operation_type& o);

       const account_auth_platform_object* auth_object = nullptr;
   };

   class reward_evaluator : public evaluator<reward_evaluator>
   {
   public:
	   typedef reward_operation operation_type;

	   void_result do_evaluate(const operation_type& o);
	   void_result do_apply(const operation_type& o);
   };

   class reward_proxy_evaluator : public evaluator<reward_proxy_evaluator>
   {
   public:
       typedef reward_proxy_operation operation_type;

       void_result do_evaluate(const operation_type& o);
       void_result do_apply(const operation_type& o);

       const account_auth_platform_object* auth_object = nullptr;
   };

   class buyout_evaluator : public evaluator<buyout_evaluator>
   {
   public:
	   typedef buyout_operation operation_type;

	   void_result do_evaluate(const operation_type& o);
	   void_result do_apply(const operation_type& o);
       const account_auth_platform_object* auth_object = nullptr;
   };

   class license_create_evaluator : public evaluator < license_create_evaluator >
   {
   public:
       typedef license_create_operation operation_type;

       void_result do_evaluate(const operation_type& op);
       object_id_type do_apply(const operation_type& op);

       const account_statistics_object* platform_ant = nullptr;
   };
} } // graphene::chain
