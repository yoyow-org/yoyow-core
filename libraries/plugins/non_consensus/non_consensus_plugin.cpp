/*
 * Copyright (c) YOYOW-team., and contributors.
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

#include <graphene/non_consensus/non_consensus_plugin.hpp>

#include <graphene/chain/impacted.hpp>
#include <graphene/chain/account_evaluator.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/config.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/operation_history_object.hpp>
#include <graphene/chain/transaction_evaluation_state.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>

namespace graphene { namespace non_consensus {

namespace detail
{
using namespace non_consensus;
class non_consensus_plugin_impl
{
   public:
      non_consensus_plugin_impl(non_consensus_plugin& _plugin)
         : _self( _plugin )
      { }
      virtual ~non_consensus_plugin_impl(){};

      non_consensus_plugin& _self;
      graphene::chain::database& database()
      {
         return _self.database();
      }
   void update_custom_vote(const account_uid_type& account, asset delta);
   void create_custom_vote_index(const custom_vote_cast_operation & op);
};

void non_consensus_plugin_impl::update_custom_vote(const account_uid_type& account,asset delta)
{  
   graphene::chain::database& db = database();
   const auto& custom_vote_idx = db.get_index_type<custom_vote_index>().indices().get<by_creater>();
   const auto& cast_vote_idx = db.get_index_type<cast_custom_vote_index>().indices().get<by_custom_vote_asset_id>();
   auto cast_vote_itr = cast_vote_idx.lower_bound(std::make_tuple(account, delta.asset_id, db.head_block_time()));

   while (cast_vote_itr != cast_vote_idx.end() && cast_vote_itr->voter == account
      && cast_vote_itr->vote_asset_id == delta.asset_id)
   {
      auto custom_vote_itr = custom_vote_idx.find(std::make_tuple(cast_vote_itr->custom_vote_creater,
         cast_vote_itr->custom_vote_vid));
      FC_ASSERT(custom_vote_itr != custom_vote_idx.end(),
         "custom vote ${id} not found.", ("id", cast_vote_itr->custom_vote_vid));

      db.modify(*custom_vote_itr, [&](custom_vote_object& obj)
      {
         for (const auto& v : cast_vote_itr->vote_result)
            obj.vote_result.at(v) += delta.amount.value;
      });
      ++cast_vote_itr;
   }

}
void non_consensus_plugin_impl::create_custom_vote_index(const custom_vote_cast_operation & op)
{
   graphene::chain::database& db = database();

   auto custom_vote_obj = db.find_custom_vote_by_vid(op.custom_vote_creater, op.custom_vote_vid);
   uint64_t votes = db.get_account_statistics_by_uid(op.voter).get_votes_from_core_balance();//db.get_balance(op.voter, custom_vote_obj->vote_asset_id).amount;
   const auto& cast_idx = db.get_index_type<cast_custom_vote_index>().indices().get<by_custom_voter>();
   auto cast_itr = cast_idx.find(std::make_tuple(op.voter, op.custom_vote_creater, op.custom_vote_vid));
 
   if (cast_itr == cast_idx.end()) {
      db.create<cast_custom_vote_object>([&](cast_custom_vote_object& obj)
      {
         obj.voter = op.voter;
         obj.custom_vote_creater = op.custom_vote_creater;
         obj.custom_vote_vid = op.custom_vote_vid;
         obj.vote_result = op.vote_result;
         obj.vote_asset_id = custom_vote_obj->vote_asset_id;
         obj.vote_expired_time = custom_vote_obj->vote_expired_time;
      });
      db.modify(*custom_vote_obj, [&](custom_vote_object& obj)
      {
         for (const auto& v : op.vote_result)
            obj.vote_result.at(v) += votes;
      });
   }
   else {
      db.modify(*custom_vote_obj, [&](custom_vote_object& obj)
      {
         for (const auto& v : cast_itr->vote_result)
            obj.vote_result.at(v) -= votes;
         for (const auto& v : op.vote_result)
            obj.vote_result.at(v) += votes;
      });
      db.modify(*cast_itr, [&](cast_custom_vote_object& obj)
      {
         obj.vote_result = op.vote_result;
      });     
   }
}
}

void non_consensus_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
   const std::vector<std::string>& ops = options["non_consensus_indexs"].as<std::vector<std::string>>();
   std::transform(ops.begin(), ops.end(), std::inserter(non_consensus_indexs, non_consensus_indexs.end()), [&](const std::string& s){return s;});
   
   if(non_consensus_indexs.count("customer_vote")){
      database().balance_adjusted.connect( [&]( const account_uid_type& account,const asset& delta){ my->update_custom_vote(account,delta); } );  
      
      database().update_non_consensus_index.connect( [&]( const operation& op) {
         if(op.which()==operation::tag<custom_vote_cast_operation>::value)
            my->create_custom_vote_index(op.get<custom_vote_cast_operation>());
         });      
   }
}

void non_consensus_plugin::plugin_startup()
{
}
   
non_consensus_plugin::non_consensus_plugin() :
   my( new detail::non_consensus_plugin_impl(*this) )
{
}
   
std::string non_consensus_plugin::plugin_name()const
{
   return "non_consensus";
}
   
non_consensus_plugin::~non_consensus_plugin()
{
}
void non_consensus_plugin::plugin_set_program_options(
                                                        boost::program_options::options_description& cli,
                                                        boost::program_options::options_description& cfg
                                                        )
{
    cli.add_options()("non_consensus_indexs", boost::program_options::value<std::vector<std::string>>()->composing()->multitoken(), "add non consensus index");
    cfg.add(cli);
}
} }
