/*
 * Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
 */
#pragma once
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace graphene { namespace chain {
   class database;


   /**
   * @brief This class custom vote
   * @ingroup object
   * @ingroup protocol
   */
   class custom_vote_object : public graphene::db::abstract_object<custom_vote_object>
   {
   public:
      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id = impl_advertising_object_type;

      account_uid_type           account;

      string                     title;
      string                     description;
      time_point_sec             vote_expired_time;

      asset_aid_type             vote_asset_id;
      share_type                 required_asset_amount;
      uint8_t                    minimum_selected_items;
      uint8_t                    maximum_selected_items;

      std::vector<string>        options;
      std::vector<uint64_t>      vote_result;
      std::map<account_uid_type, vector<uint8_t>> votes;
   };
}}

FC_REFLECT_DERIVED( graphene::chain::custom_vote_object,
                   (graphene::db::object),
                   (account)(title)(description)(vote_expired_time)(vote_asset_id)(required_asset_amount)
                   (minimum_selected_items)(maximum_selected_items)(options)(vote_result)(votes))