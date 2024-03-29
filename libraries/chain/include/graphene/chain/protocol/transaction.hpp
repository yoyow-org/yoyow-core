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
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/chain/protocol/types.hpp>

#include <numeric>

namespace graphene { namespace chain {

   /**
    * @defgroup transactions Transactions
    *
    * All transactions are sets of operations that must be applied atomically. Transactions must refer to a recent
    * block that defines the context of the operation so that they assert a known binding to the object id's referenced
    * in the transaction.
    *
    * Rather than specify a full block number, we only specify the lower 16 bits of the block number which means you
    * can reference any block within the last 65,536 blocks which is 3.5 days with a 5 second block interval or 18
    * hours with a 1 second interval.
    *
    * All transactions must expire so that the network does not have to maintain a permanent record of all transactions
    * ever published.  A transaction may not have an expiration date too far in the future because this would require
    * keeping too much transaction history in memory.
    *
    * The block prefix is the first 4 bytes of the block hash of the reference block number, which is the second 4
    * bytes of the @ref block_id_type (the first 4 bytes of the block ID are the block number)
    *
    * Note: A transaction which selects a reference block cannot be migrated between forks outside the period of
    * ref_block_num.time to (ref_block_num.time + rel_exp * interval). This fact can be used to protect market orders
    * which should specify a relatively short re-org window of perhaps less than 1 minute. Normal payments should
    * probably have a longer re-org window to ensure their transaction can still go through in the event of a momentary
    * disruption in service.
    *
    * @note It is not recommended to set the @ref ref_block_num, @ref ref_block_prefix, and @ref expiration
    * fields manually. Call the appropriate overload of @ref set_expiration instead.
    *
    * @{
    */

   /**
    *  @brief groups operations that should be applied atomically
    */
   class transaction
   {
   public:
      virtual ~transaction() = default;
      /**
       * Least significant 16 bits from the reference block number. If @ref relative_expiration is zero, this field
       * must be zero as well.
       */
      uint16_t           ref_block_num    = 0;
      /**
       * The first non-block-number 32-bits of the reference block ID. Recall that block IDs have 32 bits of block
       * number followed by the actual block hash, so this field should be set using the second 32 bits in the
       * @ref block_id_type
       */
      uint32_t           ref_block_prefix = 0;

      /**
       * This field specifies the absolute expiration for this transaction.
       */
      fc::time_point_sec expiration;

      vector<operation>  operations;
      extensions_type    extensions;

      /// Calculate the digest for a transaction
      digest_type         digest()const;
      virtual const transaction_id_type& id()const;
      virtual void                       validate() const;

      void set_expiration( fc::time_point_sec expiration_time );
      void set_reference_block( const block_id_type& reference_block );

      /// visit all operations
      template<typename Visitor>
      vector<typename Visitor::result_type> visit( Visitor&& visitor )
      {
         vector<typename Visitor::result_type> results;
         for( auto& op : operations )
            results.push_back(op.visit( std::forward<Visitor>( visitor ) ));
         return results;
      }
      template<typename Visitor>
      vector<typename Visitor::result_type> visit( Visitor&& visitor )const
      {
         vector<typename Visitor::result_type> results;
         for( auto& op : operations )
            results.push_back(op.visit( std::forward<Visitor>( visitor ) ));
         return results;
      }

      void get_required_uid_authorities( flat_set<account_uid_type>& owner_uids,
                                         flat_set<account_uid_type>& active_uids,
                                         flat_set<account_uid_type>& secondary_uids,
                                         vector<authority>& other,
                                         bool enabled_hardfork)const;

	virtual uint64_t get_packed_size()const;

   protected:
      // Calculate the digest used for signature validation
      digest_type sig_digest( const chain_id_type& chain_id )const;
      mutable transaction_id_type _tx_id_buffer;
   
   };
   
   //signed_information should send to operation_apply for authority check
   struct signed_information
   {
      struct sign_tree
      {
         account_uid_type           uid = 0;
         flat_set<public_key_type>  pub_keys;
         flat_set<sign_tree>        children;

