/*
 * Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
 */
#pragma once
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/memo.hpp>

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
      void get_required_active_uid_authorities( flat_set<account_uid_type>& a,bool enabled_hardfork )const
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
      void get_required_active_uid_authorities( flat_set<account_uid_type>& a,bool enabled_hardfork )const
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
      void get_required_active_uid_authorities( flat_set<account_uid_type>& a,bool enabled_hardfork )const
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
   class Receiptor_Parameter
   {
   public:
	   uint16_t          cur_ratio;        //the receiptor`s current ratio of the post
	   bool              to_buyout;        //is to sell receiptor`s ratio
	   uint16_t          buyout_ratio = 0; //the ratio to sell
	   share_type        buyout_price = 0; //the price of the ratio for sell
       time_point_sec    buyout_expiration = time_point_sec::maximum(); //the expiration time for receiptor`s sell order
       extensions_type   extensions;


       Receiptor_Parameter(){}

       Receiptor_Parameter(uint16_t       cur_ratio_,
                           bool           to_buyout_,
                           uint16_t       buyout_ratio_,
                           share_type     buyout_price_,
                           time_point_sec buyout_expiration_ = time_point_sec::maximum()) :
        cur_ratio(cur_ratio_), to_buyout(to_buyout_), buyout_ratio(buyout_ratio_), buyout_price(buyout_price_), buyout_expiration(buyout_expiration_)
       {
       }

	   void validate()const
	   {
           if (to_buyout){
               FC_ASSERT(buyout_price > 0, "if buyout, buyout_price must be > 0. ");
               FC_ASSERT(buyout_ratio > 0, "if buyout, buyout_ratio must be > 0. ");
               FC_ASSERT(buyout_ratio <= cur_ratio, "buyout_ratio must be less than cur_ratio");
           }
           else{
               FC_ASSERT(buyout_price == 0, "if not to buyout, buyout_price must be == 0. ");
               FC_ASSERT(buyout_ratio == 0, "if not to buyout, buyout_ratio must be == 0. ");
           }
	   }

       bool operator == (const Receiptor_Parameter& r1) const
       {
           return (cur_ratio == r1.cur_ratio) &&
                  (to_buyout == r1.to_buyout) && 
                  (buyout_price == r1.buyout_price) && 
                  (buyout_ratio == r1.buyout_ratio) &&
                  (buyout_expiration == r1.buyout_expiration);
       }
   };

   struct post_operation : public base_operation
   {
	   enum Post_Type
	   {
		   Post_Type_Post = 0,
		   Post_Type_Comment = 1,
		   Post_Type_forward = 2,
		   Post_Type_forward_And_Modify = 3,

		   Post_Type_Default
	   };

	   struct ext
	   {
           optional<uint8_t>                                      post_type = Post_Type_Post; // post`s type
		   optional<share_type>                                   forward_price;              // the price of forward this post
           optional<license_lid_type>                             license_lid;                // the license`s id of this post
           optional<uint32_t>                                     permission_flags = 0xFF;    // permissions of this post
           optional<map<account_uid_type, Receiptor_Parameter> >  receiptors;                 // map of receiptor`s parameters
           optional<account_uid_type>                             sign_platform;              // sign by platform account
	   };

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

      optional< extension<post_operation::ext> > extensions;

      account_uid_type fee_payer_uid()const { return poster; }
      void             validate()const;
      share_type       calculate_fee(const fee_parameters_type& k)const;
      void get_required_secondary_uid_authorities( flat_set<account_uid_type>& a ,bool enabled_hardfork)const
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
           optional<share_type>           forward_price;     //the post`s forward price for update
           optional<account_uid_type>     receiptor;         //the receiptor account for update his parameter
           optional<bool>                 to_buyout;         //buyout or not the receiptor`s ratio
           optional<uint16_t>             buyout_ratio;      //if buyout, the buyout ratio of the receiptor for sell
           optional<share_type>           buyout_price;      //buyout price for sell the buyout ratio
           optional<time_point_sec>       buyout_expiration; //the expiration time for this buyout order
           optional<license_lid_type>     license_lid;       //the post`s license`s id for update
           optional<uint32_t>             permission_flags;  //the post`s permissions for update
           optional<account_uid_type>     content_sign_platform;   // sign by platform account
           optional<account_uid_type>     receiptor_sign_platform; // sign by platform account
	   };

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

      optional< extension<post_update_operation::ext> > extensions;

      account_uid_type fee_payer_uid()const { return poster; }
      void            validate()const;
      share_type      calculate_fee(const fee_parameters_type& k)const;
      void get_required_secondary_uid_authorities( flat_set<account_uid_type>& a,bool enabled_hardfork )const
      {
          if (hash_value.valid() || extra_data.valid() || title.valid() || body.valid()){
              a.insert(platform);  // Requires platform to change the permissions
              a.insert(poster);    // Requires authors to change the permissions
          }
			  
		  if (extensions.valid())
		  {
              const post_update_operation::ext& ext = extensions->value;
              if (ext.forward_price.valid() || ext.permission_flags.valid() || ext.license_lid.valid()){
                  a.insert(platform);
                  a.insert(poster);
              }
              if (ext.receiptor.valid() )
                  a.insert(*(ext.receiptor));
		  }
          else{
              a.insert(poster); 
              a.insert(platform); 
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
		   uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION/100;
		   uint32_t price_per_kbyte = 0;
		   uint64_t min_real_fee = 0;
		   uint16_t min_rf_percent = 0;
		   extensions_type   extensions;
	   };

	   fee_type                     fee;

	   account_uid_type             from_account_uid; //from account`s uid
	   account_uid_type             platform;         //platform account`s uid
	   account_uid_type             poster;           //poster account`s uid
	   post_pid_type                post_pid;         //post`s pid
	   int8_t                       score;            //the score for post. range [-5,5]
       share_type                   csaf;             //the integration of yoyow for post
       optional<account_uid_type>   sign_platform;    // sign by platform account

	   extensions_type              extensions;

	   account_uid_type fee_payer_uid()const { return from_account_uid; }
	   void             validate()const;
	   share_type       calculate_fee(const fee_parameters_type& k)const;
	   void get_required_secondary_uid_authorities(flat_set<account_uid_type>& a,bool enabled_hardfork)const
	   {
           a.insert(from_account_uid);  // Requires platform to change the permissions
	   }
   };

   /**
   * @ingroup operations
   *
   * @brief rewoard an article or a reply
   *
   *  Fees are paid by the "from_account_uid" account
   *
   *  @return n/a
   */
   struct reward_operation : public base_operation
   {
	   struct fee_parameters_type {
		   uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION/10;
		   uint32_t price_per_kbyte = 0;
		   uint64_t min_real_fee = 0;
		   uint16_t min_rf_percent = 0;
		   extensions_type   extensions;
	   };

	   fee_type                     fee;

	   account_uid_type             from_account_uid; //from account`s uid
	   account_uid_type             platform;         //platform account`s uid
	   account_uid_type             poster;           //poster account`s uid
	   post_pid_type                post_pid;         //post`s pid
	   asset                        amount;           //the asset reward for the post

	   extensions_type              extensions;

	   account_uid_type fee_payer_uid()const { return from_account_uid; }
	   void             validate()const;
	   share_type       calculate_fee(const fee_parameters_type& k)const;
       void get_required_active_uid_authorities(flat_set<account_uid_type>& a,bool enabled_hardfork)const
	   {
		   a.insert(from_account_uid);    // Requires authors to change the permissions
	   }
   };

   /**
   * @ingroup operations
   *
   * @brief rewoard an article or a reply proxy by platform
   *
   *  Fees are paid by the "from_account_uid" account
   *
   *  @return n/a
   */
   struct reward_proxy_operation : public base_operation
   {
       struct fee_parameters_type {
           uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION/10;
           uint32_t price_per_kbyte = 0;
           uint64_t min_real_fee = 0;
           uint16_t min_rf_percent = 0;
           extensions_type   extensions;
       };

       fee_type                     fee;

       account_uid_type             from_account_uid;  //from account`s uid
       account_uid_type             platform;          //platform account`s uid
       account_uid_type             poster;            //poster account`s uid
       post_pid_type                post_pid;          //post`s pid
       share_type                   amount;            //amount of YOYO reward for the post proxy by platform
       optional<account_uid_type>   sign_platform;     // sign by platform account

       extensions_type              extensions;

       account_uid_type fee_payer_uid()const { return from_account_uid; }
       void             validate()const;
       share_type       calculate_fee(const fee_parameters_type& k)const;
       void get_required_secondary_uid_authorities(flat_set<account_uid_type>& a,bool enabled_hardfork)const
       {
           a.insert(from_account_uid);    // Requires authors to change the permissions
           a.insert(platform);  // Requires platform to change the permissions
       }
   };


   /**
   * @ingroup operations
   *
   * @brief buyout an article`s profit
   *
   *  Fees are paid by the "from_account_uid" account
   *
   *  @return n/a
   */
   struct buyout_operation : public base_operation
   {
	   struct fee_parameters_type {
		   uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION/10;
		   uint32_t price_per_kbyte = 0;
		   uint64_t min_real_fee = 0;
		   uint16_t min_rf_percent = 0;
		   extensions_type   extensions;
	   };

	   fee_type                     fee;

	   account_uid_type             from_account_uid;      //from account`s uid
	   account_uid_type             platform;              //platform account`s uid
	   account_uid_type             poster;                //poster account`s uid
	   post_pid_type                post_pid;              //post`s pid
	   account_uid_type             receiptor_account_uid; //the receiptor account`s uid. to buy the receiptor`s sell order.
       optional<account_uid_type>   sign_platform;         // sign by platform account

	   extensions_type              extensions;

	   account_uid_type fee_payer_uid()const { return from_account_uid; }
	   void             validate()const;
	   share_type       calculate_fee(const fee_parameters_type& k)const;
       void get_required_secondary_uid_authorities(flat_set<account_uid_type>& a,bool enabled_hardfork)const
	   {
		   a.insert(from_account_uid);    // Requires authors to change the permissions
	   }
   };

   /**
   * @ingroup operations
   *
   * @brief create license
   *
   *  Fees are paid by the "platform" account
   *
   *  @return n/a
   */
   struct license_create_operation : public base_operation
   {
       struct fee_parameters_type {
           uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION/10;
           uint32_t price_per_kbyte = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
           uint64_t min_real_fee = 0;
           uint16_t min_rf_percent = 0;
           extensions_type   extensions;
       };

       fee_type                     fee;

       license_lid_type             license_lid;       //the license`s lid
       account_uid_type             platform = 0;      //the platform account who create this license
       uint8_t                      type;              //the type of the license
       string                       hash_value;        //license`s hash
       string                       extra_data = "{}"; //license`s extra datas
       string                       title;             //the title of the license
       string                       body;              //the body of the license

       extensions_type              extensions;

       account_uid_type fee_payer_uid()const { return platform; }
       void             validate()const;
       share_type       calculate_fee(const fee_parameters_type& k)const;
       void get_required_active_uid_authorities(flat_set<account_uid_type>& a,bool enabled_hardfork)const
       {
           a.insert(platform);    // Requires platform to change the permissions
       }
   };
}} // graphene::chain

