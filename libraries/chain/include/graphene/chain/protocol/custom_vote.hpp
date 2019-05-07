/*
* Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
*/
#pragma once
#include <graphene/chain/protocol/base.hpp>

namespace graphene { namespace chain {

struct custom_vote_create_operation : public base_operation
{
   struct fee_parameters_type {
      uint64_t fee = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
      uint32_t price_per_kbyte = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
      uint64_t min_real_fee = 0;
      uint16_t min_rf_percent = 0;
      extensions_type   extensions;
   };

   fee_type                   fee;

   account_uid_type           custom_vote_creater;   //the custom vote`s creater account_uid
   custom_vote_vid_type       vote_vid;              //the custom vote`s vid
   string                     title;                 //the custom`s title
   string                     description;           //the custom`s description
   time_point_sec             vote_expired_time;     //the expiration time of this custom vote
   asset_aid_type             vote_asset_id;         //the asset`s id allowed to join in this custom
   share_type                 required_asset_amount; //the min amount of asset that can join in this custom
   uint8_t                    minimum_selected_items;//the least options should be voted
   uint8_t                    maximum_selected_items;//the most options should be voted

   std::vector<string>        options;    //the list of options for this custom vote
   extensions_type            extensions;

   account_uid_type fee_payer_uid()const { return custom_vote_creater; }
   void             validate()const;
   share_type       calculate_fee(const fee_parameters_type& k)const;
   void get_required_active_uid_authorities(flat_set<account_uid_type>& a)const
   {
      a.insert(custom_vote_creater);
   }
};

struct custom_vote_cast_operation : public base_operation
{
   struct fee_parameters_type {
      uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION / 10;
      uint32_t price_per_kbyte = 0;
      uint64_t min_real_fee = 0;
      uint16_t min_rf_percent = 0;
      extensions_type   extensions;
   };

   fee_type                     fee;

   account_uid_type             voter;               //the voter account_uid
   account_uid_type             custom_vote_creater; //the custom vote`s creater account
   custom_vote_vid_type         custom_vote_vid;     //the custom vote`s vid
   std::set<uint8_t>            vote_result;         //the selected options for this vote

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
FC_REFLECT( graphene::chain::custom_vote_create_operation, (fee)(custom_vote_creater)(vote_vid)(title)(description)(vote_expired_time)(vote_asset_id)
           (required_asset_amount)(minimum_selected_items)(maximum_selected_items)(options)(extensions))

FC_REFLECT( graphene::chain::custom_vote_cast_operation::fee_parameters_type, (fee)(price_per_kbyte)(min_real_fee)(min_rf_percent)(extensions))
FC_REFLECT( graphene::chain::custom_vote_cast_operation, (fee)(voter)(custom_vote_creater)(custom_vote_vid)(vote_result)(extensions))