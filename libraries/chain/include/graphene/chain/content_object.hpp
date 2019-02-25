/*
 * Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
 */
#pragma once
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace graphene { namespace chain {
   class database;

   /**
    * @brief This class represents a content platform on the object graph
    * @ingroup object
    * @ingroup protocol
    *
    * Content platforms are where the contents will be created.
    */
   class platform_object : public graphene::db::abstract_object<platform_object>
   {
      public:
         static const uint8_t space_id = protocol_ids;
         static const uint8_t type_id  = platform_object_type;

         struct Platform_Period_Profits
         {
             flat_map<asset_aid_type, share_type>   rewards_profits;
             share_type                             foward_profits    = 0;
             share_type                             post_profits      = 0;
             share_type                             platform_profits  = 0;
         };
         
         /// The owner account's uid.
         account_uid_type owner = 0;
         /// The platform's name.
         string name;
         // Serial number (current account number of times to create a platform)
         uint32_t sequence;

         // Is valid ("invalid" for short intermediate state)
         bool is_valid = true;
         // Get the votes
         uint64_t total_votes = 0;
         /// The platform's main url.
         string url;

         uint64_t pledge = 0;
         fc::time_point_sec  pledge_last_update;
         uint64_t            average_pledge = 0;
         fc::time_point_sec  average_pledge_last_update;
         uint32_t            average_pledge_next_update_block;

         map<time_point_sec, share_type>        vote_profits;
         map<uint32_t, Platform_Period_Profits> period_profits;

         // Other information (api interface address, other URL, platform introduction, etc.)
         string extra_data = "{}";

         time_point_sec create_time;
         time_point_sec last_update_time;

         platform_id_type get_id()const { return id; }
         void add_period_profits(uint32_t   period,
                                 uint32_t   lastest_periods,
                                 asset      reward_profit = asset(),
                                 share_type forward_profit = 0,
                                 share_type post_profit = 0,
                                 share_type platform_profit = 0
                                 )
         {
             auto iter = period_profits.find(period);
             if (iter != period_profits.end())
             {
                 if (reward_profit != asset()){
                     auto iter_reward = iter->second.rewards_profits.find(reward_profit.asset_id);
                     if (iter_reward != iter->second.rewards_profits.end())
                         iter_reward->second += reward_profit.amount;
                     else
                         iter->second.rewards_profits.emplace(reward_profit.asset_id, reward_profit.amount);
                 }
                 iter->second.foward_profits   += forward_profit;
                 iter->second.post_profits     += post_profit;
                 iter->second.platform_profits += platform_profit;
             }
             else
             {
                 if (period_profits.size() >= lastest_periods)
                     period_profits.erase(period_profits.begin());
                 Platform_Period_Profits profits;
                 if (reward_profit != asset())
                     profits.rewards_profits.emplace(reward_profit.asset_id, reward_profit.amount);
                 profits.foward_profits   = forward_profit;
                 profits.post_profits     = post_profit;
                 profits.platform_profits = platform_profit;
                 period_profits.emplace(period, profits);
             }
         }
   };


   struct by_owner{};
   struct by_valid{};
   struct by_platform_pledge;
   struct by_platform_votes;

   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      platform_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
         ordered_unique< tag<by_owner>,
            composite_key<
               platform_object,
               member<platform_object, account_uid_type, &platform_object::owner>,
               member<platform_object, uint32_t, &platform_object::sequence>
            >
         >,
         ordered_unique< tag<by_valid>,
            composite_key<
               platform_object,
               member<platform_object, bool, &platform_object::is_valid>,
               member<platform_object, account_uid_type, &platform_object::owner>,
               member<platform_object, uint32_t, &platform_object::sequence>
            >
         >,
         ordered_unique< tag<by_platform_votes>, // for API
            composite_key<
               platform_object,
               member<platform_object, bool, &platform_object::is_valid>,
               member<platform_object, uint64_t, &platform_object::total_votes>,
               member<platform_object, account_uid_type, &platform_object::owner>,
               member<platform_object, uint32_t, &platform_object::sequence>
            >,
            composite_key_compare<
               std::less< bool >,
               std::greater< uint64_t >,
               std::less< account_uid_type >,
               std::less< uint32_t >
            >
         >,
         ordered_unique< tag<by_platform_pledge>, // for API
            composite_key<
               platform_object,
               member<platform_object, bool, &platform_object::is_valid>,
               member<platform_object, uint64_t, &platform_object::pledge>,
               member<platform_object, account_uid_type, &platform_object::owner>,
               member<platform_object, uint32_t, &platform_object::sequence>
            >,
            composite_key_compare<
               std::less< bool >,
               std::greater< uint64_t >,
               std::less< account_uid_type >,
               std::less< uint32_t >
            >
         >
      >
   > platform_multi_index_type;

   /**
    * @ingroup object_index
    */
   typedef generic_index<platform_object, platform_multi_index_type> platform_index;

   /**
    * @brief This class represents a platform voting on the object graph
    * @ingroup object
    * @ingroup protocol
    */
   class platform_vote_object : public graphene::db::abstract_object<platform_vote_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_platform_vote_object_type;

         account_uid_type    voter_uid = 0;
         uint32_t            voter_sequence;
         account_uid_type    platform_owner = 0;
         uint32_t            platform_sequence;
   };

   struct by_platform_voter_seq{};
   struct by_platform_owner_seq{};

   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      platform_vote_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
         ordered_unique< tag<by_platform_voter_seq>,
            composite_key<
               platform_vote_object,
               member< platform_vote_object, account_uid_type, &platform_vote_object::voter_uid>,
               member< platform_vote_object, uint32_t, &platform_vote_object::voter_sequence>,
               member< platform_vote_object, account_uid_type, &platform_vote_object::platform_owner>,
               member< platform_vote_object, uint32_t, &platform_vote_object::platform_sequence>
            >
         >,
         ordered_unique< tag<by_platform_owner_seq>,
            composite_key<
               platform_vote_object,
               member< platform_vote_object, account_uid_type, &platform_vote_object::platform_owner>,
               member< platform_vote_object, uint32_t, &platform_vote_object::platform_sequence>,
               member< platform_vote_object, account_uid_type, &platform_vote_object::voter_uid>,
               member< platform_vote_object, uint32_t, &platform_vote_object::voter_sequence>
            >
         >
      >
   > platform_vote_multi_index_type;

    /**
    * @ingroup object_index
    */
   typedef generic_index<platform_vote_object, platform_vote_multi_index_type> platform_vote_index;

   /**
    * @brief This class represents a post on the object graph
    * @ingroup object
    * @ingroup protocol
    *
    * Contents are posts, replies.
    */
   class post_object : public graphene::db::abstract_object<post_object>
   {
      public:
          enum Post_Permission
          {
              Post_Permission_Forward = 1,   //allow forward 
              Post_Permission_Liked   = 2,   //allow liked or scored
              Post_Permission_Buyout  = 4,   //allow buyout
              Post_Permission_Comment = 8,   //allow comment
              Post_Permission_Reward  = 16   //allow reward
          };

         static const uint8_t space_id = protocol_ids;
         static const uint8_t type_id  = post_object_type;

         /// The platform's pid.
         account_uid_type             platform;
         /// The poster's uid.
         account_uid_type             poster;
         /// The post's pid.
         post_pid_type                post_pid;
         /// If it is a transcript, this value is requested as the source author uid
         optional<account_uid_type>   origin_poster;
         /// If it is a transcript, this value is required for the source id
         optional<post_pid_type>      origin_post_pid;
         /// If it is a transcript, this value is required for the source platform
         optional<account_uid_type>   origin_platform;

         string                       hash_value;
         string                       extra_data; ///< category, tags and etc
         string                       title;
         string                       body;

         time_point_sec create_time;
         time_point_sec last_update_time;

		 map<account_uid_type, Recerptor_Parameter> receiptors; //receiptors of the post
		 optional<share_type>                       forward_price;
         optional<license_lid_type>                 license_lid;
         uint32_t                                   permission_flags = 0xFFFFFFFF;
         bool                                       score_settlement = false;

         post_id_type get_id()const { return id; }
		 void receiptors_validate()const
		 {
			 auto itor = receiptors.find(platform);
			 FC_ASSERT(itor != receiptors.end(), "platform must be included by receiptors");
			 FC_ASSERT(itor->second.cur_ratio == GRAPHENE_DEFAULT_PLATFORM_RECERPTS_RATIO, "platform`s ratio must be 30%");  
			 uint32_t total = 0;
			 for (auto iter : receiptors)
			 {
                 FC_ASSERT(iter.second.cur_ratio <= (GRAPHENE_100_PERCENT - GRAPHENE_DEFAULT_PLATFORM_RECERPTS_RATIO), "The cur_ratio of receiptor should less then 70%");
				 total += iter.second.cur_ratio;
				 if (iter.second.to_buyout)
					 FC_ASSERT(iter.second.cur_ratio >= iter.second.buyout_ratio, "buyout_ratio must less then cur_ratio");
			 }
             FC_ASSERT(total == GRAPHENE_100_PERCENT, "The sum of receiptors` ratio must be 100%");
		 }
   };


   struct by_post_pid{};
   struct by_platform_create_time{};
   struct by_platform_poster_create_time{};

   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      post_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
         ordered_unique< tag<by_post_pid>,
            composite_key<
               post_object,
               member< post_object, account_uid_type,  &post_object::platform >,
               member< post_object, account_uid_type,  &post_object::poster >,
               member< post_object, post_pid_type,     &post_object::post_pid >
            >
         >,
         // TODO move non-consensus indexes to plugin
         ordered_unique< tag<by_platform_create_time>,
            composite_key<
               post_object,
               member< post_object, account_uid_type, &post_object::platform >,
               member< post_object, time_point_sec,    &post_object::create_time >,
               member< object,      object_id_type,    &post_object::id>
            >,
            composite_key_compare< std::less<account_uid_type>,
                                   std::greater<time_point_sec>,
                                   std::greater<object_id_type> >

         >,
         ordered_unique< tag<by_platform_poster_create_time>,
            composite_key<
               post_object,
               member< post_object, account_uid_type, &post_object::platform >,
               member< post_object, account_uid_type,  &post_object::poster >,
               member< post_object, time_point_sec,    &post_object::create_time >,
               member< object,      object_id_type,    &post_object::id>
            >,
            composite_key_compare< std::less<account_uid_type>,
                                   std::less<account_uid_type>,
                                   std::greater<time_point_sec>,
                                   std::greater<object_id_type> >

         >
      >
   > post_multi_index_type;

   /**
    * @ingroup object_index
    */
   typedef generic_index<post_object, post_multi_index_type> post_index;

	 /**
	 * @brief This class record rewards and approvals of a post
	 * @ingroup object
	 * @ingroup protocol
	 *
	 */
	 class active_post_object : public graphene::db::abstract_object < active_post_object >
	 {
	 public:
		 static const uint8_t space_id	= protocol_ids;
		 static const uint8_t type_id		= active_post_object_type;

         struct receiptor_detail
         {
            share_type  forward;
            share_type  post_award;
            flat_map<asset_aid_type, share_type> rewards;   
         };

		 /// The platform's pid.
		 account_uid_type                       platform;
		 /// The poster's uid.
		 account_uid_type                       poster;
		 /// The post's pid.
		 post_pid_type                          post_pid;
		 /// detail information of approvals, csaf.
		 vector<score_id_type>                  scores;
		 /// approvals of a post, csaf.
		 share_type                             total_csaf;
		 /// rewards of a post.
		 flat_map<asset_aid_type, share_type>   total_rewards;
		 /// period sequence of a post.
		 uint64_t                               period_sequence;

     bool                                   positive_win;
     share_type                             post_award;
     share_type                             forward_award;
     flat_map<account_uid_type, receiptor_detail> receiptor_details;

     void insert_receiptor(account_uid_type uid, share_type post_award = 0, share_type forward = 0)
     {
        if (receiptor_details.count(uid))
        {
           receiptor_details.at(uid).forward += forward;
           receiptor_details.at(uid).post_award += post_award;
        }
        else
        {
           receiptor_detail detail;
           detail.forward = forward;
           detail.post_award = post_award;
           receiptor_details.emplace(uid, detail);
        }
     }
     void insert_receiptor(account_uid_type uid, asset reward = asset())
     {
        if (receiptor_details.count(uid))
        {
           if (receiptor_details.at(uid).rewards.count(reward.asset_id))
              receiptor_details.at(uid).rewards.at(reward.asset_id) += reward.amount;
           else
              receiptor_details.at(uid).rewards.emplace(reward.asset_id, reward.amount);
        }
        else
        {
           receiptor_detail detail;
           detail.rewards.emplace(reward.asset_id, reward.amount);
           receiptor_details.emplace(uid, detail);
        }
     }
	 };

   struct by_poster{};
   struct by_platforms{};
   struct by_period_sequence{};
   struct by_post{};

	 /**
	 * @ingroup object_index
	 */
	 typedef multi_index_container <
		 active_post_object,
		 indexed_by<
				ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
				ordered_unique< tag<by_post_pid>,
				   composite_key<
					    active_post_object,
					    member< active_post_object, account_uid_type, &active_post_object::platform >,
					    member< active_post_object, account_uid_type, &active_post_object::poster >,				    
                        member< active_post_object, uint64_t,         &active_post_object::period_sequence >,
                        member< active_post_object, post_pid_type,    &active_post_object::post_pid >
				   >
				>,
                ordered_non_unique< tag<by_post>,
				   composite_key<
					    active_post_object,
					    member< active_post_object, account_uid_type, &active_post_object::platform >,
					    member< active_post_object, account_uid_type, &active_post_object::poster >,
                        member< active_post_object, post_pid_type,    &active_post_object::post_pid >,
                        member< active_post_object, uint64_t,         &active_post_object::period_sequence >
				   >
				>,
        ordered_non_unique< tag<by_poster>,
            composite_key<
              active_post_object,
              member< active_post_object, account_uid_type, &active_post_object::poster >,
              member< active_post_object, uint64_t,         &active_post_object::period_sequence >
           >
        >,
        ordered_non_unique< tag<by_platforms>,
            composite_key<
              active_post_object,
              member< active_post_object, account_uid_type, &active_post_object::platform >,
              member< active_post_object, uint64_t,         &active_post_object::period_sequence >
           >
        >,
				ordered_non_unique< tag<by_period_sequence>, member<active_post_object, uint64_t, &active_post_object::period_sequence> >
		 >
	 > active_post_multi_index_type;

	 /**
	 * @ingroup object_index
	 */
	 typedef generic_index<active_post_object, active_post_multi_index_type> active_post_index;

   /**
   * @brief This class represents scores for a post
   * @ingroup object
   * @ingroup protocol
   */
   class score_object : public graphene::db::abstract_object<score_object>
   {
   public:
	   static const uint8_t space_id = implementation_ids;
	   static const uint8_t type_id = impl_score_object_type;

	   account_uid_type    from_account_uid;
       account_uid_type    platform;
       account_uid_type    poster;
	   post_pid_type       post_pid;
	   int8_t              score;
	   share_type          csaf;
       uint64_t            period_sequence;
       share_type          profits;

	   time_point_sec      create_time;
   };

   struct by_from_account_uid{};
   //struct by_post_pid{};
   struct by_create_time{};
   struct by_posts_pids{};
   //struct by_period_sequence{};

   /**
   * @ingroup object_index
   */
   typedef multi_index_container<
	   score_object,
	   indexed_by<
	      ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
		  ordered_non_unique< tag<by_from_account_uid>, 
                                 composite_key< 
                                     score_object,
                                     member< score_object, account_uid_type, &score_object::from_account_uid >,
                                     member< score_object, uint64_t,         &score_object::period_sequence >>
                                 >,
          ordered_unique< tag<by_post_pid>, 
                              composite_key<
                                  score_object,
                                  member< score_object, account_uid_type, &score_object::platform >,
                                  member< score_object, account_uid_type, &score_object::poster >,
                                  member< score_object, post_pid_type,    &score_object::post_pid >,
                                  member< score_object, account_uid_type, &score_object::from_account_uid >
                              > >,
          ordered_non_unique< tag<by_posts_pids>, 
                              composite_key<
                                  score_object,
                                  member< score_object, account_uid_type, &score_object::platform >,
                                  member< score_object, account_uid_type, &score_object::poster >,
                                  member< score_object, post_pid_type,    &score_object::post_pid >
                              > >,
          ordered_non_unique< tag<by_period_sequence>, 
                              composite_key<
                                  score_object,
                                  member< score_object, account_uid_type, &score_object::platform >,
                                  member< score_object, account_uid_type, &score_object::poster >,
                                  member< score_object, post_pid_type,    &score_object::post_pid >,
                                  member< score_object, uint64_t,         &score_object::period_sequence>
                              > >,
		  ordered_non_unique< tag<by_create_time>,member< score_object, time_point_sec, &score_object::create_time> >
       >
   > score_multi_index_type;

   /**
   * @ingroup object_index
   */
   typedef generic_index<score_object, score_multi_index_type> score_index;

   /**
   * @brief This class represents scores for a post
   * @ingroup object
   * @ingroup protocol
   */
   class license_object : public graphene::db::abstract_object<license_object>
   {
   public:
       static const uint8_t space_id = implementation_ids;
       static const uint8_t type_id = impl_license_object_type;

       license_lid_type             license_lid;
       account_uid_type             platform;
       uint8_t                      license_type;
	   
	   string                       hash_value;
	   string                       extra_data;
	   string                       title;
	   string                       body;

	   time_point_sec               create_time;
   };

   struct by_license_lid{};
   struct by_platform{};
   struct by_license_type{};

   /**
   * @ingroup object_index
   */
   typedef multi_index_container<
       license_object,
	   indexed_by<
	   ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
       ordered_unique< tag<by_license_lid>,
	                   composite_key<
                                   license_object,
                                   member< license_object, account_uid_type, &license_object::platform >,
                                   member< license_object, license_lid_type, &license_object::license_lid >
								   >> ,
       ordered_non_unique< tag<by_platform>, member< license_object, account_uid_type, &license_object::platform> >,
       ordered_non_unique< tag<by_license_type>, member< license_object, uint8_t, &license_object::license_type> >
	   >
    > license_multi_index_type;

   /**
   * @ingroup object_index
   */
   typedef generic_index<license_object, license_multi_index_type> license_index;


   /**
   * @brief This class advertising space
   * @ingroup object
   * @ingroup protocol
   */
   class advertising_object : public graphene::db::abstract_object<advertising_object>
   {
   public:
      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id = impl_license_object_type;

      account_uid_type     platform;
      account_uid_type     user;
      time_point_sec       publish_time;
      share_type           sell_price;
      time_point_sec       start_time;
      time_point_sec       end_time;
      uint8_t              state = advertising_free;
      share_type           released_balance;
      
      string               description;
   };

   /**
   * @ingroup object_index
   */
   typedef multi_index_container<
      advertising_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >>
      > advertising_multi_index_type;

   /**
   * @ingroup object_index
   */
   typedef generic_index<advertising_object, advertising_multi_index_type> advertising_index;

}}

