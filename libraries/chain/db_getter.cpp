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

#include <graphene/chain/database.hpp>

#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/chain_property_object.hpp>
#include <graphene/chain/global_property_object.hpp>
#include <graphene/chain/advertising_object.hpp>
#include <graphene/chain/pledge_mining_object.hpp>

#include <fc/smart_ref_impl.hpp>

namespace graphene { namespace chain {

const asset_object& database::get_core_asset() const
{
   return get(asset_id_type());
}

const asset_object& database::get_asset_by_aid( asset_aid_type aid )const
{
   const auto& assets_by_aid = get_index_type<asset_index>().indices().get<by_aid>();
   auto itr = assets_by_aid.find(aid);
   FC_ASSERT( itr != assets_by_aid.end(), "asset ${aid} not found.", ("aid",aid) );
   return *itr;
}

const asset_object* database::find_asset_by_aid( asset_aid_type aid )const
{
   const auto& assets_by_aid = get_index_type<asset_index>().indices().get<by_aid>();
   auto itr = assets_by_aid.find(aid);
   if( itr != assets_by_aid.end() )
      return &(*itr);
   else
      return nullptr;
}

const global_property_object& database::get_global_properties()const
{
   return get( global_property_id_type() );
}

const chain_property_object& database::get_chain_properties()const
{
   return get( chain_property_id_type() );
}

const dynamic_global_property_object&database::get_dynamic_global_properties() const
{
   return get( dynamic_global_property_id_type() );
}

const fee_schedule&  database::current_fee_schedule()const
{
   return get_global_properties().parameters.current_fees;
}

time_point_sec database::head_block_time()const
{
   return get( dynamic_global_property_id_type() ).time;
}

uint32_t database::head_block_num()const
{
   return get( dynamic_global_property_id_type() ).head_block_number;
}

block_id_type database::head_block_id()const
{
   return get( dynamic_global_property_id_type() ).head_block_id;
}

decltype( chain_parameters::block_interval ) database::block_interval( )const
{
   return get_global_properties().parameters.block_interval;
}

const chain_id_type& database::get_chain_id( )const
{
   return get_chain_properties().chain_id;
}

const node_property_object& database::get_node_properties()const
{
   return _node_property_object;
}

node_property_object& database::node_properties()
{
   return _node_property_object;
}

uint32_t database::last_non_undoable_block_num() const
{
   return head_block_num() - _undo_db.size();
}

const account_object& database::get_account_by_uid( account_uid_type uid )const
{
   const auto& accounts_by_uid = get_index_type<account_index>().indices().get<by_uid>();
   auto itr = accounts_by_uid.find(uid);
   FC_ASSERT( itr != accounts_by_uid.end(), "account ${uid} not found.", ("uid",uid) );
   return *itr;
}

const account_object* database::find_account_by_uid( account_uid_type uid )const
{
   const auto& accounts_by_uid = get_index_type<account_index>().indices().get<by_uid>();
   auto itr = accounts_by_uid.find(uid);
   if( itr != accounts_by_uid.end() )
      return &(*itr);
   else
      return nullptr;
}

const optional<account_id_type> database::find_account_id_by_uid( account_uid_type uid )const
{
   const auto& accounts_by_uid = get_index_type<account_index>().indices().get<by_uid>();
   auto itr = accounts_by_uid.find(uid);
   if( itr != accounts_by_uid.end() )
      return optional<account_id_type>(itr->get_id());
   else
      return optional<account_id_type>();
}

const _account_statistics_object& database::get_account_statistics_by_uid( account_uid_type uid )const
{
   const auto& idx = get_index_type<account_statistics_index>().indices().get<by_uid>();
   auto itr = idx.find(uid);
   FC_ASSERT( itr != idx.end(), "account ${uid} not found.", ("uid",uid) );
   return *itr;
}

account_statistics_object database::get_account_statistics_struct_by_uid(account_uid_type uid)const
{
   const auto& idx = get_index_type<account_statistics_index>().indices().get<by_uid>();
   auto itr = idx.find(uid);
   FC_ASSERT(itr != idx.end(), "account ${uid} not found.", ("uid", uid));
   const _account_statistics_object& ant = *itr;

   account_statistics_object obj;
   obj.owner          = ant.owner;
   obj.most_recent_op = ant.most_recent_op;
   obj.total_ops      = ant.total_ops;
   obj.removed_ops    = ant.removed_ops;

   obj.prepaid              = ant.prepaid;
   obj.csaf                 = ant.csaf;
   obj.core_balance         = ant.core_balance;
   obj.core_leased_in       = ant.core_leased_in;
   obj.core_leased_out      = ant.core_leased_out;
   obj.total_core_in_orders = ant.total_core_in_orders;

   obj.average_coins                   = ant.average_coins;
   obj.average_coins_last_update       = ant.average_coins_last_update;
   obj.coin_seconds_earned             = ant.coin_seconds_earned;
   obj.coin_seconds_earned_last_update = ant.coin_seconds_earned_last_update;

   obj.last_witness_sequence          = ant.last_witness_sequence;
   obj.last_committee_member_sequence = ant.last_committee_member_sequence;
   obj.last_voter_sequence            = ant.last_voter_sequence;
   obj.last_platform_sequence         = ant.last_platform_sequence;
   obj.last_post_sequence             = ant.last_post_sequence;
   obj.last_custom_vote_sequence      = ant.last_custom_vote_sequence;
   obj.last_advertising_sequence      = ant.last_advertising_sequence;
   obj.last_license_sequence          = ant.last_license_sequence;

   obj.can_vote = ant.can_vote;
   obj.is_voter = ant.is_voter;

   obj.uncollected_witness_pay  = ant.uncollected_witness_pay;
   obj.uncollected_pledge_bonus = ant.uncollected_pledge_bonus;
   obj.uncollected_market_fees  = ant.uncollected_market_fees;
   obj.uncollected_score_bonus  = ant.uncollected_score_bonus;

   obj.witness_last_confirmed_block_num = ant.witness_last_confirmed_block_num;
   obj.witness_last_aslot               = ant.witness_last_aslot;
   obj.witness_total_produced           = ant.witness_total_produced;
   obj.witness_total_missed             = ant.witness_total_missed;
   obj.witness_last_reported_block_num  = ant.witness_last_reported_block_num;
   obj.witness_total_reported           = ant.witness_total_reported;

   obj.total_mining_pledge = ant.total_mining_pledge;
   obj.beneficiary = ant.beneficiary;

   if (ant.pledge_balance_ids.count(pledge_balance_type::Commitment)){
      const pledge_balance_object& pledge_balance_obj = get<pledge_balance_object>(ant.pledge_balance_ids.at(pledge_balance_type::Commitment));
      obj.total_committee_member_pledge = pledge_balance_obj.pledge;
      obj.releasing_committee_member_pledge = pledge_balance_obj.total_releasing_pledge;
      obj.committee_member_pledge_release_block_number = pledge_balance_obj.last_release_block_number();
   }

   if (ant.pledge_balance_ids.count(pledge_balance_type::Witness)){
      const pledge_balance_object& pledge_balance_obj = get<pledge_balance_object>(ant.pledge_balance_ids.at(pledge_balance_type::Witness));
      obj.total_witness_pledge = pledge_balance_obj.pledge;
      obj.releasing_witness_pledge = pledge_balance_obj.total_releasing_pledge;
      obj.witness_pledge_release_block_number = pledge_balance_obj.last_release_block_number();
   }

   if (ant.pledge_balance_ids.count(pledge_balance_type::Platform)){
      const pledge_balance_object& pledge_balance_obj = get<pledge_balance_object>(ant.pledge_balance_ids.at(pledge_balance_type::Platform));
      obj.total_platform_pledge = pledge_balance_obj.pledge;
      obj.releasing_platform_pledge = pledge_balance_obj.total_releasing_pledge;
      obj.platform_pledge_release_block_number = pledge_balance_obj.last_release_block_number();
   }

   if (ant.pledge_balance_ids.count(pledge_balance_type::Lock_balance)){
      const pledge_balance_object& pledge_balance_obj = get<pledge_balance_object>(ant.pledge_balance_ids.at(pledge_balance_type::Lock_balance));
      obj.locked_balance = pledge_balance_obj.pledge;
      obj.releasing_locked_balance = pledge_balance_obj.total_releasing_pledge;
      obj.locked_balance_release_block_number = pledge_balance_obj.last_release_block_number();
   }

   return obj;
}

const account_auth_platform_object* database::find_account_auth_platform_object_by_account_platform(account_uid_type account,
                                                                                                    account_uid_type platform)const
{
    const auto& idx = get_index_type<account_auth_platform_index>().indices().get<by_account_platform>();
    auto itr = idx.find(std::make_tuple(account, platform));
    if (itr != idx.end())
        return &(*itr);
    else
        return nullptr;
}

const account_auth_platform_object& database::get_account_auth_platform_object_by_account_platform(account_uid_type account,
                                                                                                   account_uid_type platform) const
{
    const auto& idx = get_index_type<account_auth_platform_index>().indices().get<by_account_platform>();
    auto itr = idx.find(std::make_tuple(account, platform));
    FC_ASSERT(itr != idx.end(), "account ${u} auth platform ${p} object not found.", ("u", account)("p", platform));
    return *itr;
}

const voter_object* database::find_voter( account_uid_type uid, uint32_t sequence )const
{
   const auto& idx = get_index_type<voter_index>().indices().get<by_uid_seq>();
   auto itr = idx.find( std::make_tuple( uid, sequence) );
   if( itr != idx.end() )
      return &(*itr);
   else
      return nullptr;
}

const witness_object& database::get_witness_by_uid( account_uid_type uid )const
{
   const auto& idx = get_index_type<witness_index>().indices().get<by_valid>();
   auto itr = idx.find( std::make_tuple( true, uid ) );
   FC_ASSERT( itr != idx.end(), "witness ${uid} not found.", ("uid",uid) );
   return *itr;
}

const witness_object* database::find_witness_by_uid( account_uid_type uid )const
{
   const auto& idx = get_index_type<witness_index>().indices().get<by_valid>();
   auto itr = idx.find( std::make_tuple( true, uid ) );
   if( itr != idx.end() )
      return &(*itr);
   else
      return nullptr;
}

const pledge_mining_object& database::get_pledge_mining(account_uid_type witness, account_uid_type pledge_account)const
{
   const auto& idx = get_index_type<pledge_mining_index>().indices().get<by_pledge_witness>();
   auto itr = idx.find(std::make_tuple(witness, pledge_account));
   FC_ASSERT(itr != idx.end(), "account ${uid} pledge to witness ${wid}not found.", ("wid", witness)("uid", pledge_account));
   return *itr;
}

const pledge_mining_object* database::find_pledge_mining(account_uid_type witness, account_uid_type pledge_account)const
{
   const auto& idx = get_index_type<pledge_mining_index>().indices().get<by_pledge_witness>();
   auto itr = idx.find(std::make_tuple(witness, pledge_account));
   if (itr != idx.end())
      return &(*itr);
   else
      return nullptr;
}

const witness_vote_object* database::find_witness_vote( account_uid_type voter_uid,
                                                        uint32_t         voter_sequence,
                                                        account_uid_type witness_uid,
                                                        uint32_t         witness_sequence )const
{
   const auto& idx = get_index_type<witness_vote_index>().indices().get<by_voter_seq>();
   auto itr = idx.find( std::make_tuple( voter_uid, voter_sequence, witness_uid, witness_sequence ) );
   if( itr != idx.end() )
      return &(*itr);
   else
      return nullptr;
}

const committee_member_object& database::get_committee_member_by_uid( account_uid_type uid )const
{
   const auto& idx = get_index_type<committee_member_index>().indices().get<by_valid>();
   auto itr = idx.find( std::make_tuple( true, uid ) );
   FC_ASSERT( itr != idx.end(), "committee member ${uid} not found.", ("uid",uid) );
   return *itr;
}

const committee_member_object* database::find_committee_member_by_uid( account_uid_type uid )const
{
   const auto& idx = get_index_type<committee_member_index>().indices().get<by_valid>();
   auto itr = idx.find( std::make_tuple( true, uid ) );
   if( itr != idx.end() )
      return &(*itr);
   else
      return nullptr;
}

const committee_member_vote_object* database::find_committee_member_vote( account_uid_type voter_uid,
                                                                          uint32_t         voter_sequence,
                                                                          account_uid_type committee_member_uid,
                                                                          uint32_t         committee_member_sequence )const
{
   const auto& idx = get_index_type<committee_member_vote_index>().indices().get<by_voter_seq>();
   auto itr = idx.find( std::make_tuple( voter_uid, voter_sequence, committee_member_uid, committee_member_sequence ) );
   if( itr != idx.end() )
      return &(*itr);
   else
      return nullptr;
}

const committee_proposal_object& database::get_committee_proposal_by_number( committee_proposal_number_type number )const
{
   const auto& idx = get_index_type<committee_proposal_index>().indices().get<by_number>();
   auto itr = idx.find( number );
   FC_ASSERT( itr != idx.end(), "committee proposal ${n} not found.", ("n",number) );
   return *itr;
}

const registrar_takeover_object& database::get_registrar_takeover_object( account_uid_type uid )const
{
   const auto& idx = get_index_type<registrar_takeover_index>().indices().get<by_original>();
   auto itr = idx.find( uid );
   FC_ASSERT( itr != idx.end(), "takeover registrar for registrar ${uid} not found.", ("uid",uid) );
   return *itr;
}

const registrar_takeover_object* database::find_registrar_takeover_object( account_uid_type uid )const
{
   const auto& idx = get_index_type<registrar_takeover_index>().indices().get<by_original>();
   auto itr = idx.find( uid );
   if( itr != idx.end() )
      return &(*itr);
   else
      return nullptr;
}

const platform_object& database::get_platform_by_owner( account_uid_type owner )const
{
   const auto& platforms_by_owner = get_index_type<platform_index>().indices().get<by_valid>();
   auto itr = platforms_by_owner.find( std::make_tuple( true, owner ) );
   FC_ASSERT( itr != platforms_by_owner.end(), "platform ${owner} not found.", ("owner",owner) );
   return *itr;
}

const platform_object* database::find_platform_by_owner( account_uid_type owner )const
{
   const auto& idx = get_index_type<platform_index>().indices().get<by_valid>();
   auto itr = idx.find( std::make_tuple( true, owner ) );
   if( itr != idx.end() )
      return &(*itr);
   else
      return nullptr;
}

const platform_object* database::find_platform_by_sequence(account_uid_type owner, uint32_t sequence)const
{
    const auto& idx = get_index_type<platform_index>().indices().get<by_valid>();
    auto itr = idx.find(std::make_tuple(true, owner, sequence));
    if (itr != idx.end())
        return &(*itr);
    else
        return nullptr;
}

const platform_vote_object* database::find_platform_vote( account_uid_type voter_uid,
                                                        uint32_t         voter_sequence,
                                                        account_uid_type platform_owner,
                                                        uint32_t         platform_sequence )const
{
   const auto& idx = get_index_type<platform_vote_index>().indices().get<by_platform_voter_seq>();
   auto itr = idx.find( std::make_tuple( voter_uid, voter_sequence, platform_owner, platform_sequence ) );
   if( itr != idx.end() )
      return &(*itr);
   else
      return nullptr;
}

const post_object& database::get_post_by_platform( account_uid_type platform,
                                              account_uid_type poster,
                                              post_pid_type post_pid )const
{
   const auto& posts_by_pid = get_index_type<post_index>().indices().get<by_post_pid>();
   auto itr = posts_by_pid.find(std::make_tuple(platform,poster,post_pid));
   FC_ASSERT( itr != posts_by_pid.end(),
              "post ${platform}_${uid}_${post_pid} not found.",
              ("platform",platform)("uid",poster)("post_pid",post_pid) );
   return *itr;
}

const post_object* database::find_post_by_platform( account_uid_type platform,
                                               account_uid_type poster,
                                               post_pid_type post_pid )const
{
   const auto& posts_by_pid = get_index_type<post_index>().indices().get<by_post_pid>();
   auto itr = posts_by_pid.find(std::make_tuple(platform,poster,post_pid));
   if( itr != posts_by_pid.end() )
      return &(*itr);
   else
      return nullptr;
}

const license_object& database::get_license_by_platform(account_uid_type platform, license_lid_type license_lid)const
{
    const auto& license_by_lid = get_index_type<license_index>().indices().get<by_license_lid>();
    auto itr = license_by_lid.find(std::make_tuple(platform, license_lid));
    FC_ASSERT(itr != license_by_lid.end(),
        "license ${platform}_${lid} not found.",
        ("platform", platform)("lid", license_lid));
    return *itr;
}

const license_object* database::find_license_by_platform(account_uid_type platform, license_lid_type license_lid)const
{
    const auto& license_by_lid = get_index_type<license_index>().indices().get<by_license_lid>();
    auto itr = license_by_lid.find(std::make_tuple(platform, license_lid));
    if (itr != license_by_lid.end())
        return &(*itr);
    else
        return nullptr;
}

const score_object& database::get_score(account_uid_type platform,
                                        account_uid_type poster,
                                        post_pid_type post_pid,
                                        account_uid_type from_account)const
{
    const auto& scores_by_pid = get_index_type<score_index>().indices().get<by_post_pid>();
    auto itr = scores_by_pid.find(std::make_tuple(platform, poster, post_pid, from_account));
    FC_ASSERT(itr != scores_by_pid.end(),
        "score ${platform}_${uid}_${post_pid}_${from_id} not found.",
        ("platform", platform)("uid", poster)("post_pid", post_pid)("from_id", from_account));
    return *itr;
}

const score_object* database::find_score(account_uid_type platform,
                                        account_uid_type poster,
                                        post_pid_type post_pid,
                                        account_uid_type from_account)const
{
    const auto& scores_by_pid = get_index_type<score_index>().indices().get<by_post_pid>();
    auto itr = scores_by_pid.find(std::make_tuple(platform, poster, post_pid, from_account));
    if (itr != scores_by_pid.end())
        return &(*itr);
    else
        return nullptr;
}

const advertising_object*  database::find_advertising(account_uid_type platform, advertising_aid_type advertising_aid)const
{
    const auto& advertising_by_aid = get_index_type<advertising_index>().indices().get<by_advertising_platform>();
    auto itr = advertising_by_aid.find(std::make_tuple(platform, advertising_aid));
    if (itr != advertising_by_aid.end())
        return &(*itr);
    else
        return nullptr;
}

const advertising_object&  database::get_advertising(account_uid_type platform, advertising_aid_type advertising_aid)const
{
    const auto& advertising_by_aid = get_index_type<advertising_index>().indices().get<by_advertising_platform>();
    auto itr = advertising_by_aid.find(std::make_tuple(platform, advertising_aid));
    FC_ASSERT(itr != advertising_by_aid.end(),
        "advertising_object ${platform}_${uid} not found.",
        ("platform", platform)("uid", advertising_aid));
    return *itr;
}

const advertising_order_object*  database::find_advertising_order(account_uid_type platform, advertising_aid_type advertising_aid, advertising_order_oid_type order_oid)const
{
    const auto& advertising_order_by_oid = get_index_type<advertising_order_index>().indices().get<by_advertising_order_oid>();
    auto itr = advertising_order_by_oid.find(std::make_tuple(platform, advertising_aid, order_oid));
    if (itr != advertising_order_by_oid.end())
        return &(*itr);
    else
        return nullptr;
}

const advertising_order_object&  database::get_advertising_order(account_uid_type platform, advertising_aid_type advertising_aid, advertising_order_oid_type order_oid)const
{
   const auto& advertising_order_by_oid = get_index_type<advertising_order_index>().indices().get<by_advertising_order_oid>();
   auto itr = advertising_order_by_oid.find(std::make_tuple(platform, advertising_aid, order_oid));
   FC_ASSERT(itr != advertising_order_by_oid.end(),
      "advertising_order_object ${platform}_${uid}_${order} not found.",
      ("platform", platform)("uid", advertising_aid)("order", order_oid));
   return *itr;
}
const custom_vote_object& database::get_custom_vote_by_vid(account_uid_type creator, custom_vote_vid_type vote_vid)const
{
   const auto& custom_votes_by_vid = get_index_type<custom_vote_index>().indices().get<by_creater>();
   auto itr = custom_votes_by_vid.find(std::make_tuple(creator, vote_vid));
   FC_ASSERT(itr != custom_votes_by_vid.end(), "custom vote ${vid} not found.", ("vid", vote_vid));
   return *(itr);
}

const custom_vote_object* database::find_custom_vote_by_vid(account_uid_type creator, custom_vote_vid_type vote_vid)const
{
   const auto& custom_votes_by_vid = get_index_type<custom_vote_index>().indices().get<by_creater>();
   auto itr = custom_votes_by_vid.find(std::make_tuple(creator, vote_vid));
   if (itr != custom_votes_by_vid.end())
      return &(*itr);
   else 
      return nullptr;
}

} }
