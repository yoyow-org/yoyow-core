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
#include <graphene/chain/global_property_object.hpp>
#include <graphene/chain/node_property_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/content_object.hpp>
#include <graphene/chain/custom_vote_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/fork_database.hpp>
#include <graphene/chain/block_database.hpp>
#include <graphene/chain/genesis_state.hpp>
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/wasm_interface.hpp>
#include <graphene/chain/transaction_context.hpp>

#include <graphene/db/object_database.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/simple_index.hpp>
#include <fc/signals.hpp>

#include <graphene/chain/protocol/protocol.hpp>

#include <fc/log/logger.hpp>

#include <map>

namespace graphene { namespace chain {
   using graphene::db::abstract_object;
   using graphene::db::object;
   class op_evaluator;
   class transaction_evaluation_state;

   struct budget_record;

   /**
    *   @class database
    *   @brief tracks the blockchain state in an extensible manner
    */
   class database : public db::object_database
   {
      public:
         //////////////////// db_management.cpp ////////////////////

         database();
         ~database();

         enum validation_steps
         {
            skip_nothing                = 0,
            skip_witness_signature      = 1 << 0,  ///< used while reindexing
            skip_transaction_signatures = 1 << 1,  ///< used by non-witness nodes
            skip_transaction_dupe_check = 1 << 2,  ///< used while reindexing
            skip_fork_db                = 1 << 3,  ///< used while reindexing
            skip_block_size_check       = 1 << 4,  ///< used when applying locally generated transactions
            skip_tapos_check            = 1 << 5,  ///< used while reindexing -- note this skips expiration check as well
            skip_authority_check        = 1 << 6,  ///< used while reindexing -- disables any checking of authority on transactions
            skip_merkle_check           = 1 << 7,  ///< used while reindexing
            skip_assert_evaluation      = 1 << 8,  ///< used while reindexing
            skip_undo_history_check     = 1 << 9,  ///< used while reindexing
            skip_witness_schedule_check = 1 << 10, ///< used while reindexing
            skip_invariants_check       = 1 << 11, ///< used while reindexing and used by non-witness nodes
            skip_validate               = 1 << 12, ///< used prior to checkpoint, skips validate() call on transaction
            skip_uint_test              = 1 << 13  ///< used for uint test 
         };

         /**
          * @brief Open a database, creating a new one if necessary
          *
          * Opens a database in the specified directory. If no initialized database is found, genesis_loader is called
          * and its return value is used as the genesis state when initializing the new database
          *
          * genesis_loader will not be called if an existing database is found.
          *
          * @param data_dir Path to open or create database in
          * @param genesis_loader A callable object which returns the genesis state to initialize new databases on
          * @param db_version a version string that changes when the internal database format and/or logic is modified
          */
          void open(
             const fc::path& data_dir,
             std::function<genesis_state_type()> genesis_loader,
             const std::string& db_version );

         /**
          * @brief Rebuild object graph from block history and open detabase
          *
          * This method may be called after or instead of @ref database::open, and will rebuild the object graph by
          * replaying blockchain history. When this method exits successfully, the database will be open.
          */
         void reindex(fc::path data_dir);

         /**
          * @brief wipe Delete database from disk, and potentially the raw chain as well.
          * @param include_blocks If true, delete the raw chain as well as the database.
          *
          * Will close the database before wiping. Database will be closed when this function returns.
          */
         void wipe(const fc::path& data_dir, bool include_blocks);
         void close(bool rewind = true);

         //////////////////// db_block.cpp ////////////////////

         /**
          *  @return true if the block is in our fork DB or saved to disk as
          *  part of the official chain, otherwise return false
          */
         bool                       is_known_block( const block_id_type& id )const;
         bool                       is_known_transaction( const transaction_id_type& id )const;
         block_id_type              get_block_id_for_num( uint32_t block_num )const; // get from disk directly
         block_id_type              fetch_block_id_for_num( uint32_t block_num )const; // check fork db first
         optional<signed_block>     fetch_block_by_id( const block_id_type& id )const;
         optional<signed_block>     fetch_block_by_number( uint32_t num )const;
         const signed_transaction&  get_recent_transaction( const transaction_id_type& trx_id )const;
         std::vector<block_id_type> get_block_ids_on_fork(block_id_type head_of_fork) const;

