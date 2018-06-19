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

#include <graphene/chain/protocol/asset.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/db/generic_index.hpp>
#include <graphene/db/object.hpp>

#include <boost/multi_index/composite_key.hpp>

namespace graphene { namespace chain {

using namespace graphene::db;

/**
 *  @brief an offer to sell a amount of a asset at a specified exchange rate by a certain time
 *  @ingroup object
 *  @ingroup protocol
 *  @ingroup market
 *
 *  This limit_order_objects are indexed by @ref expiration and is automatically deleted on the first block after expiration.
 */
class limit_order_object : public abstract_object<limit_order_object>
{
   public:
      static const uint8_t space_id = protocol_ids;
      static const uint8_t type_id  = limit_order_object_type;

      time_point_sec   expiration;
      account_uid_type  seller;
      share_type       for_sale; ///< asset id is sell_price.base.asset_id
      price            sell_price;
      share_type       deferred_fee;

      pair<asset_aid_type,asset_aid_type> get_market()const
      {
         auto tmp = std::make_pair( sell_price.base.asset_id, sell_price.quote.asset_id );
         if( tmp.first > tmp.second ) std::swap( tmp.first, tmp.second );
         return tmp;
      }

      asset amount_for_sale()const   { return asset( for_sale, sell_price.base.asset_id ); }
      asset amount_to_receive()const { return amount_for_sale() * sell_price; }
};

struct by_id;
struct by_price;
struct by_expiration;
struct by_account;
typedef multi_index_container<
   limit_order_object,
   indexed_by<
      ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
      ordered_unique< tag<by_expiration>,
         composite_key< limit_order_object,
            member< limit_order_object, time_point_sec, &limit_order_object::expiration>,
            member< object, object_id_type, &object::id>
         >
      >,
      ordered_unique< tag<by_price>,
         composite_key< limit_order_object,
            member< limit_order_object, price, &limit_order_object::sell_price>,
            member< object, object_id_type, &object::id>
         >,
         composite_key_compare< std::greater<price>, std::less<object_id_type> >
      >,
      ordered_unique< tag<by_account>,
         composite_key< limit_order_object,
            member<limit_order_object, account_uid_type, &limit_order_object::seller>,
            member<object, object_id_type, &object::id>
         >
      >
   >
> limit_order_multi_index_type;

typedef generic_index<limit_order_object, limit_order_multi_index_type> limit_order_index;


} } // graphene::chain

FC_REFLECT_DERIVED( graphene::chain::limit_order_object,
                    (graphene::db::object),
                    (expiration)(seller)(for_sale)(sell_price)(deferred_fee)
                  )