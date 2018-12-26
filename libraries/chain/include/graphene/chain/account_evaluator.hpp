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
#include <graphene/chain/account_object.hpp>

namespace graphene { namespace chain {

class account_create_evaluator : public evaluator<account_create_evaluator>
{
public:
   typedef account_create_operation operation_type;

   void_result do_evaluate( const account_create_operation& o );
   object_id_type do_apply( const account_create_operation& o ) ;
};

class account_manage_evaluator : public evaluator<account_manage_evaluator>
{
public:
   typedef account_manage_operation operation_type;

   void_result do_evaluate( const operation_type& o );
   void_result do_apply( const operation_type& o ) ;

   const account_object* acnt = nullptr;
};

class account_update_key_evaluator : public evaluator<account_update_key_evaluator>
{
public:
   typedef account_update_key_operation operation_type;

   void_result do_evaluate( const operation_type& o );
   void_result do_apply( const operation_type& o ) ;

   const account_object* acnt = nullptr;
   flat_map<public_key_type,weight_type>::const_iterator itr_active;
   flat_map<public_key_type,weight_type>::const_iterator itr_secondary;
   weight_type active_weight = 0;
   weight_type secondary_weight = 0;
};

class account_update_auth_evaluator : public evaluator<account_update_auth_evaluator>
{
public:
   typedef account_update_auth_operation operation_type;

   void_result do_evaluate( const operation_type& o );
   void_result do_apply( const operation_type& o ) ;

   const account_object* acnt = nullptr;
};

class account_auth_platform_evaluator : public evaluator<account_auth_platform_evaluator>
{
public:
   typedef account_auth_platform_operation operation_type;

   void_result do_evaluate( const operation_type& o );
   void_result do_apply( const operation_type& o ) ;

   const account_object* acnt = nullptr;
   const account_auth_platform_operation::ext* ext = nullptr;
   bool found = false;
};

class account_cancel_auth_platform_evaluator : public evaluator<account_cancel_auth_platform_evaluator>
{
public:
   typedef account_cancel_auth_platform_operation operation_type;

   void_result do_evaluate( const operation_type& o );
   void_result do_apply( const operation_type& o ) ;

   const account_object* acnt = nullptr;
};

class account_update_proxy_evaluator : public evaluator<account_update_proxy_evaluator>
{
   public:
      typedef account_update_proxy_operation operation_type;

      void_result do_evaluate( const account_update_proxy_operation& o );
      void_result do_apply( const account_update_proxy_operation& o );

      const account_statistics_object* account_stats = nullptr;
      const voter_object* voter_obj = nullptr;
      const voter_object* invalid_voter_obj = nullptr;
      const voter_object* current_proxy_voter_obj = nullptr;
      const voter_object* invalid_current_proxy_voter_obj = nullptr;
      const voter_object* proxy_voter_obj = nullptr;
};

class account_enable_allowed_assets_evaluator : public evaluator<account_enable_allowed_assets_evaluator>
{
public:
   typedef account_enable_allowed_assets_operation operation_type;

   void_result do_evaluate( const operation_type& o );
   void_result do_apply( const operation_type& o ) ;

   const account_object* acnt = nullptr;
};

class account_update_allowed_assets_evaluator : public evaluator<account_update_allowed_assets_evaluator>
{
public:
   typedef account_update_allowed_assets_operation operation_type;

   void_result do_evaluate( const operation_type& o );
   void_result do_apply( const operation_type& o ) ;

   const account_object* acnt = nullptr;
};

class account_whitelist_evaluator : public evaluator<account_whitelist_evaluator>
{
public:
   typedef account_whitelist_operation operation_type;

   void_result do_evaluate( const account_whitelist_operation& o);
   void_result do_apply( const account_whitelist_operation& o);

   const account_object* listed_account = nullptr;
};

} } // graphene::chain
