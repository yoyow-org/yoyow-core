#pragma once
#include <graphenelib/print.hpp>
#include <graphenelib/action.hpp>

#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/include/std_tuple.hpp>

#include <boost/mp11/tuple.hpp>
#define N(X) ::graphenelib::string_to_name(#X)
namespace graphene {
   template<typename Contract, typename FirstAction>
   bool dispatch( uint64_t code, uint64_t act ) {
      if( code == FirstAction::get_account() && FirstAction::get_name() == act ) {
         Contract().on( unpack_action_data<FirstAction>() );
         return true;
      }
      return false;
   }

   template<typename Contract, typename FirstAction, typename SecondAction, typename... Actions>
   bool dispatch( uint64_t code, uint64_t act ) {
      if( code == FirstAction::get_account() && FirstAction::get_name() == act ) {
         Contract().on( unpack_action_data<FirstAction>() );
         return true;
      }
      return graphene::dispatch<Contract,SecondAction,Actions...>( code, act );
   }

   template<typename T, typename Q, typename... Args>
   bool execute_action( T* obj, void (Q::*func)(Args...)  ) {
      size_t size = action_data_size();

      //using malloc/free here potentially is not exception-safe, although WASM doesn't support exceptions
      constexpr size_t max_stack_buffer_size = 512;
      void* buffer = nullptr;
      if( size > 0 ) {
         buffer = max_stack_buffer_size < size ? malloc(size) : alloca(size);
         read_action_data( buffer, size );
      }

      auto args = unpack<std::tuple<std::decay_t<Args>...>>( (char*)buffer, size );

      if ( max_stack_buffer_size < size ) {
         free(buffer);
      }

      auto f2 = [&]( auto... a ){
         (obj->*func)( a... );
      };

      boost::mp11::tuple_apply( f2, args );
      return true;
   }

#define GRAPHENE_API_CALL( r, OP, elem ) \
   case graphenelib::string_to_name( BOOST_PP_STRINGIZE(elem) ): \
      graphene::execute_action( &thiscontract, &OP::elem ); \
      break;

#define GRAPHENE_API( TYPE,  MEMBERS ) \
   BOOST_PP_SEQ_FOR_EACH( GRAPHENE_API_CALL, TYPE, MEMBERS )

#define GRAPHENE_ABI( TYPE, MEMBERS ) \
extern "C" { \
   void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
      auto self = receiver; \
      if (code == self) { \
         TYPE thiscontract( self ); \
         switch( action ) { \
            GRAPHENE_API( TYPE, MEMBERS ) \
         } \
      } \
   } \
} \

}