FC_REFLECT( graphene::chain::platform_create_operation::fee_parameters_type, (fee)(min_real_fee)(min_rf_percent)(price_per_kbyte)(extensions) )
FC_REFLECT(graphene::chain::platform_create_operation, (fee)(account)(pledge)(name)(url)(extra_data)(extensions) )

FC_REFLECT( graphene::chain::platform_update_operation::fee_parameters_type, (fee)(min_real_fee)(min_rf_percent)(price_per_kbyte)(extensions) )
FC_REFLECT(graphene::chain::platform_update_operation, (fee)(account)(new_pledge)(new_name)(new_url)(new_extra_data)(extensions) )

FC_REFLECT( graphene::chain::platform_vote_update_operation::fee_parameters_type, (basic_fee)(price_per_platform)(min_real_fee)(min_rf_percent)(extensions) )
FC_REFLECT(graphene::chain::platform_vote_update_operation, (fee)(voter)(platform_to_add)(platform_to_remove)(extensions) )

FC_REFLECT(graphene::chain::post_operation::ext, (post_type)(forward_price)(license_lid)(permission_flags)(receiptors)(sign_platform))
FC_REFLECT(graphene::chain::post_update_operation::ext, (forward_price)(receiptor)(to_buyout)(buyout_ratio)(buyout_price)(buyout_expiration)
                                                        (license_lid)(permission_flags)(content_sign_platform)(receiptor_sign_platform))

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

