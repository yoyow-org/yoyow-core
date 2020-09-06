/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once
#include <graphene/chain/protocol/types.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>

namespace graphene { namespace chain {

class witness_schedule_object;

class witness_schedule_object : public graphene::db::abstract_object<witness_schedule_object>
{
   public:
      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id = impl_witness_schedule_object_type;

      uint32_t                   next_schedule_block_num = 0;
      fc::uint128_t              current_by_pledge_time=0;
      fc::uint128_t              current_by_vote_time=0;
      vector< account_uid_type > current_shuffled_witnesses;
};

} }

FC_REFLECT_DERIVED(
   graphene::chain::witness_schedule_object,
   (graphene::db::object),
   (next_schedule_block_num)
   (current_by_pledge_time)
   (current_by_vote_time)
   (current_shuffled_witnesses)
)