         bool operator < (const sign_tree& a)const {
            return uid < a.uid;
         }
         sign_tree(const account_uid_type& id = 0) :uid(id){}
      };

      flat_map<account_uid_type, sign_tree>  owner;
      flat_map<account_uid_type, sign_tree>  active;
      flat_map<account_uid_type, sign_tree>  secondary;

      account_uid_type real_owner_uid(account_uid_type uid, uint32_t depth)const
      {
         if (owner.find(uid) == owner.end())
            return account_uid_type(0);

         sign_tree root = owner.at(uid);
         while (root.pub_keys.empty() && root.children.size() == 1 && depth)
         {
            root = *(root.children.begin());
            --depth;
         }
         return root.uid;
      }

      account_uid_type real_active_uid(account_uid_type uid, uint32_t depth)const
      {
         if (active.find(uid) == active.end())
            return account_uid_type(0);

         sign_tree root = active.at(uid);
         while (root.pub_keys.empty() && root.children.size() == 1 && depth)
         {
            root = *(root.children.begin());
            --depth;
         }
         return root.uid;
      }

      account_uid_type real_secondary_uid(account_uid_type uid, uint32_t depth)const
      {
         if (secondary.find(uid) == secondary.end())
            return account_uid_type(0);

         sign_tree root = secondary.at(uid);
         while (root.pub_keys.empty() && root.children.size() == 1 && depth)
         {
            root = *(root.children.begin());
            --depth;
         }
         return root.uid;
      }

   };

   /**
    *  @brief adds a signature to a transaction
    */
   class signed_transaction : public transaction
   {
   public:
      signed_transaction( const transaction& trx = transaction() )
         : transaction(trx){}
      virtual ~signed_transaction() = default;

      /** signs and appends to signatures */
      const signature_type& sign( const private_key_type& key, const chain_id_type& chain_id );

      /** returns signature but does not append */
      signature_type sign( const private_key_type& key, const chain_id_type& chain_id )const;

      /**
       *  The purpose of this method is to identify some subset of
       *  @ref available_keys that will produce sufficient signatures
       *  for a transaction.  The result is not always a minimal set of
       *  signatures, but any non-minimal result will still pass
       *  validation.
       *
       *  @return a tuple, the first set contains a usable subset,
       *          the second set contains more potential keys required,
       *          the third set contains redundant signatures need to be removed.
       */
      std::tuple<flat_set<public_key_type>,flat_set<public_key_type>,flat_set<signature_type>> get_required_signatures(
         const chain_id_type& chain_id,
         const flat_set<public_key_type>& available_keys,
         const std::function<const authority*(account_uid_type)>& get_owner_by_uid,
         const std::function<const authority*(account_uid_type)>& get_active_by_uid,
         const std::function<const authority*(account_uid_type)>& get_secondary_by_uid,
         bool	  enabled_hardfork,
         uint32_t max_recursion = GRAPHENE_MAX_SIG_CHECK_DEPTH
         )const;

      signed_information verify_authority(
         const chain_id_type& chain_id,
         const std::function<const authority*(account_uid_type)>& get_owner_by_uid,
         const std::function<const authority*(account_uid_type)>& get_active_by_uid,
         const std::function<const authority*(account_uid_type)>& get_secondary_by_uid,
         bool	  enabled_hardfork,
         uint32_t max_recursion = GRAPHENE_MAX_SIG_CHECK_DEPTH )const;

      /**
       * This is a slower replacement for get_required_signatures()
       * which returns a minimal set in all cases, including
       * some cases where get_required_signatures() returns a
       * non-minimal set.
       */
      /*
      set<public_key_type> minimize_required_signatures(
         const chain_id_type& chain_id,
         const flat_set<public_key_type>& available_keys,
         const std::function<const authority*(account_id_type)>& get_active,
         const std::function<const authority*(account_id_type)>& get_owner,
         uint32_t max_recursion = GRAPHENE_MAX_SIG_CHECK_DEPTH
         ) const;
      */

      virtual const flat_map<public_key_type,signature_type>& get_signature_keys( const chain_id_type& chain_id )const;