FC_REFLECT( graphene::chain::platform_object::Platform_Period_Profits,
                    (rewards_profits)(foward_profits)(post_profits)(platform_profits))

FC_REFLECT_DERIVED( graphene::chain::platform_object,
                    (graphene::db::object),
                    (owner)(name)(sequence)(is_valid)(total_votes)(url)
                    (pledge)(pledge_last_update)(average_pledge)(average_pledge_last_update)(average_pledge_next_update_block)
                    (extra_data)
                    (create_time)(last_update_time)
                    (vote_profits)(period_profits)
                  )

FC_REFLECT_DERIVED( graphene::chain::platform_vote_object, (graphene::db::object),
                    (voter_uid)
                    (voter_sequence)
                    (platform_owner)
                    (platform_sequence)
                  )

FC_REFLECT_DERIVED( graphene::chain::post_object,
                    (graphene::db::object),
                    (platform)(poster)(post_pid)(origin_poster)(origin_post_pid)(origin_platform)
                    (hash_value)(extra_data)(title)(body)
                    (create_time)(last_update_time)(receiptors)(forward_price)(license_lid)(permission_flags)(score_settlement)
                  )

FC_REFLECT( graphene::chain::active_post_object::receiptor_detail,
                   (forward)(post_award)(rewards))

FC_REFLECT_DERIVED( graphene::chain::active_post_object,
										(graphene::db::object),
                                        (platform)(poster)(post_pid)(scores)(total_csaf)(total_rewards)(period_sequence)
                    (positive_win)(post_award)(forward_award)(receiptor_details)
									)

FC_REFLECT_DERIVED(graphene::chain::score_object,
					(graphene::db::object),
                    (from_account_uid)(platform)(poster)(post_pid)(score)(csaf)(profits)(create_time)
					)

FC_REFLECT_DERIVED(graphene::chain::license_object,
                    (graphene::db::object), (license_lid)(platform)(license_type)(hash_value)(extra_data)(title)(body)(create_time)
					)

FC_REFLECT_DERIVED( graphene::chain::advertising_object,
                   (graphene::db::object), 
                   (platform)(user)(publish_time)(sell_price)(start_time)(end_time)(state)(released_balance)(description)
          )