/*
 * Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
 */
#pragma once
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace graphene { namespace chain {
   class database;

   enum advertising_state
   {
      advertising_undetermined = 0,
      advertising_accepted     = 1,
      advertising_refused      = 2,
      advertising_ransom       = 3
   };

   class advertising_order_object : public graphene::db::abstract_object < advertising_order_object >
   {
   public:
      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id = impl_advertising_order_object_type;

      advertising_order_oid_type  advertising_order_oid;
      account_uid_type            platform;
      advertising_aid_type        advertising_aid;

      account_uid_type            user;
      share_type                  released_balance;
      time_point_sec              start_time;
      time_point_sec              end_time;
      time_point_sec              buy_request_time;
      advertising_state           status;
      time_point_sec              handle_time;
                                  
      optional<memo_data>         memo;
      string                      extra_data;

      time_point_sec get_clear_time()const {
         if (status == advertising_accepted)
            return end_time;
         else if (status == advertising_undetermined)
            return time_point_sec::maximum();
         else
            return handle_time;
      }
   };

   struct by_advertising_order_oid{};
   struct by_clear_time{};
   struct by_advertising_order_state{};
   struct by_advertising_user_id{};

   typedef multi_index_container<
      advertising_order_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
         ordered_unique< tag<by_advertising_order_oid>,
                             composite_key<advertising_order_object,
                             member< advertising_order_object, account_uid_type,           &advertising_order_object::platform >,
                             member< advertising_order_object, advertising_aid_type,       &advertising_order_object::advertising_aid >,
                             member< advertising_order_object, advertising_order_oid_type, &advertising_order_object::advertising_order_oid >> >,
         ordered_non_unique< tag<by_advertising_order_state>,
                             composite_key<advertising_order_object,
                             member< advertising_order_object, advertising_state,          &advertising_order_object::status >,
                             member< advertising_order_object, account_uid_type,           &advertising_order_object::platform >,
                             member< advertising_order_object, advertising_aid_type,       &advertising_order_object::advertising_aid >> >,
         
         ordered_non_unique< tag<by_clear_time>, const_mem_fun<advertising_order_object, time_point_sec, &advertising_order_object::get_clear_time  >>,
         ordered_unique< tag<by_advertising_user_id>,
                             composite_key<advertising_order_object,
                             member< advertising_order_object, account_uid_type,    &advertising_order_object::user>,
                             member< object,                   object_id_type,      &object::id >> >
        >
   > advertising_order_multi_index_type;

   /**
   * @ingroup object_index
   */
   typedef generic_index<advertising_order_object, advertising_order_multi_index_type> advertising_order_index;

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

      advertising_aid_type       advertising_aid;
      account_uid_type           platform;
      bool                       on_sell;
      uint32_t                   unit_time;
      share_type                 unit_price;
      string                     description;
      uint64_t                   last_order_sequence = 0;
                                 
      time_point_sec             publish_time;
      time_point_sec             last_update_time;
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
      ordered_unique< tag<by_advertising_platform>,
         composite_key<
            advertising_object,
            member< advertising_object, account_uid_type, &advertising_object::platform>,
            member< advertising_object, advertising_aid_type, &advertising_object::advertising_aid >
            >>,
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

FC_REFLECT_ENUM( graphene::chain::advertising_state,
                   (advertising_undetermined)
                   (advertising_accepted)
                   (advertising_refused)
                   (advertising_ransom))

FC_REFLECT_DERIVED( graphene::chain::advertising_order_object,
                   (graphene::db::object), (advertising_order_oid)(platform)
                   (advertising_aid)(user)(released_balance)(start_time)(end_time)
                   (buy_request_time)(status)(handle_time)(memo)(extra_data)
                   )

FC_REFLECT_DERIVED( graphene::chain::advertising_object,
                   (graphene::db::object), (advertising_aid)
                   (platform)(on_sell)(unit_time)(unit_price)(description)(last_order_sequence)
                   (publish_time)(last_update_time))
