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
#include <graphene/chain/protocol/config.hpp>
#include <graphene/chain/protocol/types.hpp>

namespace graphene { namespace chain {

   extern const int64_t scaled_precision_lut[];

   struct asset
   {
      asset( share_type a = 0, asset_aid_type id = 1 )
      :amount(a),asset_id(id){}

      share_type    amount;
      asset_aid_type asset_id;

      asset& operator += ( const asset& o )
      {
         FC_ASSERT( asset_id == o.asset_id );
         amount += o.amount;
         return *this;
      }
      asset& operator -= ( const asset& o )
      {
         FC_ASSERT( asset_id == o.asset_id );
         amount -= o.amount;
         return *this;
      }
      asset operator -()const { return asset( -amount, asset_id ); }

      friend bool operator == ( const asset& a, const asset& b )
      {
         return std::tie( a.asset_id, a.amount ) == std::tie( b.asset_id, b.amount );
      }
      friend bool operator < ( const asset& a, const asset& b )
      {
         FC_ASSERT( a.asset_id == b.asset_id );
         return a.amount < b.amount;
      }
      friend bool operator <= ( const asset& a, const asset& b )
      {
         return (a == b) || (a < b);
      }

      friend bool operator != ( const asset& a, const asset& b )
      {
         return !(a == b);
      }
      friend bool operator > ( const asset& a, const asset& b )
      {
         return !(a <= b);
      }
      friend bool operator >= ( const asset& a, const asset& b )
      {
         return !(a < b);
      }

      friend asset operator - ( const asset& a, const asset& b )
      {
         FC_ASSERT( a.asset_id == b.asset_id );
         return asset( a.amount - b.amount, a.asset_id );
      }
      friend asset operator + ( const asset& a, const asset& b )
      {
         FC_ASSERT( a.asset_id == b.asset_id );
         return asset( a.amount + b.amount, a.asset_id );
      }

      static share_type scaled_precision( uint8_t precision )
      {
         FC_ASSERT( precision < 19 );
         return scaled_precision_lut[ precision ];
      }
   };

   /**
    * @brief The price struct stores asset prices in the Graphene system.
    *
    * A price is defined as a ratio between two assets, and represents a possible exchange rate between those two
    * assets. prices are generally not stored in any simplified form, i.e. a price of (1000 CORE)/(20 USD) is perfectly
    * normal.
    *
    * The assets within a price are labeled base and quote. Throughout the Graphene code base, the convention used is
    * that the base asset is the asset being sold, and the quote asset is the asset being purchased, where the price is
    * represented as base/quote, so in the example price above the seller is looking to sell CORE asset and get USD in
    * return.
    */
   struct price
   {
      explicit price(const asset& base = asset(), const asset quote = asset())
         : base(base),quote(quote){}

      asset base;
      asset quote;

      static price max(asset_aid_type base, asset_aid_type quote );
      static price min(asset_aid_type base, asset_aid_type quote );

      static price max(asset_id_type base, asset_id_type quote )
      { return max( object_id_type(base).instance(), object_id_type(quote).instance() ); }
      static price min(asset_id_type base, asset_id_type quote )
      { return min( object_id_type(base).instance(), object_id_type(quote).instance() ); }

      static price call_price(const asset& debt, const asset& collateral, uint16_t collateral_ratio);

      /// The unit price for an asset type A is defined to be a price such that for any asset m, m*A=m
      static price unit_price(asset_aid_type a = GRAPHENE_CORE_ASSET_AID) { return price(asset(1, a), asset(1, a)); }

      price max()const { return price::max( base.asset_id, quote.asset_id ); }
      price min()const { return price::min( base.asset_id, quote.asset_id ); }

      double to_real()const { return double(base.amount.value)/double(quote.amount.value); }

      bool is_null()const;
      void validate()const;
   };

   price operator / ( const asset& base, const asset& quote );
   inline price operator~( const price& p ) { return price{p.quote,p.base}; }

   bool  operator <  ( const asset& a, const asset& b );
   bool  operator <= ( const asset& a, const asset& b );
   bool  operator <  ( const price& a, const price& b );
   bool  operator <= ( const price& a, const price& b );
   bool  operator >  ( const price& a, const price& b );
   bool  operator >= ( const price& a, const price& b );
   bool  operator == ( const price& a, const price& b );
   bool  operator != ( const price& a, const price& b );
   asset operator *  ( const asset& a, const price& b );

} }

FC_REFLECT( graphene::chain::asset, (amount)(asset_id) )
FC_REFLECT( graphene::chain::price, (base)(quote) )
