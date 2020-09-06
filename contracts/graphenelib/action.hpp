#pragma once
#include <graphenelib/action.h>
#include <graphenelib/datastream.hpp>
#include <graphenelib/serialize.hpp>
#include <graphenelib/global.h>
#include <boost/preprocessor/variadic/size.hpp>
#include <boost/preprocessor/variadic/to_tuple.hpp>
#include <boost/preprocessor/tuple/enum.hpp>
#include <boost/preprocessor/facilities/overload.hpp>
#include "contract_asset.hpp"

namespace graphene {

   template<typename T>
   T unpack_action_data() {
      constexpr size_t max_stack_buffer_size = 512;
      size_t size = action_data_size();
      const bool heap_allocation = max_stack_buffer_size < size;
      char *buffer = (char *) (heap_allocation ? malloc(size) : alloca(size));
      read_action_data(buffer, size);
      auto res = unpack<T>( buffer, size );
      // free allocated memory
      if (heap_allocation) {
          free(buffer);
      }
      return res;
   }

   /**
    * This is the packed representation of an action along with
    * meta-data about the authorization levels.
    */
   struct action {
      uint64_t                   sender;
      uint64_t                   contract_id;
      contract_asset             amount;
      action_name                method;
      bytes                      method_args_binary;

      action() = default;

      /**
       *  @tparam T - the type of the action data
       *  @param contract_id - id of the contract account
       *  @param method - name of the action
       *  @param args - will be serialized via pack into data
       */
      template <typename T>
      action(uint64_t contract_id, action_name method, T &&args)
          : contract_id(contract_id)
          , method(method)
          , method_args_binary(pack(std::forward<T>(args)))
      {
      }

      /**
       *  @tparam T - the type of the action data
       *  @param contract_id - id of the contract account
       *  @param method - name of the action
       *  @param args - will be serialized via pack into data
       *  @param sender - the contract caller
       *  @param amt - the amount of asset to transfer to target contract
       */
      template <typename T>
      action(uint64_t contract_id, action_name method, T &&args, uint64_t sender, const contract_asset &amt = {0, 0})
          : sender(sender)
          , contract_id(contract_id)
          , amount(amt)
          , method(method)
          , method_args_binary(pack(std::forward<T>(args)))
      {
      }

      /**
       *  @tparam T - the type of the action data
       *  @param contract - name of the contract account
       *  @param method - name of the action
       *  @param args - will be serialized via pack into data
       *  @param sender - the contract caller
       *  @param amt - the amount of asset to transfer to target contract
       */
      template <typename T>
      action(std::string contract, action_name method, T &&args, uint64_t sender, const contract_asset &amt = {0, 0})
          : sender(sender)
          , amount(amt)
          , method(method)
          , method_args_binary(pack(std::forward<T>(args)))
      {
          int64_t acc_id = get_account_id(contract.c_str(), contract.length());
          graphene_assert(-1 != acc_id, "account not found");
          contract_id = acc_id;
      }

      GRAPHENE_SERIALIZE(action, (sender)(contract_id)(amount)(method)(method_args_binary))

      void send() const
      {
          auto serialize = pack(*this);
          ::send_inline(serialize.data(), serialize.size());
      }
   };

} // namespace graphene

#define INLINE_ACTION_SENDER2( CONTRACT_CLASS, NAME )\
INLINE_ACTION_SENDER3( CONTRACT_CLASS, NAME, ::graphene::string_to_name(#NAME) )

#define INLINE_ACTION_SENDER(...) BOOST_PP_OVERLOAD(INLINE_ACTION_SENDER,__VA_ARGS__)(__VA_ARGS__)

#define SEND_INLINE_ACTION( CONTRACT, NAME, ... )\
INLINE_ACTION_SENDER(std::decay_t<decltype(CONTRACT)>, NAME)( (CONTRACT).get_self(),\
BOOST_PP_TUPLE_ENUM(BOOST_PP_VARIADIC_SIZE(__VA_ARGS__), BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__)) );
