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

         // Other information (api interface address, other URL, platform introduction, etc.)
         string extra_data = "{}";

         time_point_sec create_time;
         time_point_sec last_update_time;

         platform_id_type get_id()const { return id; }
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

         post_id_type get_id()const { return id; }
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
               member< post_object, account_uid_type, &post_object::platform >,
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

}}

FC_REFLECT_DERIVED( graphene::chain::platform_object,
                    (graphene::db::object),
                    (owner)(name)(sequence)(is_valid)(total_votes)(url)
                    (pledge)(pledge_last_update)(average_pledge)(average_pledge_last_update)(average_pledge_next_update_block)
                    (extra_data)
                    (create_time)(last_update_time)
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
                    (create_time)(last_update_time)
                  )


