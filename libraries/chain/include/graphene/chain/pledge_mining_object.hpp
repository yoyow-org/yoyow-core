/*
* Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
*/
#pragma once
#include <graphene/chain/protocol/asset.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <graphene/chain/account_object.hpp>

namespace graphene { namespace chain {

   /**
   * @brief This class represents a account pledge asset to witness on the object graph
   * @ingroup object
   * @ingroup protocol
   */
   class pledge_mining_object : public graphene::db::abstract_object<pledge_mining_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id = impl_pledge_mining_object_type;

         account_uid_type     pledge_account;
         account_uid_type     witness;
         uint32_t             last_bonus_block_num = 0;
         pledge_balance_id_type pledge_id;

   };

   struct by_pledge_witness;
   struct by_pledge_account;

   /**
   * @ingroup object_index
   */
   typedef multi_index_container<
      pledge_mining_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
         ordered_unique< tag<by_pledge_witness>,
            composite_key<
            pledge_mining_object,
            member<pledge_mining_object, account_uid_type, &pledge_mining_object::witness>,
            member<pledge_mining_object, account_uid_type, &pledge_mining_object::pledge_account>
            >
         >,
         ordered_unique< tag<by_pledge_account>,
            composite_key<
            pledge_mining_object,
            member<pledge_mining_object, account_uid_type, &pledge_mining_object::pledge_account>,
            member<pledge_mining_object, account_uid_type, &pledge_mining_object::witness>
            >
         >
      >
   > pledge_mining_multi_index_type;

   /**
   * @ingroup object_index
   */
   typedef generic_index<pledge_mining_object, pledge_mining_multi_index_type> pledge_mining_index;

} } // graphene::chain

FC_REFLECT_DERIVED( graphene::chain::pledge_mining_object, (graphene::db::object),
                    (pledge_account)
                    (witness)
                    (last_bonus_block_num)
                    (pledge_id)
                  )