         /**
          *  Calculate the percent of block production slots that were missed in the
          *  past 128 blocks, not including the current block.
          */
         uint32_t witness_participation_rate()const;

         void                              add_checkpoints( const flat_map<uint32_t,block_id_type>& checkpts );
         const flat_map<uint32_t,block_id_type> get_checkpoints()const { return _checkpoints; }
         bool before_last_checkpoint()const;

         // set / get max_trx_cpu_time
         void set_max_trx_cpu_time(uint32_t max_trx_cpu_time) { _max_trx_cpu_time = max_trx_cpu_time; }
         const uint32_t  get_max_trx_cpu_time() { return _max_trx_cpu_time; };

         bool push_block( const signed_block& b, uint32_t skip = skip_nothing );
         processed_transaction push_transaction( const precomputable_transaction& trx, uint32_t skip = skip_nothing );
         bool _push_block( const signed_block& b );
         processed_transaction _push_transaction( const precomputable_transaction& trx );

         ///@throws fc::exception if the proposed transaction fails to apply.
         processed_transaction push_proposal(const proposal_object& proposal, const signed_information& sigs);

         signed_block generate_block(
            const fc::time_point_sec when,
            account_uid_type witness_uid,
            const fc::ecc::private_key& block_signing_private_key,
            uint32_t skip
            );
         signed_block _generate_block(
            const fc::time_point_sec when,
            account_uid_type witness_uid,
            const fc::ecc::private_key& block_signing_private_key
            );

         void pop_block();
         void clear_pending();

         /**
          *  This method is used to track appied operations during the evaluation of a block, these
          *  operations should include any operation actually included in a transaction as well
          *  as any implied/virtual operations that resulted, such as filling an order.  The
          *  applied operations is cleared after applying each block and calling the block
          *  observers which may want to index these operations.
          *
          *  @return the op_id which can be used to set the result after it has finished being applied.
          */
         uint32_t  push_applied_operation( const operation& op );
         void      set_applied_operation_result( uint32_t op_id, const operation_result& r );
         const vector<optional< operation_history_object > >& get_applied_operations()const;

         string to_pretty_string( const asset& a )const;
         string to_pretty_core_string( const share_type amount )const;

         /**
          *  This signal is emitted after all operations and virtual operation for a
          *  block have been applied but before the get_applied_operations() are cleared.
          *
          *  You may not yield from this callback because the blockchain is holding
          *  the write lock and may be in an "inconstant state" until after it is
          *  released.
          */
         fc::signal<void(const signed_block&)>           applied_block;

         /**
          * This signal is emitted any time a new transaction is added to the pending
          * block state.
          */
         fc::signal<void(const signed_transaction&)>     on_pending_transaction;

         /**
          *  Emitted After a block has been applied and committed.  The callback
          *  should not yield and should execute quickly.
          */
         fc::signal<void(const vector<object_id_type>&, const flat_set<account_uid_type>&)> new_objects;

         /**
          *  Emitted After a block has been applied and committed.  The callback
          *  should not yield and should execute quickly.
          */
         fc::signal<void(const vector<object_id_type>&, const flat_set<account_uid_type>&)> changed_objects;

         /** this signal is emitted any time an object is removed and contains a
          * pointer to the last value of every object that was removed.
          */
         fc::signal<void(const vector<object_id_type>&, const vector<const object*>&, const flat_set<account_uid_type>&)>  removed_objects;
      
         /** this signal is emitted any time account balance adjust for update vote
          */
         fc::signal<void(const account_uid_type&,const asset& delta)>           balance_adjusted;
      
      
         /** this signal is emitted any time to update non consensus index
          */
         fc::signal<void(const operation&)>                                     update_non_consensus_index;

