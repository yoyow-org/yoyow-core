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
#include <graphene/chain/protocol/market.hpp>

namespace graphene { namespace chain {

share_type limit_order_create_operation::calculate_fee(const fee_parameters_type& k)const
{
    share_type core_fee_required = k.fee;
    return core_fee_required;
}

void limit_order_create_operation::validate()const
{
    FC_ASSERT(amount_to_sell.asset_id != min_to_receive.asset_id);
    FC_ASSERT(amount_to_sell.amount > 0);
    FC_ASSERT(min_to_receive.amount > 0);
    validate_op_fee(fee, "limit_order_create ");
    validate_account_uid(seller, "seller ");
    FC_ASSERT(!extensions.valid(), "extension is currently not allowed");
}

share_type limit_order_cancel_operation::calculate_fee(const fee_parameters_type& k)const
{
    share_type core_fee_required = k.fee;
    return core_fee_required;
}

void limit_order_cancel_operation::validate()const
{
    validate_op_fee(fee, "limit_order_cancel ");
    validate_account_uid(fee_paying_account, "fee_paying_account ");
    FC_ASSERT(!extensions.valid(), "extension is currently not allowed");
}

share_type fill_order_operation::calculate_fee(const fee_parameters_type& k)const
{
    share_type core_fee_required = k.fee;
    return core_fee_required;
}

void fill_order_operation::validate()const
{
    FC_ASSERT(!"virtual operation");
}

void market_fee_collect_operation::validate()const
{
   validate_op_fee(fee, "market_fee_collect ");
   validate_account_uid(account, "fee_paying_account ");

   FC_ASSERT(amount > 0, "amount should more than 0. ");
   FC_ASSERT(!extensions.valid(), "extension is currently not allowed");
}

} } // graphene::chain
