/*
 * Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
 */
#pragma once
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/memo.hpp>

namespace graphene { namespace chain {

   struct advertising_create_operation : public base_operation
   {
      struct fee_parameters_type {
         uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION / 10;
         uint32_t price_per_kbyte = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee = 0;
         uint16_t min_rf_percent = 0;
         extensions_type   extensions;
      };

      fee_type                     fee;

      // The advertising aid
      advertising_aid_type         advertising_aid;
      // The platform account that advertising belong to
      account_uid_type             platform;
      // Unit time for advertising sale
      uint32_t                     unit_time;
      // Selling price for per unit time of advertising 
      share_type                   unit_price;
      string                       description;

      extensions_type              extensions;

      account_uid_type fee_payer_uid()const { return platform; }
      void             validate()const;
      share_type       calculate_fee(const fee_parameters_type& k)const;
      void get_required_active_uid_authorities(flat_set<account_uid_type>& a, bool enabled_hardfork)const
      {
         a.insert(platform);    // Requires platform to change the permissions
      }
   };

   struct advertising_update_operation : public base_operation
   {
      struct fee_parameters_type {
         uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION / 10;
         uint32_t price_per_kbyte = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee = 0;
         uint16_t min_rf_percent = 0;
         extensions_type   extensions;
      };

      fee_type                     fee;

      // The platform account that advertising belong to
      account_uid_type             platform;
      // The advertising aid
      advertising_aid_type         advertising_aid;

      optional<string>             description;
      // Selling price for per unit time of advertising 
      optional<share_type>         unit_price;
      // Unit time for advertising sale
      optional<uint32_t>           unit_time;
      // Advertising status, is selling or not selling
      optional<bool>               on_sell;

      extensions_type              extensions;

      account_uid_type fee_payer_uid()const { return platform; }
      void             validate()const;
      share_type       calculate_fee(const fee_parameters_type& k)const;
      void get_required_active_uid_authorities(flat_set<account_uid_type>& a, bool enabled_hardfork)const
      {
         a.insert(platform);    // Requires platform to change the permissions
      }
   };

   struct advertising_buy_operation : public base_operation
   {
      struct fee_parameters_type {
         uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION / 10;
         uint32_t price_per_kbyte = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee = 0;
         uint16_t min_rf_percent = 0;
         extensions_type   extensions;
      };

      fee_type                     fee;
      // The advertising order id
      advertising_order_oid_type   advertising_order_oid;

      // Account that buy advertising
      account_uid_type             from_account;
      // The platform account that advertising belong to
      account_uid_type             platform;
      // The advertising aid
      advertising_aid_type         advertising_aid;
      // Advertising order start time
      time_point_sec               start_time;
      // Number of advertising unit
      uint32_t                     buy_number;

      string                       extra_data;
      optional<memo_data>          memo;

      extensions_type              extensions;

      account_uid_type fee_payer_uid()const { return from_account; }
      void             validate()const;
      share_type       calculate_fee(const fee_parameters_type& k)const;
      void get_required_active_uid_authorities(flat_set<account_uid_type>& a, bool enabled_hardfork)const
      {
         a.insert(from_account);    // Requires platform to change the permissions
      }
   };

   struct advertising_confirm_operation : public base_operation
   {
      struct fee_parameters_type {
         uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION / 10;
         uint32_t price_per_kbyte = 0;
         uint64_t min_real_fee = 0;
         uint16_t min_rf_percent = 0;
         extensions_type   extensions;
      };

      fee_type                     fee;

      // The platform account that confirm advertising order
      account_uid_type             platform;
      // The advertising aid
      advertising_aid_type         advertising_aid;
      // The advertising order id
      advertising_order_oid_type   advertising_order_oid;
      // Advertising order confirm status, accepted or refused
      bool                         isconfirm = false;

      extensions_type              extensions;

      account_uid_type fee_payer_uid()const { return platform; }
      void             validate()const;
      share_type       calculate_fee(const fee_parameters_type& k)const;
      void get_required_active_uid_authorities(flat_set<account_uid_type>& a, bool enabled_hardfork)const
      {
         a.insert(platform);    // Requires platform to change the permissions
      }
   };

   struct advertising_ransom_operation : public base_operation
   {
      struct fee_parameters_type {
         uint64_t fee = 0;
         uint32_t price_per_kbyte = 0;
         uint64_t min_real_fee = 0;
         uint16_t min_rf_percent = 0;
         extensions_type   extensions;
      };

      fee_type                     fee;

      // Account that ransom advertising order
      account_uid_type             from_account;
      // The platform account that advertising belong to
      account_uid_type             platform;
      // The advertising aid
      advertising_aid_type         advertising_aid;
      // The advertising order id
      advertising_order_oid_type   advertising_order_oid;

      extensions_type              extensions;

      account_uid_type fee_payer_uid()const { return from_account; }
      void             validate()const;
      share_type       calculate_fee(const fee_parameters_type& k)const;
      void get_required_active_uid_authorities(flat_set<account_uid_type>& a, bool enabled_hardfork)const
      {
         a.insert(from_account);    // Requires from_account to ransom 
      }
   };
}} // graphene::chain

FC_REFLECT(graphene::chain::advertising_create_operation::fee_parameters_type, (fee)(price_per_kbyte)(min_real_fee)(min_rf_percent)(extensions))
FC_REFLECT(graphene::chain::advertising_create_operation, (fee)(advertising_aid)(platform)(unit_time)(unit_price)(description)(extensions))

FC_REFLECT(graphene::chain::advertising_update_operation::fee_parameters_type, (fee)(price_per_kbyte)(min_real_fee)(min_rf_percent)(extensions))
FC_REFLECT(graphene::chain::advertising_update_operation, (fee)(platform)(advertising_aid)(description)(unit_price)(unit_time)(on_sell)(extensions))

FC_REFLECT(graphene::chain::advertising_buy_operation::fee_parameters_type, (fee)(price_per_kbyte)(min_real_fee)(min_rf_percent)(extensions))
FC_REFLECT(graphene::chain::advertising_buy_operation, (fee)(advertising_order_oid)(from_account)(platform)(advertising_aid)(start_time)(buy_number)(extra_data)(memo)(extensions))

FC_REFLECT(graphene::chain::advertising_confirm_operation::fee_parameters_type, (fee)(price_per_kbyte)(min_real_fee)(min_rf_percent)(extensions))
FC_REFLECT(graphene::chain::advertising_confirm_operation, (fee)(platform)(advertising_aid)(advertising_order_oid)(isconfirm)(extensions))

FC_REFLECT(graphene::chain::advertising_ransom_operation::fee_parameters_type, (fee)(price_per_kbyte)(min_real_fee)(min_rf_percent)(extensions))
FC_REFLECT(graphene::chain::advertising_ransom_operation, (fee)(from_account)(platform)(advertising_aid)(advertising_order_oid)(extensions))