         //////////////////// db_witness_schedule.cpp ////////////////////

         /**
          * @brief Get the witness scheduled for block production in a slot.
          *
          * slot_num always corresponds to a time in the future.
          *
          * If slot_num == 1, returns the next scheduled witness.
          * If slot_num == 2, returns the next scheduled witness after
          * 1 block gap.
          *
          * Use the get_slot_time() and get_slot_at_time() functions
          * to convert between slot_num and timestamp.
          *
          * Passing slot_num == 0 returns GRAPHENE_NULL_WITNESS
          */
         account_uid_type get_scheduled_witness(uint32_t slot_num)const;

         /**
          * Get the time at which the given slot occurs.
          *
          * If slot_num == 0, return time_point_sec().
          *
          * If slot_num == N for N > 0, return the Nth next
          * block-interval-aligned time greater than head_block_time().
          */
         fc::time_point_sec get_slot_time(uint32_t slot_num)const;

         /**
          * Get the last slot which occurs AT or BEFORE the given time.
          *
          * The return value is the greatest value N such that
          * get_slot_time( N ) <= when.
          *
          * If no such N exists, return 0.
          */
         uint32_t get_slot_at_time(fc::time_point_sec when)const;

         void update_witness_avg_pledge( const account_uid_type uid );
         void update_witness_avg_pledge( const witness_object& wit );
         void adjust_witness_votes( const witness_object& witness, share_type delta );
      
         void handle_non_consensus_index(const operation& op);

      private:
         void update_witness_schedule();

         void reset_witness_by_pledge_schedule();
         void reset_witness_by_vote_schedule();


         //////////////////// db_voter.cpp ////////////////////
      public:
         bool check_voter_valid( const voter_object& voter, bool deep_check );
         void invalidate_voter( const voter_object& voter );
         void clear_voter_votes( const voter_object& voter );
         void adjust_voter_proxy_votes( const voter_object& voter, vector<share_type> delta, bool update_last_vote );
         void clear_voter_proxy_votes( const voter_object& voter );
         void adjust_committee_member_votes( const committee_member_object& committee_member, share_type delta );
      private:
         void update_voter_effective_votes( const voter_object& voter );
         void adjust_voter_votes( const voter_object& voter, share_type delta );
         void adjust_voter_self_votes( const voter_object& voter, share_type delta );
         void adjust_voter_self_witness_votes( const voter_object& voter, share_type delta );
         void adjust_voter_self_committee_member_votes( const voter_object& voter, share_type delta );
         void adjust_voter_self_platform_votes( const voter_object& voter, share_type delta );
         void clear_voter_witness_votes( const voter_object& voter );
         void clear_voter_committee_member_votes( const voter_object& voter );
         void clear_voter_platform_votes( const voter_object& voter );
         uint32_t process_invalid_proxied_voters( const voter_object& proxy, uint32_t max_voters_to_process );

         //////////////////// db_getter.cpp ////////////////////
      public:

         const chain_id_type&                   get_chain_id()const;
         const asset_object&                    get_core_asset()const;
         const asset_object&                    get_asset_by_aid( asset_aid_type aid )const;
         const asset_object*                    find_asset_by_aid( asset_aid_type aid )const;
         const chain_property_object&           get_chain_properties()const;
         const global_property_object&          get_global_properties()const;
		 
		 extension_parameter_type        		get_global_extension_params() const;


         const bool                             get_contract_log_to_console() const { return contract_log_to_console; }
         void                                   set_contract_log_to_console(bool log_switch) { contract_log_to_console = log_switch; }
         
         const dynamic_global_property_object&  get_dynamic_global_properties()const;
         const node_property_object&            get_node_properties()const;
         const fee_schedule&                    current_fee_schedule()const;

         time_point_sec   head_block_time()const;
         uint32_t         head_block_num()const;
         block_id_type    head_block_id()const;
         account_uid_type head_block_witness()const;

         decltype( chain_parameters::block_interval ) block_interval( )const;

         node_property_object& node_properties();


