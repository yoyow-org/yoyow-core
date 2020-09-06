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

#if 0
#include <fc/uint128.hpp>

#include <graphene/chain/hardfork.hpp>

#include <graphene/chain/market_object.hpp>
#include <graphene/chain/exceptions.hpp>

#include <boost/test/unit_test.hpp>

#include "../common/database_fixture.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;

BOOST_FIXTURE_TEST_SUITE( fee_tests, database_fixture )

BOOST_AUTO_TEST_CASE( nonzero_fee_test )
{
   try
   {
      ACTORS((1000)(1001));

      const share_type prec = asset::scaled_precision( asset_id_type()(db).precision );

      // Return number of core shares (times precision)
      auto _core = [&]( int64_t x ) -> asset
      {  return asset( x*prec );    };

      transfer( committee_account, u_1000_id, _core(1000000) );

      // make sure the database requires our fee to be nonzero
      enable_fees();

      signed_transaction tx;
      transfer_operation xfer_op;
      xfer_op.from = u_1000_id;
      xfer_op.to = u_1001_id;
      xfer_op.amount = _core(1000);
      xfer_op.fee = _core(0);
      tx.operations.push_back( xfer_op );
      set_expiration( db, tx );
      sign( tx, u_1000_private_key );
      GRAPHENE_REQUIRE_THROW( PUSH_TX( db, tx ), insufficient_fee );
   }
   catch( const fc::exception& e )
   {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_SUITE_END()
#endif