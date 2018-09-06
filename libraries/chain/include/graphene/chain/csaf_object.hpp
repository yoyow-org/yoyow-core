/*
 * Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
 */
#pragma once
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace graphene { namespace chain {
   class database;

   /**
    * @brief This class represents a coin-seconds-as-fee leasing on the object graph
    * @ingroup object
    * @ingroup protocol
    *
    */
   class csaf_lease_object : public graphene::db::abstract_object<csaf_lease_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_csaf_lease_object_type;

         account_uid_type             from = 0;
         account_uid_type             to = 0;
         share_type                   amount;
         time_point_sec               expiration;

   };


   struct by_from_to{};
   struct by_expiration;
   struct by_to_from{};

   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      csaf_lease_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
         ordered_unique< tag<by_from_to>,
            composite_key<
               csaf_lease_object,
               member< csaf_lease_object, account_uid_type,  &csaf_lease_object::from>,
               member< csaf_lease_object, account_uid_type,  &csaf_lease_object::to>
            >
         >,
         ordered_unique< tag<by_expiration>,
            composite_key<
               csaf_lease_object,
               member< csaf_lease_object, time_point_sec,    &csaf_lease_object::expiration>,
               member< object, object_id_type, &object::id >
            >
         >,
         // TODO move non-consensus indexes to plugin
         ordered_unique< tag<by_to_from>,
            composite_key<
               csaf_lease_object,
               member< csaf_lease_object, account_uid_type,  &csaf_lease_object::to>,
               member< csaf_lease_object, account_uid_type,  &csaf_lease_object::from>
            >
         >
      >
   > csaf_lease_object_multi_index_type;

   /**
    * @ingroup object_index
    */
   typedef generic_index<csaf_lease_object, csaf_lease_object_multi_index_type> csaf_lease_index;

}}

FC_REFLECT_DERIVED( graphene::chain::csaf_lease_object,
                    (graphene::db::object),
                    (from)(to)(amount)(expiration)
                  )