      vector<signature_type> signatures;

      /// Removes all operations and signatures
      void clear() { operations.clear(); signatures.clear(); }

      /** Removes all signatures */
      void clear_signatures() { signatures.clear(); }
   protected:
      /** Public keys extracted from signatures */
      mutable flat_map<public_key_type,signature_type> _signees;
   };
   
   /** This represents a signed transaction that will never have its operations,
    *  signatures etc. modified again, after initial creation. It is therefore
    *  safe to cache results from various calls.
    */
   class precomputable_transaction : public signed_transaction {
   public:
      precomputable_transaction() {}
      precomputable_transaction( const signed_transaction& tx ) : signed_transaction(tx) {};
      precomputable_transaction( signed_transaction&& tx ) : signed_transaction( std::move(tx) ) {};
      virtual ~precomputable_transaction() = default;

      virtual const transaction_id_type&       id()const override;
      virtual void                             validate()const override;
      virtual const flat_map<public_key_type,signature_type>& get_signature_keys( const chain_id_type& chain_id )const override;
      virtual uint64_t                         get_packed_size()const override;
   protected:
      mutable bool _validated = false;
      mutable uint64_t _packed_size = 0;
   };

signed_information verify_authority(const vector<operation>& ops, const flat_map<public_key_type, signature_type>& sigs,
                       const std::function<const authority*(account_uid_type)>& get_owner_by_uid,
                       const std::function<const authority*(account_uid_type)>& get_active_by_uid,
                       const std::function<const authority*(account_uid_type)>& get_secondary_by_uid,
                       bool	  enabled_hardfork,
                       uint32_t max_recursion_depth = GRAPHENE_MAX_SIG_CHECK_DEPTH,
                       bool allow_committe = false,
                       const flat_set<account_uid_type>& owner_uid_approvals = flat_set<account_uid_type>(),
                       const flat_set<account_uid_type>& active_uid_aprovals = flat_set<account_uid_type>(),
                       const flat_set<account_uid_type>& secondary_uid_approvals = flat_set<account_uid_type>());

void get_authority_uid( const account_uid_type uid,
                        const std::function<const account_object*(account_uid_type)>& get_acc_by_uid,
                        flat_set<account_uid_type>& owner_auth_uid,
                        flat_set<account_uid_type>& active_auth_uid,
                        flat_set<account_uid_type>& secondary_auth_uid
                        );

void get_authority_uid( const authority* au,
                        const std::function<const account_object*(account_uid_type)>& get_acc_by_uid,
                        flat_set<account_uid_type>& owner_auth_uid,
                        flat_set<account_uid_type>& active_auth_uid,
                        flat_set<account_uid_type>& secondary_auth_uid,
                        uint32_t depth = 0);

   /**
    *  @brief captures the result of evaluating the operations contained in the transaction
    *
    *  When processing a transaction some operations generate
    *  new object IDs and these IDs cannot be known until the
    *  transaction is actually included into a block.  When a
    *  block is produced these new ids are captured and included
    *  with every transaction.  The index in operation_results should
    *  correspond to the same index in operations.
    *
    *  If an operation did not create any new object IDs then 0
    *  should be returned.
    */
   struct processed_transaction : public precomputable_transaction
   {
      processed_transaction( const signed_transaction& trx = signed_transaction() )
         : precomputable_transaction(trx){}
      virtual ~processed_transaction() = default;

      vector<operation_result> operation_results;

      digest_type merkle_digest()const;
   };

   /// @} transactions group

} } // graphene::chain

FC_REFLECT( graphene::chain::transaction, (ref_block_num)(ref_block_prefix)(expiration)(operations)(extensions) )
FC_REFLECT_DERIVED( graphene::chain::signed_transaction, (graphene::chain::transaction), (signatures) )
FC_REFLECT_DERIVED( graphene::chain::precomputable_transaction, (graphene::chain::signed_transaction), )
FC_REFLECT_DERIVED( graphene::chain::processed_transaction, (graphene::chain::precomputable_transaction), (operation_results) )