         uint32_t last_non_undoable_block_num() const;

         const account_object& get_account_by_uid( account_uid_type uid )const;
         const account_object* find_account_by_uid( account_uid_type uid )const;
         const optional<account_id_type> find_account_id_by_uid( account_uid_type uid )const;
         const _account_statistics_object& get_account_statistics_by_uid( account_uid_type uid )const;
         account_statistics_object get_account_statistics_struct_by_uid(account_uid_type uid)const;
         const account_auth_platform_object* find_account_auth_platform_object_by_account_platform(account_uid_type account,
                                                                                                   account_uid_type platform)const;
         const account_auth_platform_object& get_account_auth_platform_object_by_account_platform(account_uid_type account,
                                                                                                  account_uid_type platform) const;

         const voter_object* find_voter( account_uid_type uid, uint32_t sequence )const;

         const witness_object& get_witness_by_uid( account_uid_type uid )const;
         const witness_object* find_witness_by_uid( account_uid_type uid )const;

         const pledge_mining_object& get_pledge_mining(account_uid_type witness, account_uid_type pledge_account)const;
         const pledge_mining_object* find_pledge_mining(account_uid_type witness, account_uid_type pledge_account)const;

         const witness_vote_object* find_witness_vote( account_uid_type voter_uid,
                                                       uint32_t         voter_sequence,
                                                       account_uid_type witness_uid,
                                                       uint32_t         witness_sequence )const;

         const committee_member_object& get_committee_member_by_uid( account_uid_type uid )const;
         const committee_member_object* find_committee_member_by_uid( account_uid_type uid )const;

         const committee_member_vote_object* find_committee_member_vote( account_uid_type voter_uid,
                                                                         uint32_t         voter_sequence,
                                                                         account_uid_type committee_member_uid,
                                                                         uint32_t         committee_member_sequence )const;

         const committee_proposal_object& get_committee_proposal_by_number( committee_proposal_number_type number )const;

         const registrar_takeover_object& get_registrar_takeover_object( account_uid_type uid )const;
         const registrar_takeover_object* find_registrar_takeover_object( account_uid_type uid )const;

         const platform_object& get_platform_by_owner( account_uid_type owner )const;
         const platform_object* find_platform_by_sequence( account_uid_type owner, uint32_t sequence )const;
         const platform_object* find_platform_by_owner(account_uid_type owner)const;
         const platform_vote_object* find_platform_vote( account_uid_type voter_uid,
                                                        uint32_t         voter_sequence,
                                                        account_uid_type platform_owner,
                                                        uint32_t         platform_sequence )const;

         const post_object& get_post_by_platform( account_uid_type platform,
                                             account_uid_type poster,
                                             post_pid_type post_pid )const;
         const post_object* find_post_by_platform( account_uid_type platform,
                                              account_uid_type poster,
                                              post_pid_type post_pid )const;

         const license_object& get_license_by_platform(account_uid_type platform, license_lid_type license_lid)const;
         const license_object* find_license_by_platform(account_uid_type platform, license_lid_type license_lid)const;
         const score_object& get_score(account_uid_type platform,
                                       account_uid_type poster,
                                       post_pid_type post_pid,
                                       account_uid_type from_account)const;
         const score_object* find_score(account_uid_type platform,
                                        account_uid_type poster,
                                        post_pid_type post_pid,
                                        account_uid_type from_account)const;

         const advertising_object*  find_advertising(account_uid_type platform, advertising_aid_type advertising_aid)const;
         const advertising_object&  get_advertising(account_uid_type platform, advertising_aid_type advertising_aid)const;

         const advertising_order_object*  find_advertising_order(account_uid_type platform, advertising_aid_type advertising_aid, advertising_order_oid_type order_oid)const;
         const advertising_order_object&  get_advertising_order(account_uid_type platform, advertising_aid_type advertising_aid, advertising_order_oid_type order_oid)const;

