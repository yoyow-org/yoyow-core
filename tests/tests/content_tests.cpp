
#include <boost/test/unit_test.hpp>

#include <graphene/chain/database.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>

#include <graphene/chain/protocol/content.hpp>
#include <graphene/chain/content_object.hpp>

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/witness_object.hpp>

#include <fc/crypto/digest.hpp>
#include <fc/log/logger.hpp>

#include "../common/database_fixture.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;

BOOST_FIXTURE_TEST_SUITE( operation_tests, database_fixture )

BOOST_AUTO_TEST_CASE(transfer_extension_test)
{
    try{
        ACTORS((1000)(1001));

        const account_object& u1000 = db.get_account_by_uid(u_1000_id);
        dlog("u_1000_id : ${u}",("u",u1000));

        const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);

        // Return number of core shares (times precision)
        auto _core = [&](int64_t x) -> asset
        {  return asset(x*prec);    };

        transfer(committee_account, u_1000_id, _core(10000));
        transfer(committee_account, u_1001_id, _core(10000));
        add_csaf_for_account(u_1000_id, 10000);
        add_csaf_for_account(u_1001_id, 10000);
        const account_statistics_object& temp = db.get_account_statistics_by_uid(u_1000_id);

        // make sure the database requires our fee to be nonzero
        enable_fees();

        signed_transaction tx;
        transfer_operation xfer_op;
        xfer_op.extensions = extension< transfer_operation::ext >();
        xfer_op.extensions->value.from_balance = 6000 * prec;
        xfer_op.extensions->value.to_prepaid = 6000 * prec;
        xfer_op.from = u_1000_id;
        xfer_op.to = u_1000_id;
        xfer_op.amount = _core(6000);
        xfer_op.fee = _core(0);

        tx.operations.push_back(xfer_op);
        set_expiration(db, tx);
        
        for (auto& op : tx.operations){
            const fee_schedule& s = db.get_global_properties().parameters.current_fees;
            s.set_fee_with_csaf(op);
            //db.current_fee_schedule().set_fee(op);
        }
        tx.validate();
        sign(tx, u_1000_private_key);
        PUSH_TX(db, tx);

        const account_statistics_object& ant1000 = db.get_account_statistics_by_uid(u_1000_id);
        BOOST_CHECK(ant1000.prepaid == 6000 * prec);
        BOOST_CHECK(ant1000.core_balance == 4000 * prec);

        signed_transaction tx2;
        transfer_operation xfer_op2;
        xfer_op2.extensions = extension< transfer_operation::ext >();
        xfer_op2.extensions->value.from_prepaid = 5000 * prec;
        xfer_op2.extensions->value.to_balance = 5000 * prec;
        xfer_op2.from = u_1000_id;
        xfer_op2.to = u_1001_id;
        xfer_op2.amount = _core(5000);
        xfer_op2.fee = _core(0);

        tx2.operations.push_back(xfer_op2);
        set_expiration(db, tx2);
        for (auto& op : tx2.operations){
            const fee_schedule& s = db.get_global_properties().parameters.current_fees;
            s.set_fee_with_csaf(op);
            //db.current_fee_schedule().set_fee(op);
        }
        tx2.validate();
        sign(tx2, u_1000_private_key);
        PUSH_TX(db, tx2);

        const account_statistics_object& ant1000_1 = db.get_account_statistics_by_uid(u_1000_id);
        const account_statistics_object& ant1001 = db.get_account_statistics_by_uid(u_1001_id);
        BOOST_CHECK(ant1000_1.prepaid == 1000 * prec);
        BOOST_CHECK(ant1001.core_balance == 15000 * prec);

        signed_transaction tx3;
        transfer_operation xfer_op3;
        xfer_op3.extensions = extension< transfer_operation::ext >();
        xfer_op3.extensions->value.from_balance = 15000 * prec;
        xfer_op3.extensions->value.to_balance = 15000 * prec;
        xfer_op3.from = u_1001_id;
        xfer_op3.to = u_1000_id;
        xfer_op3.amount = _core(15000);
        xfer_op3.fee = _core(0);

        tx3.operations.push_back(xfer_op3);
        set_expiration(db, tx3);
        for (auto& op : tx3.operations){
            const fee_schedule& s = db.get_global_properties().parameters.current_fees;
            s.set_fee_with_csaf(op);
            //db.current_fee_schedule().set_fee(op);
        }
        tx3.validate();
        sign(tx3, u_1001_private_key);
        PUSH_TX(db, tx3);

        const account_statistics_object& ant1000_2 = db.get_account_statistics_by_uid(u_1000_id);
        BOOST_CHECK(ant1000_2.prepaid == 1000 * prec);
        BOOST_CHECK(ant1000_2.core_balance == 19000 * prec);

        signed_transaction tx4;
        transfer_operation xfer_op4;
        xfer_op4.extensions = extension< transfer_operation::ext >();
        xfer_op4.extensions->value.from_prepaid = 1000 * prec;
        xfer_op4.extensions->value.to_prepaid = 1000 * prec;
        xfer_op4.from = u_1000_id;
        xfer_op4.to = u_1001_id;
        xfer_op4.amount = _core(1000);
        xfer_op4.fee = _core(0);

        tx4.operations.push_back(xfer_op4);
        set_expiration(db, tx4);
        for (auto& op : tx4.operations){
            const fee_schedule& s = db.get_global_properties().parameters.current_fees;
            s.set_fee_with_csaf(op);
            //db.current_fee_schedule().set_fee(op);
        }
        tx4.validate();
        sign(tx4, u_1000_private_key);
        PUSH_TX(db, tx4);

        const account_statistics_object& ant1001_2 = db.get_account_statistics_by_uid(u_1001_id);
        const account_statistics_object& ant1000_3 = db.get_account_statistics_by_uid(u_1000_id);
        BOOST_CHECK(ant1001_2.prepaid == 1000 * prec);
        BOOST_CHECK(ant1000_3.prepaid == 0);

    } catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_SUITE_END()
