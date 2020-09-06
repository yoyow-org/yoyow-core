#pragma once
#include <graphenelib/types.h>

extern "C" {

   /**
    *  Aborts processing of this action and unwinds all pending changes if the test condition is true
    *  @brief Aborts processing of this action and unwinds all pending changes
    *  @param test - 0 to abort, 1 to ignore
    *  @param msg - a null terminated string explaining the reason for failure
    */
   void  graphene_assert( uint32_t test, const char* msg );

   /**
    *  Aborts processing of this action and unwinds all pending changes if the test condition is true
    *  @brief Aborts processing of this action and unwinds all pending changes
    *  @param test - 0 to abort, 1 to ignore
    *  @param msg - a pointer to the start of string explaining the reason for failure
    *  @param msg_len - length of the string
    */
   void  graphene_assert_message( uint32_t test, const char* msg, uint32_t msg_len );

   /**
    *  Aborts processing of this action and unwinds all pending changes if the test condition is true
    *  @brief Aborts processing of this action and unwinds all pending changes
    *  @param test - 0 to abort, 1 to ignore
    *  @param code - the error code

    */
   void  graphene_assert_code( uint32_t test, uint64_t code );

   /**
    * This method will abort execution of wasm without failing the contract. This
    * is used to bypass all cleanup / destructors that would normally be called.
    */
   [[noreturn]] void  graphene_exit( int32_t code );

}
