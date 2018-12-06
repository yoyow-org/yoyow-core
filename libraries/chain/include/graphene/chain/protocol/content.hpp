/*
 * Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
 */
#pragma once
#include <graphene/chain/protocol/base.hpp>

namespace graphene { namespace chain {

   /**
    * @brief Create a platform on the web and pay everyone for it
    * @ingroup operations
    *
    * Anyone can use this operation to create a platform object
    */
   struct platform_create_operation : public base_operation
   {
      struct fee_parameters_type
      {
         uint64_t fee              = 1000 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee     = 1000 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint16_t min_rf_percent   = 10000;
         uint32_t price_per_kbyte  = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
         extensions_type   extensions;
      };

      // Fee
      fee_type          fee;
      /// Account with platform. This account pays for this operation.
      account_uid_type  account;
      // Mortgage amount
      asset             pledge;
      // name
      string            name;
      // Platform main domain name
      string            url;
      // Other information (json format string, API interface, other URL, platform introduction, etc.)
      string            extra_data = "{}";
      extensions_type   extensions;

      account_uid_type  fee_payer_uid()const { return account; }
      void              validate()const;
      share_type        calculate_fee(const fee_parameters_type& k)const;
      void get_required_active_uid_authorities( flat_set<account_uid_type>& a )const
      {
         // Necessary balance of authority
         a.insert( account );
      }
   };

   /**
    * @brief Update platform related information
    * @ingroup operations
    */
   struct platform_update_operation : public base_operation
   {
      struct fee_parameters_type
      {
         uint64_t fee              = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee     = 0;
         uint16_t min_rf_percent   = 0;
         uint32_t price_per_kbyte  = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
         extensions_type   extensions;
      };

      fee_type          fee;
      /// Platform owner account
      account_uid_type  account;
      /// New mortgage amount
      optional<asset>   new_pledge;
      // New name
      optional<string>  new_name;
      /// New domain name
      optional<string>  new_url;
      // New additional information
      optional<string>  new_extra_data;

      extensions_type   extensions;

      account_uid_type  fee_payer_uid()const { return account; }
      void              validate()const;
      share_type      calculate_fee(const fee_parameters_type& k)const;
      void get_required_active_uid_authorities( flat_set<account_uid_type>& a )const
      {
         // Necessary balance of authority
         a.insert( account );
      }
   };

   /**
    * @brief Change or refresh platform voting status.
    * @ingroup operations
    */
   struct platform_vote_update_operation : public base_operation
   {
      struct fee_parameters_type
      {
         uint64_t basic_fee             = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t price_per_platform    = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee          = 0;
         uint16_t min_rf_percent        = 0;
         extensions_type   extensions;
      };
      
      // fee
      fee_type                   fee;
      /// Voter, as the account to pay the cost of voting operations
      account_uid_type           voter;
      // Add a voting platform list
      flat_set<account_uid_type> platform_to_add;
      // Remove the voting platform list
      flat_set<account_uid_type> platform_to_remove;

      extensions_type   extensions;

      account_uid_type  fee_payer_uid()const { return voter; }
      void              validate()const;
      share_type        calculate_fee(const fee_parameters_type& k)const;
      void get_required_active_uid_authorities( flat_set<account_uid_type>& a )const
      {
         // Necessary balance of authority
         a.insert( voter );
      }
   };

   /**
    * @ingroup operations
    *
    * @brief Post an article or a reply
    *
    *  Fees are paid by the "poster" account
    *
    *  @return n/a
    */
   struct Recerptor_Parameter
   {
	   int8_t      cur_ratio;
	   bool        to_buyout;
	   int8_t      buyout_ratio;
	   share_type  buyout_price;

	   void validate()const
	   {
		   if (to_buyout)
			   FC_ASSERT(buyout_ratio <= cur_ratio, "forward_ratio must be less then cur_ratio");
		   FC_ASSERT(cur_ratio <= (10000 - GRAPHENE_DEFAULT_PLATFORM_RECERPTS_RATIO), "forward_ratio must be less then 75%");
	   }
   };

