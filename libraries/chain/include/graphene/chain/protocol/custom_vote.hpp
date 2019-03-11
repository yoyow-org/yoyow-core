/*
* Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
*/
#pragma once
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/memo.hpp>

namespace graphene { namespace chain {

struct custom_vote_create_operation : public base_operation
{
   struct fee_parameters_type {
      uint64_t fee = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
      uint32_t price_per_kbyte = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
      uint64_t min_real_fee = 0;
      uint16_t min_rf_percent = 0;
      extensions_type   extensions;
   };

   fee_type                   fee;

   account_uid_type           create_account;
   string                     title;
   string                     description;
   time_point_sec             vote_expired_time;
   asset_aid_type             vote_asset_id;
   share_type                 required_asset_amount;
   uint8_t                    minimum_selected_items;
   uint8_t                    maximum_selected_items;

   std::vector<string>        options;
   extensions_type            extensions;

   account_uid_type fee_payer_uid()const { return create_account; }
   void             validate()const;
   share_type       calculate_fee(const fee_parameters_type& k)const;
   void get_required_active_uid_authorities(flat_set<account_uid_type>& a)const
   {
      a.insert(create_account);   
   }
};

struct custom_vote_cast_operation : public base_operation
{
   struct fee_parameters_type {
      uint64_t fee = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
      uint32_t price_per_kbyte = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
      uint64_t min_real_fee = 0;
      uint16_t min_rf_percent = 0;
      extensions_type   extensions;
   };

   fee_type                     fee;

   account_uid_type             voter;
   custom_vote_id_type          custom_vote_id;
   vector<uint8_t>              vote_result;

   extensions_type              extensions;

   account_uid_type fee_payer_uid()const { return voter; }
   void             validate()const;
   share_type       calculate_fee(const fee_parameters_type& k)const;
   void get_required_active_uid_authorities(flat_set<account_uid_type>& a)const
   {
      a.insert(voter);  
   }
};
}} // graphene::chain



FC_REFLECT( graphene::chain::custom_vote_create_operation::fee_parameters_type, (fee)(price_per_kbyte)(min_real_fee)(min_rf_percent)(extensions))
FC_REFLECT( graphene::chain::custom_vote_create_operation, (fee)(create_account)(title)(description)(vote_expired_time)(vote_asset_id)
           (required_asset_amount)(minimum_selected_items)(maximum_selected_items)(options)(extensions))

FC_REFLECT( graphene::chain::custom_vote_cast_operation::fee_parameters_type, (fee)(price_per_kbyte)(min_real_fee)(min_rf_percent)(extensions))
FC_REFLECT( graphene::chain::custom_vote_cast_operation, (fee)(voter)(custom_vote_id)(vote_result)(extensions))