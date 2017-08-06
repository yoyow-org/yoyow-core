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
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/is_authorized_asset.hpp>
#include <graphene/chain/transaction_evaluation_state.hpp>

#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/fba_object.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/market_evaluator.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>

#include <fc/uint128.hpp>

namespace graphene { namespace chain {
database& generic_evaluator::db()const { return trx_state->db(); }

   operation_result generic_evaluator::start_evaluate( transaction_evaluation_state& eval_state, const operation& op, bool apply )
   { try {
      trx_state   = &eval_state;
      //check_required_authorities(op);
      auto result = evaluate( op );

      if( apply ) result = this->apply( op );
      return result;
   } FC_CAPTURE_AND_RETHROW() }

   void generic_evaluator::prepare_fee(account_uid_type account_uid, asset fee)
   {
      fee_paying_account = &db().get_account_by_uid( account_uid );
      prepare_fee( fee );
   }

   void generic_evaluator::prepare_fee(account_id_type account_id, asset fee)
   {
      fee_paying_account = &account_id( db() );
      prepare_fee( fee );
   }

   void generic_evaluator::prepare_fee(asset fee)
   {
      const database& d = db();
      fee_from_account = fee;
      FC_ASSERT( fee.amount >= 0 );

      fee_paying_account_statistics = &fee_paying_account->statistics(d);

      fee_asset = &asset_id_type(fee.asset_id)(d);
      fee_asset_dyn_data = &fee_asset->dynamic_asset_data_id(d);

      if( d.head_block_time() > HARDFORK_419_TIME )
      {
         FC_ASSERT( is_authorized_asset( d, *fee_paying_account, *fee_asset ), "Account ${acct} '${name}' attempted to pay fee by using asset ${a} '${sym}', which is unauthorized due to whitelist / blacklist",
            ("acct", fee_paying_account->id)("name", fee_paying_account->name)("a", fee_asset->id)("sym", fee_asset->symbol) );
      }

      if( fee_from_account.asset_id == GRAPHENE_CORE_ASSET_AID )
         core_fee_paid = fee_from_account.amount;
      else
      {
         asset fee_from_pool = fee_from_account * fee_asset->options.core_exchange_rate;
         FC_ASSERT( fee_from_pool.asset_id == GRAPHENE_CORE_ASSET_AID );
         core_fee_paid = fee_from_pool.amount;
         FC_ASSERT( core_fee_paid <= fee_asset_dyn_data->fee_pool, "Fee pool balance of '${b}' is less than the ${r} required to convert ${c}",
                    ("r", db().to_pretty_string( fee_from_pool))("b",db().to_pretty_string(fee_asset_dyn_data->fee_pool))("c",db().to_pretty_string(fee)) );
      }
   }

   void generic_evaluator::prepare_fee(account_uid_type account_uid, const fee_type& fee)
   {
      fee_paying_account = &db().get_account_by_uid( account_uid );
      prepare_fee( fee );
   }

   void generic_evaluator::prepare_fee(const fee_type& fee)
   {
      const database& d = db();
      fee_paying_account_statistics = &fee_paying_account->statistics(d);

      if( !fee.options.valid() )
         fee_from_account = fee.total;
      else if( fee.options->value.from_balance.valid() )
         fee_from_account = *fee.options->value.from_balance;
      //else
      //   fee_from_account = asset(); // it's default, so no need here

      from_balance = fee_from_account.amount;
      total_fee_paid = fee.total.amount;

      if( fee.options.valid() )
      {
         const auto& fov = fee.options->value;
         //if( fov.from_balance.valid() )
         //   from_balance = fov.from_balance->amount;
         if( fov.from_prepaid.valid() )
         {
            from_prepaid = fov.from_prepaid->amount;
            FC_ASSERT( from_prepaid <= fee_paying_account_statistics->prepaid,
                       "Insufficient prepaid fee: account ${a}'s prepaid fee of ${b} is less than required ${r}",
                       ("a",fee_paying_account->uid)
                       ("b",d.to_pretty_core_string(fee_paying_account_statistics->prepaid))
                       ("r",d.to_pretty_core_string(from_prepaid)) );
         }
         if( fov.from_csaf.valid() )
         {
            from_csaf = fov.from_csaf->amount;
            /* this check is removed, so the payer can pay csaf collected in the same op
            FC_ASSERT( from_csaf <= fee_paying_account_statistics->csaf,
                       "Insufficient csaf: account ${a}'s csaf of ${b} is less than required ${r}",
                       ("a",fee_paying_account->uid)
                       ("b",d.to_pretty_core_string(fee_paying_account_statistics->csaf))
                       ("r",d.to_pretty_core_string(from_csaf)) );
            */
         }
      }

      fee_asset = &asset_id_type(fee_from_account.asset_id)(d);
      fee_asset_dyn_data = &fee_asset->dynamic_asset_data_id(d);

      if( d.head_block_time() > HARDFORK_419_TIME )
      {
         FC_ASSERT( is_authorized_asset( d, *fee_paying_account, *fee_asset ), "Account ${acct} '${name}' attempted to pay fee by using asset ${a} '${sym}', which is unauthorized due to whitelist / blacklist",
            ("acct", fee_paying_account->id)("name", fee_paying_account->name)("a", fee_asset->id)("sym", fee_asset->symbol) );
      }

      if( fee_from_account.asset_id == GRAPHENE_CORE_ASSET_AID )
         core_fee_paid = fee_from_account.amount;
      else
      {
         asset fee_from_pool = fee_from_account * fee_asset->options.core_exchange_rate;
         FC_ASSERT( fee_from_pool.asset_id == GRAPHENE_CORE_ASSET_AID );
         core_fee_paid = fee_from_pool.amount;
         FC_ASSERT( core_fee_paid <= fee_asset_dyn_data->fee_pool, "Fee pool balance of '${b}' is less than the ${r} required to convert ${c}",
                    ("r", db().to_pretty_string( fee_from_pool))("b",db().to_pretty_string(fee_asset_dyn_data->fee_pool))("c",db().to_pretty_string(fee_from_account)) );
      }
   }

