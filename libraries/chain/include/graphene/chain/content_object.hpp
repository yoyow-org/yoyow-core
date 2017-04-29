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

         /// The platform's pid.
         platform_pid_type pid = 0;
         /// The owner account's uid.
         account_uid_type owner = 0;
         /// The platform's name.
         string name;
         /// The platform's main url.
         string url;

         string extra_data = "{}";

         time_point_sec create_time;
         time_point_sec last_update_time;

         platform_id_type get_id()const { return id; }
   };


   struct by_pid{};
   struct by_owner{};

   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      platform_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
         ordered_unique< tag<by_pid>, member<platform_object, platform_pid_type, &platform_object::pid> >
      >
   > platform_multi_index_type;

   /**
    * @ingroup object_index
    */
   typedef generic_index<platform_object, platform_multi_index_type> platform_index;

   /**
    * @brief This class represents a content on the object graph
    * @ingroup object
    * @ingroup protocol
    *
    * Contents are posts, replies.
    */
   class content_object : public graphene::db::abstract_object<content_object>
   {
      public:
         static const uint8_t space_id = protocol_ids;
         static const uint8_t type_id  = content_object_type;

         /// The platform's pid.
         platform_pid_type            platform;
         /// The poster's uid.
         account_uid_type             poster;
         /// The content's cid.
         content_cid_type             cid;
         /// The content's parent poster uid if it's not root post.
         optional<account_uid_type>   parent_poster;
         /// The content's parent content cid if it's not root post.
         optional<content_cid_type>   parent_cid;

         post_options                 options;

         string                       hash_value;
         string                       extra_data; ///< category, tags and etc
         string                       title;
         string                       body;

         time_point_sec create_time;
         time_point_sec last_update_time;

         content_id_type get_id()const { return id; }
   };


   struct by_cid{};

   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      content_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
         ordered_unique< tag<by_cid>,
            composite_key<
               content_object,
               member< content_object, platform_pid_type, &content_object::platform >,
               member< content_object, account_uid_type,  &content_object::poster >,
               member< content_object, content_cid_type,  &content_object::cid >
            >
         >
      >
   > content_multi_index_type;

   /**
    * @ingroup object_index
    */
   typedef generic_index<content_object, content_multi_index_type> content_index;

}}

FC_REFLECT_DERIVED( graphene::chain::platform_object,
                    (graphene::db::object),
                    (pid)(owner)(name)(url)(extra_data)
                    (create_time)(last_update_time)
                  )

FC_REFLECT_DERIVED( graphene::chain::content_object,
                    (graphene::db::object),
                    (platform)(poster)(cid)(parent_poster)(parent_cid)
                    (options)
                    (hash_value)(extra_data)(title)(body)
                    (create_time)(last_update_time)
                  )