FC_REFLECT(graphene::chain::score_create_operation::fee_parameters_type, (fee)(price_per_kbyte)(min_real_fee)(min_rf_percent)(extensions))
FC_REFLECT(graphene::chain::score_create_operation, (fee)(from_account_uid)(platform)(poster)(post_pid)(score)(csaf)(sign_platform)(extensions))

FC_REFLECT(graphene::chain::reward_operation::fee_parameters_type, (fee)(price_per_kbyte)(min_real_fee)(min_rf_percent)(extensions))
FC_REFLECT(graphene::chain::reward_operation, (fee)(from_account_uid)(platform)(poster)(post_pid)(amount)(extensions))

FC_REFLECT(graphene::chain::reward_proxy_operation::fee_parameters_type, (fee)(price_per_kbyte)(min_real_fee)(min_rf_percent)(extensions))
FC_REFLECT(graphene::chain::reward_proxy_operation, (fee)(from_account_uid)(platform)(poster)(post_pid)(amount)(sign_platform)(extensions))

FC_REFLECT(graphene::chain::Receiptor_Parameter, (cur_ratio)(to_buyout)(buyout_ratio)(buyout_price)(buyout_expiration)(extensions))

FC_REFLECT(graphene::chain::buyout_operation::fee_parameters_type, (fee)(price_per_kbyte)(min_real_fee)(min_rf_percent)(extensions))
FC_REFLECT(graphene::chain::buyout_operation, (fee)(from_account_uid)(platform)(poster)(post_pid)(receiptor_account_uid)(sign_platform)(extensions))

FC_REFLECT(graphene::chain::license_create_operation::fee_parameters_type, (fee)(price_per_kbyte)(min_real_fee)(min_rf_percent)(extensions))
FC_REFLECT(graphene::chain::license_create_operation, (fee)(license_lid)(platform)(type)(hash_value)(extra_data)(title)(body)(extensions))