         const custom_vote_object& get_custom_vote_by_vid(account_uid_type creator, custom_vote_vid_type vote_vid)const;
         const custom_vote_object* find_custom_vote_by_vid(account_uid_type creator, custom_vote_vid_type vote_vid)const;

         //////////////////// db_init.cpp ////////////////////

         void initialize_evaluators();
         /// Reset the object graph in-memory 
         void initialize_indexes();
         void init_genesis(const genesis_state_type& genesis_state = genesis_state_type());

         template<typename EvaluatorType>
         void register_evaluator()
         {
            _operation_evaluators[
               operation::tag<typename EvaluatorType::operation_type>::value].reset( new op_evaluator_impl<EvaluatorType>() );
         }

         //////////////////// db_balance.cpp ////////////////////

         /**
          * @brief Retrieve a particular account's balance in a given asset
          * @param owner Account whose balance should be retrieved
          * @param asset_id ID of the asset to get balance in
          * @return owner's balance in asset
          */
         asset get_balance(account_uid_type owner, asset_aid_type asset_id)const;
         /// This is an overloaded method.
         asset get_balance(const account_object& owner, const asset_object& asset_obj)const;
         /// This is an overloaded method.
         asset get_balance(const account_object& owner, asset_aid_type asset_id)const;

         /**
          * @brief Adjust a particular account's balance in a given asset by a delta
          * @param account ID of account whose balance should be adjusted
          * @param delta Asset ID and amount to adjust balance by
          */
         void adjust_balance(account_uid_type account, asset delta);
         void adjust_balance(const account_object& account, asset delta);

         // helper to handle witness pay
         void deposit_witness_pay(const witness_object& wit, share_type amount, scheduled_witness_type wit_type);


         //////////////////// db_debug.cpp ////////////////////

         void debug_dump();
         void apply_debug_updates();
         void debug_update( const fc::variant_object& update );

         //////////////////// db_notify.cpp ////////////////////

         //////////////////// db_market.cpp ////////////////////

         /// @{ @group Market Helpers
         void cancel_limit_order(const limit_order_object& order, bool create_virtual_op = true, bool skip_cancel_fee = false);

         /**
         * @brief Process a new limit order through the markets
         * @param order The new order to process
         * @return true if order was completely filled; false otherwise
         *
         * This function takes a new limit order, and runs the markets attempting to match it with existing orders
         * already on the books.
         */
         //bool apply_order_before_hardfork_625(const limit_order_object& new_order_object, bool allow_black_swan = true);
         bool apply_order(const limit_order_object& new_order_object, bool allow_black_swan = true);

         /**
         * Matches the two orders, the first parameter is taker, the second is maker.
         *
         * @return a bit field indicating which orders were filled (and thus removed)
         *
         * 0 - no orders were matched
         * 1 - taker was filled
         * 2 - maker was filled
         * 3 - both were filled
         */
         ///@{
         int match(const limit_order_object& taker, const limit_order_object& maker, const price& trade_price);

         /**
         * @return true if the order was completely filled and thus freed.
         */
         bool fill_limit_order(const limit_order_object& order, const asset& pays, const asset& receives, bool cull_if_small,
             const price& fill_price, const bool is_maker);

         // helpers to fill_order
         void pay_order(const account_object& receiver, const asset& receives, const asset& pays);

         asset calculate_market_fee(const asset_object& recv_asset, const asset& trade_amount);
         asset pay_market_fees(const asset_object& recv_asset, const asset& receives);
         asset pay_market_fees(const account_object& seller, const asset_object& recv_asset, const asset& receives);

		/** Precomputes digests, signatures and operation validations depending
          *  on skip flags. "Expensive" computations may be done in a parallel
          *  thread.
          *
          * @param block the block to preprocess
          * @param skip indicates which computations can be skipped
          * @return a future that will resolve to the input block with
          *         precomputations applied
          */
         fc::future<void> precompute_parallel( const signed_block& block, const uint32_t skip = skip_nothing )const;

