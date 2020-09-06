#pragma once

#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/multi_index_includes.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/db/generic_index.hpp>
#include <graphene/db/object.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <softfloat.hpp>

#include <array>
#include <type_traits>

namespace graphene { namespace chain {

//typedef __uint128_t uint128_t;

class table_id_object : public graphene::db::abstract_object<table_id_object>
{
  public:
      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id  = impl_table_id_object_type;

      account_name            code;
      scope_name              scope;
      table_name              table;
      account_name            payer;
      uint32_t                count = 0; /// the number of elements in the table
};

struct by_code_scope_table;

using table_id_multi_index_type = multi_index_container<
  table_id_object,
  indexed_by<
      ordered_unique< tag<by_id>,
        member< object, object_id_type, &object::id >
      >,
      ordered_unique<tag<by_code_scope_table>,
        composite_key< table_id_object,
           member<table_id_object, account_name, &table_id_object::code>,
           member<table_id_object, scope_name,   &table_id_object::scope>,
           member<table_id_object, table_name,   &table_id_object::table>
        >
     >
  >
>;

typedef generic_index<table_id_object, table_id_multi_index_type> table_id_multi_index;

struct by_scope_primary;
struct by_scope_secondary;
struct by_scope_tertiary;

typedef table_id_object_id_type table_id;
class key_value_object : public graphene::db::abstract_object<key_value_object>
{
  public:
    static const uint8_t space_id = implementation_ids;
    static const uint8_t type_id = impl_key_value_object_type;

    typedef uint64_t key_type;
    static const int number_of_keys = 1;

    table_id                    t_id;
    uint64_t                    primary_key;
    account_name                payer = 0;
    bytes                       value;
};

using key_value_multi_index_type = multi_index_container<
  key_value_object,
  indexed_by<
     ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
     ordered_unique<tag<by_scope_primary>,
        composite_key< key_value_object,
           member<key_value_object, table_id, &key_value_object::t_id>,
           member<key_value_object, uint64_t, &key_value_object::primary_key>
        >,
        composite_key_compare< std::less<table_id>, std::less<uint64_t> >
     >
  >
>;
typedef generic_index<key_value_object, key_value_multi_index_type> key_value_index;

struct by_primary;
struct by_secondary;

template <typename SecondaryKey, uint64_t ObjectTypeId, typename SecondaryKeyLess = std::less<SecondaryKey>>
struct secondary_index {
    class index_object : public graphene::db::abstract_object<index_object> {
    public:
      typedef SecondaryKey secondary_key_type;

      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id  = ObjectTypeId;

      table_id      t_id;
      uint64_t      primary_key;
      account_name  payer = 0;
      SecondaryKey  secondary_key;
    };


    using index_multi_index_type =  multi_index_container<
        index_object,
        indexed_by<
            ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
            ordered_unique<tag<by_primary>,
               composite_key<index_object,
                 member<index_object, table_id, &index_object::t_id>,
                 member<index_object, uint64_t, &index_object::primary_key>>,
               composite_key_compare<std::less<table_id>, std::less<uint64_t>>
            >,
            ordered_unique<tag<by_secondary>,
               composite_key<index_object,
                 member<index_object, table_id, &index_object::t_id>,
                 member<index_object, SecondaryKey, &index_object::secondary_key>,
                 member<index_object, uint64_t, &index_object::primary_key>
               >,
               composite_key_compare<std::less<table_id>, SecondaryKeyLess, std::less<uint64_t>>
            >
        >
    >;
    typedef generic_index<index_object, index_multi_index_type> index_index;

};

typedef secondary_index<uint64_t, index64_object_type>::index_object index64_object;
typedef secondary_index<uint64_t, index64_object_type>::index_index index64_index;

} }  // namespace graphene::chain

template<typename T>
struct get_gph_index_type {};

#define GPH_SET_INDEX_TYPE(OBJECT_TYPE, INDEX_TYPE)  \
    template<> struct get_gph_index_type<OBJECT_TYPE> { typedef INDEX_TYPE type; };

GPH_SET_INDEX_TYPE(graphene::chain::index64_object, graphene::chain::index64_index)

FC_REFLECT_DERIVED(graphene::chain::table_id_object, (graphene::db::object),
                   (code)
                   (scope)
                   (table)
                   (payer)
                   (count))

FC_REFLECT_DERIVED(graphene::chain::key_value_object, (graphene::db::object),
                  (t_id)
                  (primary_key)
                  (payer)
                  (value))

FC_REFLECT_DERIVED(graphene::chain::index64_object, (graphene::db::object),
                  (t_id)
                  (primary_key)
                  (payer)
                  (secondary_key))
