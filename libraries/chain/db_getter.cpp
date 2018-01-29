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

#include <fc/smart_ref_impl.hpp>

namespace graphene { namespace chain {

const asset_object& database::get_core_asset() const
{
   return get(asset_id_type());
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
      return itr->get_id();
   else
      return optional<account_id_type>();
}

const account_statistics_object& database::get_account_statistics_by_uid( account_uid_type uid )const
{
   const auto& idx = get_index_type<account_statistics_index>().indices().get<by_uid>();
   auto itr = idx.find(uid);
   FC_ASSERT( itr != idx.end(), "account ${uid} not found.", ("uid",uid) );
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
   const auto& platforms_by_owner = get_index_type<platform_index>().indices().get<by_owner>();
   auto itr = platforms_by_owner.find(owner);
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
              "post ${pid}_${uid}_${post_pid} not found.",
              ("pid",platform)("uid",poster)("post_pid",post_pid) );
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




} }