         /** Precomputes digests, signatures and operation validations.
          *  "Expensive" computations may be done in a parallel thread.
          *
          * @param trx the transaction to preprocess
          * @return a future that will resolve to the input transaction with
          *         precomputations applied
          */
         fc::future<void> precompute_parallel( const precomputable_transaction& trx )const;
		 fc::future<void> precompute_parallel( const vector<precomputable_transaction>& trxs )const;
      private:
         void notify_changed_objects();
		 
		 template<typename Trx>
         void _precompute_parallel( const Trx* trx, const size_t count, const uint32_t skip )const;

         //////////////////// db_block.cpp ////////////////////

      protected:
         //Mark pop_undo() as protected -- we do not want outside calling pop_undo(); it should call pop_block() instead
         void pop_undo() { object_database::pop_undo(); }

      private:
         optional<undo_database::session>       _pending_tx_session;
         vector< unique_ptr<op_evaluator> >     _operation_evaluators;

         uint32_t                               _check_invariants_interval = uint32_t(-1);
         uint32_t                               _advertising_order_remaining_time = 86400*365;
         uint32_t                               _custom_vote_remaining_time = 86400*365;

         template<class Index>
         vector<std::reference_wrapper<const typename Index::object_type>> sort_votable_objects(size_t count)const;

      public:
	  	 wasm_interface                 wasmif;
         // these were formerly private, but they have a fairly well-defined API, so let's make them public
         void                  apply_block( const signed_block& next_block, uint32_t skip = skip_nothing );
         processed_transaction apply_transaction( const signed_transaction& trx, uint32_t skip = skip_nothing, const vector<operation_result> &operation_results={} );
         operation_result      apply_operation(transaction_evaluation_state& eval_state, const operation& op, const signed_information& sigs = signed_information(),const uint32_t& billed_cpu_time_us = 0);

         void set_check_invariants_interval(uint32_t interval){ _check_invariants_interval = interval; }
         void set_advertising_remain_time(uint32_t time){ _advertising_order_remaining_time = time; }
         void set_custom_vote_remain_time(uint32_t time){ _custom_vote_remaining_time = time; }
         /**
          *  This method validates transactions without adding it to the pending state.
          *  @return true if the transaction would validate
          */
         processed_transaction validate_transaction( const signed_transaction& trx );


// set and get current trx
       public:
         const transaction *get_cur_trx() const { return cur_trx; }
         void set_cur_trx(const transaction *trx) { cur_trx = trx; }

       private:
         const transaction *cur_trx = nullptr;

      private:

         void                  _apply_block( const signed_block& next_block );
         processed_transaction _apply_transaction( const signed_transaction& trx,const vector<operation_result> &operation_results = {});

         ///Steps involved in applying a new block
         ///@{

         const witness_object& validate_block_header( uint32_t skip, const signed_block& next_block )const;
         const witness_object& _validate_block_header( const signed_block& next_block )const;
         void create_block_summary(const signed_block& next_block);

         ///@}

         //////////////////// db_update.cpp ////////////////////
      public:
         void execute_committee_proposal( const committee_proposal_object& proposal, bool silent_fail = false );
         void set_active_post_periods(const uint32_t& periods){ _latest_active_post_periods = periods; }
         uint32_t get_active_post_periods()const{ return _latest_active_post_periods; }
      private:
         void update_global_dynamic_data( const signed_block& b );
         void update_undo_db_size();
         void update_signing_witness(const witness_object& signing_witness, const signed_block& new_block);
         share_type get_witness_pay_by_pledge(const global_property_object& gpo, const dynamic_global_property_object& dpo, const uint16_t by_pledge_witness_count);
         void update_last_irreversible_block();
         void clear_expired_transactions();
         void clear_expired_proposals();
         void clear_active_post();
         void clear_unnecessary_objects();//advertising order, custom vote and cast custom vote
         void update_reduce_witness_csaf();//only execute once for HARDFORK_0_4_BLOCKNUM
         void update_account_permission();
         void update_account_feepoint(); //only execute once for HARDFORK_0_5_BLOCKNUM
         void update_account_reg_info(); //only execute once for HARDFORK_0_5_BLOCKNUM
         void update_core_asset_flags(); //only execute once for HARDFORK_0_5_BLOCKNUM

