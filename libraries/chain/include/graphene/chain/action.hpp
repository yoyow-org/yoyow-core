#pragma once

#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/name.hpp>
#include <graphene/chain/protocol/contract_asset.hpp>

namespace graphene { namespace chain {

   struct action {

      uint64_t                   sender;
      uint64_t                   contract_id;
      contract_asset             amount;
      action_name                method_name;
      bytes                      data;

      action() {}

      action(uint64_t sender, uint64_t contract_id, action_name name, const bytes& data, const contract_asset &amt = contract_asset{0, 0})
            : sender(sender)
            , contract_id(contract_id)
            , amount(amt)
            , method_name(name)
            , data(data)
      {
      }
   };

} } /// namespace graphene::chain

FC_REFLECT(graphene::chain::action, (sender)(contract_id)(amount)(method_name)(data))
