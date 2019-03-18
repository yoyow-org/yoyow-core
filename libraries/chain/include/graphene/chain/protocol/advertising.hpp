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
           uint64_t fee = 50 * GRAPHENE_BLOCKCHAIN_PRECISION;
           uint32_t price_per_kbyte = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
           uint64_t min_real_fee = 0;
           uint16_t min_rf_percent = 0;
           extensions_type   extensions;
       };

       fee_type                     fee;

       advertising_aid_type         advertising_aid;
       account_uid_type             platform;
       uint32_t                     unit_time;
       share_type                   unit_price;
       string                       description;

       extensions_type              extensions;

       account_uid_type fee_payer_uid()const { return platform; }
       void             validate()const;
       share_type       calculate_fee(const fee_parameters_type& k)const;
       void get_required_active_uid_authorities(flat_set<account_uid_type>& a)const
       {
           a.insert(platform);    // Requires platform to change the permissions
       }
   };

   struct advertising_update_operation : public base_operation
   {
       struct fee_parameters_type {
           uint64_t fee = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
           uint32_t price_per_kbyte = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
           uint64_t min_real_fee = 0;
           uint16_t min_rf_percent = 0;
           extensions_type   extensions;
       };

       fee_type                     fee;

       account_uid_type             platform;
       advertising_aid_type         advertising_aid;

       optional<string>             description;
       optional<share_type>         unit_price;
       optional<uint32_t>           unit_time;
       optional<bool>               on_sell;

       extensions_type              extensions;

       account_uid_type fee_payer_uid()const { return platform; }
       void             validate()const;
       share_type       calculate_fee(const fee_parameters_type& k)const;
       void get_required_active_uid_authorities(flat_set<account_uid_type>& a)const
       {
           a.insert(platform);    // Requires platform to change the permissions
       }
   };

   struct advertising_buy_operation : public base_operation
   {
       struct fee_parameters_type {
           uint64_t fee = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
           uint32_t price_per_kbyte = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
           uint64_t min_real_fee = 0;
           uint16_t min_rf_percent = 0;
           extensions_type   extensions;
       };

       fee_type                     fee;
       advertising_order_oid_type   advertising_order_oid;
       account_uid_type             from_account;
       account_uid_type             platform;
       advertising_aid_type         advertising_aid;
       time_point_sec               start_time;
       uint32_t                     buy_number;

       string                       extra_data;
       optional<memo_data>          memo;
       
       extensions_type              extensions;

       account_uid_type fee_payer_uid()const { return from_account; }
       void             validate()const;
       share_type       calculate_fee(const fee_parameters_type& k)const;
       void get_required_active_uid_authorities(flat_set<account_uid_type>& a)const
       {
          a.insert(from_account);    // Requires platform to change the permissions
       }
   };

   struct advertising_confirm_operation : public base_operation
   {
       struct fee_parameters_type {
           uint64_t fee = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
           uint32_t price_per_kbyte = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
           uint64_t min_real_fee = 0;
           uint16_t min_rf_percent = 0;
           extensions_type   extensions;
       };

       fee_type                     fee;

       account_uid_type             platform;
       advertising_aid_type         advertising_aid;
       advertising_order_oid_type   advertising_order_oid;
       bool                         iscomfirm;

       extensions_type              extensions;

       account_uid_type fee_payer_uid()const { return platform; }
       void             validate()const;
       share_type       calculate_fee(const fee_parameters_type& k)const;
       void get_required_active_uid_authorities(flat_set<account_uid_type>& a)const
       {
           a.insert(platform);    // Requires platform to change the permissions
       }
   };

   struct advertising_ransom_operation : public base_operation
   {
       struct fee_parameters_type {
           uint64_t fee = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
           uint32_t price_per_kbyte = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
           uint64_t min_real_fee = 0;
           uint16_t min_rf_percent = 0;
           extensions_type   extensions;
       };

       fee_type                     fee;

       account_uid_type             from_account;
       account_uid_type             platform;
       advertising_aid_type         advertising_aid;
       advertising_order_oid_type   advertising_order_oid;

       extensions_type              extensions;

       account_uid_type fee_payer_uid()const { return from_account; }
       void             validate()const;
       share_type       calculate_fee(const fee_parameters_type& k)const;
       void get_required_active_uid_authorities(flat_set<account_uid_type>& a)const
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
FC_REFLECT(graphene::chain::advertising_confirm_operation, (fee)(platform)(advertising_aid)(advertising_order_oid)(iscomfirm)(extensions))

FC_REFLECT(graphene::chain::advertising_ransom_operation::fee_parameters_type, (fee)(price_per_kbyte)(min_real_fee)(min_rf_percent)(extensions))
FC_REFLECT(graphene::chain::advertising_ransom_operation, (fee)(from_account)(platform)(advertising_aid)(advertising_order_oid)(extensions))