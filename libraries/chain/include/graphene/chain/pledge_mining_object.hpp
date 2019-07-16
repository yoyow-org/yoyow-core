/*
* Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
*/
#pragma once
#include <graphene/chain/protocol/asset.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>

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
         share_type           pledge;

         uint32_t             last_bonus_block_num = 0;
         ///coins that are requested to be released but not yet unlocked.
         share_type           releasing_mining_pledge;
         ///block number that releasing pledges to witness will be finally unlocked.
         uint32_t             mining_pledge_release_block_number = -1;
      
         //return the delta pledge need to subtract from account balance
         share_type update_pledge(share_type new_pledge,uint32_t new_relase_num){
            
            auto delta=new_pledge-(pledge+releasing_mining_pledge);
            auto delta_releasing=pledge-new_pledge;
            pledge=new_pledge;
            if(delta>=0){
               releasing_mining_pledge=0;
               mining_pledge_release_block_number=-1;
               return delta;
            }else{
               releasing_mining_pledge+=delta_releasing;
               if(delta_releasing>0)//releasing adding
                  mining_pledge_release_block_number=new_relase_num;// new releasing will extension the old releasing release time
               return 0;
            }
         }
         share_type total_unrelease_pledge()const {return pledge+releasing_mining_pledge;}
   private:
   };

   struct by_pledge_witness;
   struct by_pledge_mining_release;

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
         ordered_non_unique< tag<by_pledge_mining_release>,
            composite_key<
            pledge_mining_object,
            member<pledge_mining_object, uint32_t, &pledge_mining_object::mining_pledge_release_block_number>
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
                    (pledge)
                    (last_bonus_block_num)
                    (releasing_mining_pledge)
                    (mining_pledge_release_block_number)
                  )
