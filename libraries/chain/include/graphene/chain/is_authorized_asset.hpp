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

namespace graphene { namespace chain {

class account_object;
class asset_object;
class database;

namespace detail {

void _validate_authorized_asset( const database& d,
                                 const account_object& acct,
                                 const asset_object& asset_obj,
                                 const string& account_desc_prefix = "" );
}

/**
 * validate if the account is whitelisted and not blacklisted to transact the asset,
 * and the asset is allowed by the account.
 */
inline void validate_authorized_asset( const database& d,
                                       const account_object& acct,
                                       const asset_object& asset_obj,
                                       string account_desc_prefix = "" )
{
   if( asset_obj.enabled_whitelist() || acct.enabled_allowed_assets() )
      detail::_validate_authorized_asset( d, acct, asset_obj, account_desc_prefix );
}

} }
