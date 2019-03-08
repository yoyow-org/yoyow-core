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
   * @brief This class advertising space
   * @ingroup object
   * @ingroup protocol
   */
   class advertising_object : public graphene::db::abstract_object<advertising_object>
   {
   public:
      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id = impl_advertising_object_type;

      struct Advertising_Order
      {
          account_uid_type       user;
          share_type             released_balance;
          time_point_sec         start_time;
          time_point_sec         end_time;
          time_point_sec         buy_request_time;
                                 
          optional<memo_data>    memo;
          string                 extra_data;
      };                         
                                 
      account_uid_type           platform;
      bool                       on_sell;
      uint32_t                   unit_time;
      share_type                 unit_price;
      string                     description;
                                 
      time_point_sec             publish_time;
      time_point_sec             last_update_time;
      uint32_t                   order_sequence = 0;

      map<time_point_sec, Advertising_Order>  effective_orders;
      map<uint32_t, Advertising_Order>        undetermined_orders;
   };

   struct by_advertising_platform{};
   struct by_advertising_state{};
   
   /**
   * @ingroup object_index
   */
   typedef multi_index_container<
      advertising_object,
      indexed_by<
      ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
      ordered_non_unique< tag<by_advertising_platform>,
         member< advertising_object, account_uid_type, &advertising_object::platform> >,
      ordered_non_unique< tag<by_advertising_state>,
         composite_key<
            advertising_object,
            member< advertising_object, account_uid_type, &advertising_object::platform >,
            member< advertising_object, bool, &advertising_object::on_sell >>
       > 
       >
      > advertising_multi_index_type;

   /**
   * @ingroup object_index
   */
   typedef generic_index<advertising_object, advertising_multi_index_type> advertising_index;


}}

FC_REFLECT(graphene::chain::advertising_object::Advertising_Order,
                   (user)(released_balance)(start_time)(end_time)
                   (buy_request_time)(memo)(extra_data)
                   )

FC_REFLECT_DERIVED( graphene::chain::advertising_object,
                   (graphene::db::object), 
                   (platform)(on_sell)(unit_time)(unit_price)(description)
                   (publish_time)(last_update_time)(order_sequence)(effective_orders)(undetermined_orders))