   struct post_operation : public base_operation
   {
	   struct ext
	   {
		   optional<share_type> forward_price;
		   optional< map<account_uid_type, Recerptor_Parameter> > receiptors;
	   };
	   typedef static_variant< ext > extension_parameter;

      struct fee_parameters_type {
         uint64_t fee              = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint32_t price_per_kbyte  = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee     = 0;
         uint16_t min_rf_percent   = 0;
         extensions_type   extensions;
      };

      fee_type                     fee;

      /// The post's pid.
      post_pid_type                post_pid;
      account_uid_type             platform = 0;
      account_uid_type             poster = 0;
      optional<account_uid_type>   origin_poster;
      optional<post_pid_type>      origin_post_pid;
      optional<account_uid_type>   origin_platform;

      string                       hash_value;
      string                       extra_data = "{}"; ///< category, tags and etc
      string                       title;
      string                       body;

	  optional< flat_set<extension_parameter> > extensions;
      //extensions_type              extensions;

      account_uid_type fee_payer_uid()const { return poster; }
      void             validate()const;
      share_type       calculate_fee(const fee_parameters_type& k)const;
      void get_required_secondary_uid_authorities( flat_set<account_uid_type>& a )const
      {
         a.insert( poster );    // Requires authors to change the permissions
         a.insert( platform );  // Requires platform to change the permissions
      }
   };

   /**
    * @ingroup operations
    *
    * @brief update an article
    *
    *  Fees are paid by the "poster" account
    *
    *  @return n/a
    */
   struct post_update_operation : public base_operation
   {
	   struct ext
	   {
		   optional<share_type>        forward_price;
		   optional<account_uid_type>  receiptor;
		   optional<bool>              to_buyout;
		   optional<int8_t>            buyout_ratio;
		   optional<share_type>        buyout_price;
	   };
	   typedef static_variant< ext > extension_parameter;

      struct fee_parameters_type {
         uint64_t fee              = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint32_t price_per_kbyte  = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee     = 0;
         uint16_t min_rf_percent   = 0;
         extensions_type   extensions;
      };

      fee_type                     fee;

      account_uid_type             platform;
      account_uid_type             poster;
      post_pid_type                post_pid;

      optional< string >           hash_value;
      optional< string >           extra_data; ///< category, tags and etc
      optional< string >           title;
      optional< string >           body;

	  optional< flat_set<extension_parameter> > extensions;
      //extensions_type              extensions;

      account_uid_type fee_payer_uid()const { return poster; }
      void            validate()const;
      share_type      calculate_fee(const fee_parameters_type& k)const;
      void get_required_secondary_uid_authorities( flat_set<account_uid_type>& a )const
      {
		  a.insert(platform);  // Requires platform to change the permissions
		  if (hash_value.valid() || extra_data.valid() || title.valid() || body.valid())
			  a.insert(poster);    // Requires authors to change the permissions
		  if (extensions.valid())
		  {
			  for (auto ext_iter = extensions->begin(); ext_iter != extensions->end(); ext_iter++)
			  {
				  if (ext_iter->which() == post_update_operation::extension_parameter::tag<post_update_operation::ext>::value)
				  {
					  const post_update_operation::ext& ext = ext_iter->get<post_update_operation::ext>();
					  if (ext.forward_price.valid() && a.find(poster) == a.end())
					      a.insert(poster);
					  if (ext.receiptor.valid() && a.find(*(ext.receiptor)) == a.end())
						  a.insert(*(ext.receiptor));
				  }
			  }
		  }
      }
   };

   /**
   * @ingroup operations
   *
   * @brief Score an article or a reply
   *
   *  Fees are paid by the "from_account_uid" account
   *
   *  @return n/a
   */
   struct score_create_operation : public base_operation
   {
	   struct fee_parameters_type {
		   uint64_t fee = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
		   uint64_t min_real_fee = 0;
		   uint16_t min_rf_percent = 0;
		   extensions_type   extensions;
	   };

	   fee_type                     fee;

	   account_uid_type             from_account_uid;//from account
	   post_pid_type                post_pid; //post id
	   int8_t                       score;
	   int64_t                      csaf;

