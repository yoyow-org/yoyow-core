/**
 *  @file
 *  @copyright defined in LICENSE.txt
 */
#pragma once
#include <graphenelib/system.h>

extern "C" {
   /**
    * @defgroup actionapi Action API
    * @ingroup contractdev
    * @brief Define API for querying action properties
    *
    */

   /**
    * @defgroup actioncapi Action C API
    * @ingroup actionapi
    * @brief Define API for querying action properties
    *
    */

   /**
    *  Copy up to @ref len bytes of current action data to the specified location
    *  @brief Copy current action data to the specified location
    *  @param msg - a pointer where up to @ref len bytes of the current action data will be copied
    *  @param len - len of the current action data to be copied, 0 to report required size
    *  @return the number of bytes copied to msg, or number of bytes that can be copied if len==0 passed
    */
   uint32_t read_action_data( void* msg, uint32_t len );

   /**
    * Get the length of the current action's data field
    * This method is useful for dynamically sized actions
    * @brief Get the length of current action's data field
    * @return the length of the current action's data field
    */
   uint32_t action_data_size();


   /**
    *  Send an inline action in the context of this action's parent transaction
    * @param serialized_action - serialized action
    * @param size - size of serialized action in bytes
    */
   void send_inline(char *serialized_action, size_t size);

   /**
    *  Get the current receiver of the action
    *  @brief Get the current receiver of the action
    *  @return the account which specifies the current receiver of the action
    */
   uint64_t current_receiver();

   uint64_t get_action_asset_id();

   int64_t get_action_asset_amount();

   ///@ } actioncapi
}
