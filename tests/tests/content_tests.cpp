
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

        flat_set<fc::ecc::private_key> sign_keys;
        sign_keys.insert(u_1000_private_key);
        transfer_extension(sign_keys, u_1000_id, u_1000_id, _core(6000), "", true, false);
        const account_statistics_object& ant1000 = db.get_account_statistics_by_uid(u_1000_id);
        BOOST_CHECK(ant1000.prepaid == 6000 * prec);
        BOOST_CHECK(ant1000.core_balance == 4000 * prec);

        transfer_extension(sign_keys, u_1000_id, u_1001_id, _core(5000), "", false, true);
        const account_statistics_object& ant1000_1 = db.get_account_statistics_by_uid(u_1000_id);
        const account_statistics_object& ant1001 = db.get_account_statistics_by_uid(u_1001_id);
        BOOST_CHECK(ant1000_1.prepaid == 1000 * prec);
        BOOST_CHECK(ant1001.core_balance == 15000 * prec);

        flat_set<fc::ecc::private_key> sign_keys1;
        sign_keys1.insert(u_1001_private_key);
        transfer_extension(sign_keys1, u_1001_id, u_1000_id, _core(15000), "", true, true);
        const account_statistics_object& ant1000_2 = db.get_account_statistics_by_uid(u_1000_id);
        BOOST_CHECK(ant1000_2.prepaid == 1000 * prec);
        BOOST_CHECK(ant1000_2.core_balance == 19000 * prec);

        transfer_extension(sign_keys, u_1000_id, u_1001_id, _core(1000), "", false, false);
        const account_statistics_object& ant1001_2 = db.get_account_statistics_by_uid(u_1001_id);
        const account_statistics_object& ant1000_3 = db.get_account_statistics_by_uid(u_1000_id);
        BOOST_CHECK(ant1001_2.prepaid == 1000 * prec);
        BOOST_CHECK(ant1000_3.prepaid == 0);

    } catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(account_auth_platform_test)
{
    try{
        ACTORS((1000)(9000));
        const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);
        auto _core = [&](int64_t x) -> asset
        {  return asset(x*prec);    };
        transfer(committee_account, u_1000_id, _core(10000));
        transfer(committee_account, u_9000_id, _core(10000));
        add_csaf_for_account(u_1000_id, 10000);
        add_csaf_for_account(u_9000_id, 10000);

        flat_set<fc::ecc::private_key> sign_keys;
        sign_keys.insert(u_9000_private_key);
        create_platform(u_9000_id, "platform", _core(10000), "www.123456789.com", "", sign_keys);

        flat_set<fc::ecc::private_key> sign_keys1;
        sign_keys1.insert(u_1000_private_key);
        account_auth_platform(sign_keys1, u_1000_id, u_9000_id, 1000 * prec, account_statistics_object::Platform_Permission_Forward |
                                                                             account_statistics_object::Platform_Permission_Liked |
                                                                             account_statistics_object::Platform_Permission_Buyout |
                                                                             account_statistics_object::Platform_Permission_Comment |
                                                                             account_statistics_object::Platform_Permission_Reward);

        const account_statistics_object& ant1000 = db.get_account_statistics_by_uid(u_1000_id);
        auto iter = ant1000.prepaids_for_platform.find(u_9000_id);
        BOOST_CHECK(iter != ant1000.prepaids_for_platform.end());
        BOOST_CHECK(iter->second.max_limit == 1000 * prec);
        BOOST_CHECK(iter->second.permission_flags & account_statistics_object::Platform_Permission_Forward);
        BOOST_CHECK(iter->second.permission_flags & account_statistics_object::Platform_Permission_Liked);
        BOOST_CHECK(iter->second.permission_flags & account_statistics_object::Platform_Permission_Buyout);
        BOOST_CHECK(iter->second.permission_flags & account_statistics_object::Platform_Permission_Comment);
        BOOST_CHECK(iter->second.permission_flags & account_statistics_object::Platform_Permission_Reward);
    }
    catch (fc::exception& e) {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE(license_test)
{
    try{
        ACTORS((1000)(9000));
        const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);
        auto _core = [&](int64_t x) -> asset
        {  return asset(x*prec);    };
        transfer(committee_account, u_9000_id, _core(10000));
        add_csaf_for_account(u_9000_id, 10000);

        flat_set<fc::ecc::private_key> sign_keys;
        sign_keys.insert(u_9000_private_key);
        create_platform(u_9000_id, "platform", _core(10000), "www.123456789.com", "", sign_keys);

        create_license(u_9000_id, 6, "999999999", "license title", "license body", "extra", sign_keys);

        const license_object& license = db.get_license_by_platform(u_9000_id, 1);
        BOOST_CHECK(license.license_type == 6);
        BOOST_CHECK(license.hash_value == "999999999");
        BOOST_CHECK(license.extra_data == "extra");
        BOOST_CHECK(license.title == "license title");
        BOOST_CHECK(license.body == "license body");
    }
    catch (fc::exception& e) {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE(post_test)
{
    try{
        ACTORS((1000)(2000)(9000));
        const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);
        auto _core = [&](int64_t x) -> asset
        {  return asset(x*prec);    };
        transfer(committee_account, u_1000_id, _core(10000));
        transfer(committee_account, u_2000_id, _core(10000));
        transfer(committee_account, u_9000_id, _core(10000));
        add_csaf_for_account(u_1000_id, 10000);
        add_csaf_for_account(u_2000_id, 10000);
        add_csaf_for_account(u_9000_id, 10000);

        flat_set<fc::ecc::private_key> sign_keys;
        sign_keys.insert(u_9000_private_key);
        create_platform(u_9000_id, "platform", _core(10000), "www.123456789.com", "", sign_keys);

        flat_set<fc::ecc::private_key> sign_keys1;
        sign_keys1.insert(u_1000_private_key);
        account_auth_platform(sign_keys1, u_1000_id, u_9000_id, 1000 * prec, account_statistics_object::Platform_Permission_Forward |
            account_statistics_object::Platform_Permission_Liked |
            account_statistics_object::Platform_Permission_Buyout |
            account_statistics_object::Platform_Permission_Comment |
            account_statistics_object::Platform_Permission_Reward);

        //const account_statistics_object& poster_account_statistics = db.get_account_statistics_by_uid(u_1000_id);
        //post_operation post_op;
        //post_op.post_pid = poster_account_statistics.last_post_sequence + 1;
        //post_op.platform = u_9000_id;
        //post_op.poster = u_1000_id;
        //post_op.hash_value = "6666666";
        //post_op.extra_data = "extra";
        //post_op.title = "document name";
        //post_op.body = "document body";

        //post_op.extensions = flat_set<post_operation::extension_parameter>();
        //post_operation::ext extension;
        //extension.post_type = ;
        //extension.forward_price = ;
        //extension.license_lid = ;
        //extension.permission_flags = ;
        //extension.receiptors = ;


    }
    catch (fc::exception& e) {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_SUITE_END()
