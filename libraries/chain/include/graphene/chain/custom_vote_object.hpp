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
   * @brief This class cast custom vote
   * @ingroup object
   * @ingroup protocol
   */
   class cast_custom_vote_object : public graphene::db::abstract_object<cast_custom_vote_object>
   {
   public:
      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id = impl_cast_custom_vote_object_type;

      account_uid_type           voter;
      account_uid_type           custom_vote_creater;
      custom_vote_vid_type       custom_vote_vid;
      std::set<uint8_t>          vote_result;
   };
     
   struct by_custom_vote_vid{};
   struct by_custom_voter{};
   struct by_cast_custom_vote_id{};

   /**
   * @ingroup object_index
   */
   typedef multi_index_container<
      cast_custom_vote_object,
      indexed_by<
      ordered_unique< tag<by_id>, member< object, object_id_type,  &object::id > >,
      ordered_non_unique< tag<by_custom_vote_vid>,
      composite_key<cast_custom_vote_object,
            member< cast_custom_vote_object, account_uid_type,     &cast_custom_vote_object::custom_vote_creater>,
            member< cast_custom_vote_object, custom_vote_vid_type, &cast_custom_vote_object::custom_vote_vid >,
            member< object, object_id_type,  &object::id > 
      >> ,
      ordered_unique< tag<by_custom_voter>,
         composite_key<
            cast_custom_vote_object,
            member< cast_custom_vote_object, account_uid_type,     &cast_custom_vote_object::voter>,
            member< cast_custom_vote_object, account_uid_type,     &cast_custom_vote_object::custom_vote_creater>,
            member< cast_custom_vote_object, custom_vote_vid_type, &cast_custom_vote_object::custom_vote_vid >>
         >,
      ordered_unique< tag<by_cast_custom_vote_id>,
         composite_key<
            cast_custom_vote_object,
            member< cast_custom_vote_object, account_uid_type,     &cast_custom_vote_object::voter>,
            member< object, object_id_type,  &object::id >>
         >
      >
   > cast_custom_vote_multi_index_type;

   /**
   * @ingroup object_index
   */
   typedef generic_index<cast_custom_vote_object, cast_custom_vote_multi_index_type> cast_custom_vote_index;


   /**
   * @brief This class custom vote
   * @ingroup object
   * @ingroup protocol
   */
   class custom_vote_object : public graphene::db::abstract_object<custom_vote_object>
   {
   public:
      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id = impl_custom_vote_object_type;

      account_uid_type           custom_vote_creater;
      custom_vote_vid_type       vote_vid;

      string                     title;
      string                     description;
      time_point_sec             vote_expired_time;

      asset_aid_type             vote_asset_id;
      share_type                 required_asset_amount;
      uint8_t                    minimum_selected_items;
      uint8_t                    maximum_selected_items;

      std::vector<string>        options;
      std::vector<uint64_t>      vote_result;
   };

   struct by_creater{};
   struct by_expired_time{};

   /**
   * @ingroup object_index
   */
   typedef multi_index_container<
      custom_vote_object,
      indexed_by<
      ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
      ordered_unique< tag<by_creater>,
         composite_key<
            custom_vote_object,
            member< custom_vote_object, account_uid_type,     &custom_vote_object::custom_vote_creater>,
            member< custom_vote_object, custom_vote_vid_type, &custom_vote_object::vote_vid > > >,
      ordered_non_unique< tag<by_expired_time>,
            member< custom_vote_object, time_point_sec,       &custom_vote_object::vote_expired_time> >
      >
   > custom_vote_multi_index_type;

   /**
   * @ingroup object_index
   */
   typedef generic_index<custom_vote_object, custom_vote_multi_index_type> custom_vote_index;
}}

FC_REFLECT_DERIVED( graphene::chain::custom_vote_object,
                   (graphene::db::object),
                   (custom_vote_creater)(vote_vid)(title)(description)(vote_expired_time)(vote_asset_id)(required_asset_amount)
                   (minimum_selected_items)(maximum_selected_items)(options)(vote_result))

FC_REFLECT_DERIVED( graphene::chain::cast_custom_vote_object,
                   (graphene::db::object),
                   (voter)(custom_vote_creater)(custom_vote_vid)(vote_result))