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
         //序号（当前账号第几次创建平台）
         uint32_t sequence;

         //是否有效（“无效”为短暂的中间状态） 
         bool is_valid = true;
         //获得票数
         uint64_t total_votes = 0;
         /// The platform's main url.
         string url;

         uint64_t pledge;
         
         //其他信息（API接口，其他URL，平台介绍等）
         string extra_data = "{}";

         time_point_sec create_time;
         time_point_sec last_update_time;

         platform_id_type get_id()const { return id; }
   };


   struct by_owner{};
   struct by_valid{};

   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      platform_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
         ordered_unique< tag<by_owner>, member<platform_object, account_uid_type, &platform_object::owner> >,
         ordered_unique< tag<by_valid>,
            composite_key<
               platform_object,
               member<platform_object, bool, &platform_object::is_valid>,
               member<platform_object, account_uid_type, &platform_object::owner>,
               member<platform_object, uint32_t, &platform_object::sequence>
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
         account_uid_type    platform_uid = 0;
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
               member< platform_vote_object, account_uid_type, &platform_vote_object::platform_uid>,
               member< platform_vote_object, uint32_t, &platform_vote_object::platform_sequence>
            >
         >,
         ordered_unique< tag<by_platform_owner_seq>,
            composite_key<
               platform_vote_object,
               member< platform_vote_object, account_uid_type, &platform_vote_object::platform_uid>,
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
         account_uid_type            platform;
         /// The poster's uid.
         account_uid_type             poster;
         /// The post's pid.
         post_pid_type                post_pid;
         /// The post's parent poster uid if it's not root post.
         optional<account_uid_type>   parent_poster;
         /// The post's parent post pid if it's not root post.
         optional<post_pid_type>      parent_post_pid;

         post_options                 options;

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
   struct by_parent_create_time{};

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

         >,
         ordered_unique< tag<by_parent_create_time>,
            composite_key<
               post_object,
               member< post_object, account_uid_type,           &post_object::platform >,
               member< post_object, optional<account_uid_type>,  &post_object::parent_poster >,
               member< post_object, optional<post_pid_type>,     &post_object::parent_post_pid >,
               member< post_object, time_point_sec,              &post_object::create_time >,
               member< object,      object_id_type,              &post_object::id>
            >,
            composite_key_compare< std::less<account_uid_type>,
                                   std::less<optional<account_uid_type>>,
                                   std::less<optional<post_pid_type>>,
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
                    (owner)(name)(sequence)(is_valid)(total_votes)(url)(extra_data)
                    (create_time)(last_update_time)
                  )

FC_REFLECT_DERIVED( graphene::chain::platform_vote_object, (graphene::db::object),
                    (voter_uid)
                    (voter_sequence)
                    (platform_uid)
                    (platform_sequence)
                  )

FC_REFLECT_DERIVED( graphene::chain::post_object,
                    (graphene::db::object),
                    (platform)(poster)(post_pid)(parent_poster)(parent_post_pid)
                    (options)
                    (hash_value)(extra_data)(title)(body)
                    (create_time)(last_update_time)
                  )