   void generic_evaluator::convert_fee()
   {
      if( !trx_state->skip_fee ) {
         if( fee_asset->get_id() != asset_id_type() )
         {
            db().modify(*fee_asset_dyn_data, [this](asset_dynamic_data_object& d) {
               d.accumulated_fees += fee_from_account.amount;
               d.fee_pool -= core_fee_paid;
            });
         }
      }
   }

   void generic_evaluator::pay_fee()
   { try {
      if( !trx_state->skip_fee ) {
         database& d = db();
         /// TODO: db().pay_fee( account_id, core_fee );
         d.modify(*fee_paying_account_statistics, [&](account_statistics_object& s)
         {
            s.pay_fee( core_fee_paid, d.get_global_properties().parameters.cashback_vesting_threshold );
         });
      }
   } FC_CAPTURE_AND_RETHROW() }

   void generic_evaluator::pay_fba_fee( uint64_t fba_id )
   {
      database& d = db();
      const fba_accumulator_object& fba = d.get< fba_accumulator_object >( fba_accumulator_id_type( fba_id ) );
      if( !fba.is_configured(d) )
      {
         generic_evaluator::pay_fee();
         return;
      }
      d.modify( fba, [&]( fba_accumulator_object& _fba )
      {
         _fba.accumulated_fba_fees += core_fee_paid;
      } );
   }

   void generic_evaluator::process_fee_options()
   { try {
      if( !trx_state->skip_fee ) {
         database& d = db();
         if( from_prepaid > 0 )
            FC_ASSERT( fee_paying_account_statistics->prepaid >= from_prepaid,
                       "Insufficient Prepaid: account ${a}'s prepaid of ${b} is less than required ${r}",
                       ("a",fee_paying_account->uid)
                       ("b",d.to_pretty_core_string(fee_paying_account_statistics->prepaid))
                       ("r",d.to_pretty_core_string(from_prepaid)) );
         if( from_csaf > 0 )
            FC_ASSERT( fee_paying_account_statistics->csaf >= from_csaf,
                       "Insufficient CSAF: account ${a}'s csaf of ${b} is less than required ${r}",
                       ("a",fee_paying_account->uid)
                       ("b",d.to_pretty_core_string(fee_paying_account_statistics->csaf))
                       ("r",d.to_pretty_core_string(from_csaf)) );
         d.modify(*fee_paying_account_statistics, [&](account_statistics_object& s)
         {
            if( from_prepaid         > 0 ) s.prepaid         -= from_prepaid;
            if( from_csaf            > 0 ) s.csaf            -= from_csaf;
         });
         d.modify( asset_id_type()(d).dynamic_data(d), [&]( asset_dynamic_data_object& o )
         {
            o.current_supply -= (from_prepaid + from_balance);
         });
      }
   } FC_CAPTURE_AND_RETHROW() }

   share_type generic_evaluator::calculate_fee_for_operation(const operation& op) const
   {
      return db().current_fee_schedule().calculate_fee( op ).amount;
   }
   std::pair<share_type,share_type> generic_evaluator::calculate_fee_pair_for_operation(const operation& op) const
   {
      return db().current_fee_schedule().calculate_fee_pair( op );
   }
   void generic_evaluator::db_adjust_balance(const account_id_type& fee_payer, asset fee_from_account)
   {
      // TODO review
      FC_ASSERT( "deprecated." );
      //db().adjust_balance(fee_payer, fee_from_account);
   }
   void generic_evaluator::db_adjust_balance(const account_uid_type& fee_payer, asset fee_from_account)
   {
      db().adjust_balance(fee_payer, fee_from_account);
   }
   string generic_evaluator::db_to_pretty_string( const asset& a )const
   {
      return db().to_pretty_string( a );
   }
   string generic_evaluator::db_to_pretty_core_string( const share_type amount )const
   {
      return db().to_pretty_core_string( amount );
   }

} }
