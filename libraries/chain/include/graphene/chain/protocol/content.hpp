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
    * @brief 在网络上创建一个平台，并以所有人支付其费用
    * @ingroup operations
    *
    * 任何人可以用此操作创建一个平台对象
    */
   struct platform_create_operation : public base_operation
   {
      struct fee_parameters_type
      {
         uint64_t fee              = 1000 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee     = 1000 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint16_t min_rf_percent   = 10000;
         uint32_t price_per_kbyte  = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
         extensions_type   extensions;
      };

      // 手续费
      fee_type          fee;
      /// 拥有平台的帐户。 此帐户支付此操作的费用。
      account_uid_type  account;
      // 抵押金额
      asset             pledge;
      //名称
      string            name;
      //平台主域名
      string            url;
      //其他信息（ json 格式字符串，API接口，其他URL，平台介绍等）
      string            extra_data = "{}";
      extensions_type   extensions;

      account_uid_type  fee_payer_uid()const { return account; }
      void              validate()const;
      share_type        calculate_fee(const fee_parameters_type& k)const;
      void get_required_active_uid_authorities( flat_set<account_uid_type>& a )const
      {
         // 需要余额权限
         a.insert( account );
      }
   };

   /**
    * @brief 更新平台相关信息
    * @ingroup operations
    */
   struct platform_update_operation : public base_operation
   {
      struct fee_parameters_type
      {
         uint64_t fee              = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee     = 0;
         uint16_t min_rf_percent   = 0;
         uint32_t price_per_kbyte  = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
         extensions_type   extensions;
      };

      fee_type          fee;
      /// 平台所有者账号
      account_uid_type  account;
      /// 新的抵押金额
      optional<asset>   new_pledge;
      // 新的名称
      optional<string>  new_name;
      /// 新的域名
      optional<string>  new_url;
      // 新的其他信息
      optional<string>  new_extra_data;

      extensions_type   extensions;

      account_uid_type  fee_payer_uid()const { return account; }
      void              validate()const;
      share_type      calculate_fee(const fee_parameters_type& k)const;
      void get_required_active_uid_authorities( flat_set<account_uid_type>& a )const
      {
         // 需要余额权限
         a.insert( account );
      }
   };

   /**
    * @brief 更改或刷新平台投票状态。
    * @ingroup operations
    */
   struct platform_vote_update_operation : public base_operation
   {
      struct fee_parameters_type
      {
         uint64_t basic_fee             = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t price_per_platform    = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee          = 0;
         uint16_t min_rf_percent        = 0;
         extensions_type   extensions;
      };
      
      // 手续费
      fee_type                   fee;
      /// 投票人,以此账号支付投票操作费用
      account_uid_type           voter;
      //新增投票平台清单
      flat_set<account_uid_type> platform_to_add;
      //移除投票平台清单
      flat_set<account_uid_type> platform_to_remove;

      extensions_type   extensions;

      account_uid_type  fee_payer_uid()const { return voter; }
      void              validate()const;
      share_type        calculate_fee(const fee_parameters_type& k)const;
      void get_required_active_uid_authorities( flat_set<account_uid_type>& a )const
      {
         // 需要余额权限
         a.insert( voter );
      }
   };

   struct post_options
   {
      extensions_type        extensions;

      void validate()const;
   };

   /**
    * @ingroup operations
    *
    * @brief Post an article or a reply
    *
    *  Fees are paid by the "poster" account
    *
    *  @return n/a
    */
   struct post_operation : public base_operation
   {
      struct fee_parameters_type {
         uint64_t fee              = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint32_t price_per_kbyte  = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee     = 0;
         uint16_t min_rf_percent   = 0;
         extensions_type   extensions;
      };

      fee_type                     fee;

      account_uid_type             platform = 0;
      account_uid_type             poster = 0;
      optional<account_uid_type>   origin_poster;
      optional<post_pid_type>      origin_post_pid;
      optional<account_uid_type>   origin_platform;

      post_options                 options;

      string                       hash_value;
      string                       extra_data = "{}"; ///< category, tags and etc
      string                       title;
      string                       body;

      extensions_type              extensions;

      account_uid_type fee_payer_uid()const { return poster; }
      void             validate()const;
      share_type       calculate_fee(const fee_parameters_type& k)const;
      void get_required_secondary_uid_authorities( flat_set<account_uid_type>& a )const
      {
         a.insert( poster );    // 要求作者零钱权限
         a.insert( platform );  // 要求平台零钱权限
      }
   };

   /**
    * @ingroup operations
    *
    * @brief update an article
    *
    *  Fees are paid by the "poster" account
    *
    *  @return n/a
    */
   struct post_update_operation : public base_operation
   {
      struct fee_parameters_type {
         uint64_t fee              = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint32_t price_per_kbyte  = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee     = 0;
         uint16_t min_rf_percent   = 0;
         extensions_type   extensions;
      };

      fee_type                     fee;

      account_uid_type             platform;
      account_uid_type             poster;
      post_pid_type                post_pid;

      post_options                 options;

      optional< string >           hash_value;
      optional< string >           extra_data = "{}"; ///< category, tags and etc
      optional< string >           title;
      optional< string >           body;

      extensions_type              extensions;

      account_uid_type fee_payer_uid()const { return poster; }
      void            validate()const;
      share_type      calculate_fee(const fee_parameters_type& k)const;
      void get_required_secondary_uid_authorities( flat_set<account_uid_type>& a )const
      {
         a.insert( poster );    // 要求作者零钱权限
         a.insert( platform );  // 要求平台零钱权限
      }
   };

}} // graphene::chain

FC_REFLECT( graphene::chain::platform_create_operation::fee_parameters_type, (fee)(min_real_fee)(min_rf_percent)(price_per_kbyte)(extensions) )
FC_REFLECT(graphene::chain::platform_create_operation, (fee)(account)(pledge)(name)(url)(extra_data)(extensions) )

FC_REFLECT( graphene::chain::platform_update_operation::fee_parameters_type, (fee)(min_real_fee)(min_rf_percent)(price_per_kbyte)(extensions) )
FC_REFLECT(graphene::chain::platform_update_operation, (fee)(account)(new_pledge)(new_name)(new_url)(new_extra_data)(extensions) )

FC_REFLECT( graphene::chain::platform_vote_update_operation::fee_parameters_type, (basic_fee)(price_per_platform)(min_real_fee)(min_rf_percent)(extensions) )
FC_REFLECT(graphene::chain::platform_vote_update_operation, (fee)(voter)(platform_to_add)(platform_to_remove)(extensions) )

FC_REFLECT(graphene::chain::post_options, (extensions) )

FC_REFLECT( graphene::chain::post_operation::fee_parameters_type, (fee)(price_per_kbyte)(min_real_fee)(min_rf_percent)(extensions) )
FC_REFLECT( graphene::chain::post_operation,
            (fee)
            (platform)(poster)(origin_poster)(origin_post_pid)(origin_platform)
            (options)
            (hash_value)(extra_data)(title)(body)
            (extensions) )

FC_REFLECT( graphene::chain::post_update_operation::fee_parameters_type, (fee)(price_per_kbyte)(min_real_fee)(min_rf_percent)(extensions) )
FC_REFLECT( graphene::chain::post_update_operation,
            (fee)
            (platform)(poster)(post_pid)
            (options)
            (hash_value)(extra_data)(title)(body)
            (extensions) )