         std::tuple<set<std::tuple<score_id_type, share_type, bool>>, share_type>
              get_effective_csaf(const active_post_object& active_post);
         void clear_expired_scores();
         void clear_expired_limit_orders();
         void update_maintenance_flag( bool new_maintenance_flag );
         void clear_expired_csaf_leases();
         void update_average_witness_pledges();
         void update_average_platform_pledges();
         void clear_resigned_witness_votes();
         void clear_resigned_committee_member_votes();
         void invalidate_expired_governance_voters();
         void process_invalid_governance_voters();
         void update_voter_effective_votes();
         void adjust_budgets();
         void update_committee();
         void clear_unapproved_committee_proposals();
         void execute_committee_proposals();
         void check_invariants();
         void clear_resigned_platform_votes();
         void process_content_platform_awards();
         void process_platform_voted_awards();
         void process_pledge_balance_release();

         void update_platform_avg_pledge( const account_uid_type uid );
        
      public:
         void resign_pledge_mining(const witness_object& wit);
         void update_platform_avg_pledge( const platform_object& pla );
         void adjust_platform_votes( const platform_object& platform, share_type delta );
         void update_pledge_mining_bonus();
         void update_pledge_mining_bonus_by_witness(const witness_object& witness_obj);
         share_type update_pledge_mining_bonus_by_account(const pledge_mining_object& pledge_mining_obj, share_type bonus_per_pledge);


      public:
         /** when popping a block, the transactions that were removed get cached here so they
          * can be reapplied at the proper time */
         std::deque< signed_transaction >       _popped_tx;

      private:

         vector< processed_transaction >        _pending_tx;
         fork_database                          _fork_db;

         /**
          *  Note: we can probably store blocks by block num rather than
          *  block id because after the undo window is past the block ID
          *  is no longer relevant and its number is irreversible.
          *
          *  During the "fork window" we can cache blocks in memory
          *  until the fork is resolved.  This should make maintaining
          *  the fork tree relatively simple.
          */
         block_database   _block_id_to_block;

         /**
          * Contains the set of ops that are in the process of being applied from
          * the current block.  It contains real and virtual operations in the
          * order they occur and is cleared after the applied_block signal is
          * emited.
          */
         vector<optional<operation_history_object> >  _applied_ops;

         time_point_sec                    _current_block_time;
         uint32_t                          _current_block_num    = 0;
         uint16_t                          _current_trx_in_block = 0;
         uint16_t                          _current_op_in_trx    = 0;
         uint16_t                          _current_virtual_op   = 0;

         flat_map<uint32_t,block_id_type>  _checkpoints;

         // max transaction cpu time, configured by config.ini
         uint32_t                          _max_trx_cpu_time = 10000;

         node_property_object              _node_property_object;

/**
          * Whether database is successfully opened or not.
          *
          * The database is considered open when there's no exception
          * or assertion fail during database::open() method, and
          * database::close() has not been called, or failed during execution.
          */
         bool                              _opened = false;
         bool                              contract_log_to_console = false;
         uint32_t                          _latest_active_post_periods = 10;
 // for inter contract
       public:
          void                     set_contract_transaction_ctx(transaction_context *ctx) { contract_transaction_ctx = ctx; }
          transaction_context*     get_contract_transaction_ctx() const { return contract_transaction_ctx; }
       private:
          transaction_context             *contract_transaction_ctx = nullptr;
   };

   namespace detail
   {
       template<int... Is>
       struct seq { };

       template<int N, int... Is>
       struct gen_seq : gen_seq<N - 1, N - 1, Is...> { };

       template<int... Is>
       struct gen_seq<0, Is...> : seq<Is...> { };

       template<typename T, int... Is>
       void for_each(T&& t, const account_object& a, seq<Is...>)
       {
           auto l = { (std::get<Is>(t)(a), 0)... };
           (void)l;
       }
   }

} }