	   extensions_type              extensions;

	   account_uid_type fee_payer_uid()const { return from_account_uid; }
	   void             validate()const;
	   share_type       calculate_fee(const fee_parameters_type& k)const;
	   void get_required_secondary_uid_authorities(flat_set<account_uid_type>& a)const
	   {
		   a.insert(from_account_uid);    // Requires authors to change the permissions
	   }
   };

   /**
   * @ingroup operations
   *
   * @brief Score an article or a reply
   *
   *  Fees are paid by the "from_account_uid" account
   *
   *  @return n/a
   */
   struct reward_operation : public base_operation
   {
	   struct fee_parameters_type {
		   uint64_t fee = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
		   uint64_t min_real_fee = 0;
		   uint16_t min_rf_percent = 0;
		   extensions_type   extensions;
	   };

	   fee_type                     fee;

	   account_uid_type             from_account_uid;//from account
	   post_pid_type                post_pid; //post id
	   asset                        amount;

	   extensions_type              extensions;

	   account_uid_type fee_payer_uid()const { return from_account_uid; }
	   void             validate()const;
	   share_type       calculate_fee(const fee_parameters_type& k)const;
	   void get_required_secondary_uid_authorities(flat_set<account_uid_type>& a)const
	   {
		   a.insert(from_account_uid);    // Requires authors to change the permissions
	   }
   };

}} // graphene::chain

FC_REFLECT( graphene::chain::platform_create_operation::fee_parameters_type, (fee)(min_real_fee)(min_rf_percent)(price_per_kbyte)(extensions) )
FC_REFLECT(graphene::chain::platform_create_operation, (fee)(account)(pledge)(name)(url)(extra_data)(extensions) )

FC_REFLECT( graphene::chain::platform_update_operation::fee_parameters_type, (fee)(min_real_fee)(min_rf_percent)(price_per_kbyte)(extensions) )
FC_REFLECT(graphene::chain::platform_update_operation, (fee)(account)(new_pledge)(new_name)(new_url)(new_extra_data)(extensions) )

FC_REFLECT( graphene::chain::platform_vote_update_operation::fee_parameters_type, (basic_fee)(price_per_platform)(min_real_fee)(min_rf_percent)(extensions) )
FC_REFLECT(graphene::chain::platform_vote_update_operation, (fee)(voter)(platform_to_add)(platform_to_remove)(extensions) )

FC_REFLECT( graphene::chain::post_operation::fee_parameters_type, (fee)(price_per_kbyte)(min_real_fee)(min_rf_percent)(extensions) )
FC_REFLECT( graphene::chain::post_operation,
            (fee)
            (post_pid)(platform)(poster)(origin_poster)(origin_post_pid)(origin_platform)
            (hash_value)(extra_data)(title)(body)
            (extensions) )

FC_REFLECT( graphene::chain::post_update_operation::fee_parameters_type, (fee)(price_per_kbyte)(min_real_fee)(min_rf_percent)(extensions) )
FC_REFLECT( graphene::chain::post_update_operation,
            (fee)
            (platform)(poster)(post_pid)
            (hash_value)(extra_data)(title)(body)
            (extensions) )

FC_REFLECT(graphene::chain::score_create_operation::fee_parameters_type, (fee)(min_real_fee)(min_rf_percent)(extensions))
FC_REFLECT(graphene::chain::score_create_operation, (fee)(from_account_uid)(post_pid)(score)(csaf)(extensions))

FC_REFLECT(graphene::chain::reward_operation::fee_parameters_type, (fee)(min_real_fee)(min_rf_percent)(extensions))
FC_REFLECT(graphene::chain::reward_operation, (fee)(from_account_uid)(post_pid)(amount)(extensions))

FC_REFLECT(graphene::chain::post_operation::ext, (forward_price)(receiptors))
FC_REFLECT(graphene::chain::post_update_operation::ext, (forward_price)(receiptor)(to_buyout)(buyout_ratio)(buyout_price))

FC_REFLECT(graphene::chain::Recerptor_Parameter, (cur_ratio)(to_buyout)(buyout_ratio)(buyout_price))