#include <algorithm>
#include <graphene/chain/apply_context.hpp>
#include <graphene/chain/transaction_context.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/wasm_interface.hpp>
#include <graphene/chain/wast_to_wasm.hpp>
#include <graphene/chain/contract_table_objects.hpp>

#include <boost/container/flat_set.hpp>

using boost::container::flat_set;

namespace graphene { namespace chain {

void apply_context::exec_one()
{
    if(contract_log_to_console) {
        auto prefix = fc::format_string(
                                        "[(${a},${n})->${r}] ",
                                        fc::mutable_variant_object()
                                        ("a", act.contract_id)
                                        ("n", std::string(act.method_name))
                                        ("r", receiver)
                                        );

        dlog(prefix + "CONSOLE OUTPUT BEGIN =====================");
    }

    auto start = fc::time_point::now();
    try {
        account_uid_type contract_id = (account_uid_type)(receiver & GRAPHENE_DB_MAX_INSTANCE_ID);
        auto &contract_obj = db().get_account_by_uid(contract_id);
        auto wasm_bytes = bytes(contract_obj.code.begin(), contract_obj.code.end());
        try {
            wasm_interface &wasm = const_cast<wasm_interface &>(_db->wasmif);
            digest_type code_version{contract_obj.code_version};
            wasm.apply(code_version, wasm_bytes, *this);
        } catch (const wasm_exit &) {
        }
   } FC_CAPTURE_AND_RETHROW((_pending_console_output.str()));

   if(contract_log_to_console) {
       auto prefix = fc::format_string(
                                       "[(${a},${n})->${r}] ",
                                       fc::mutable_variant_object()
                                       ("a", act.contract_id)
                                       ("n", std::string(act.method_name))
                                       ("r", receiver)
                                       );

       dlog(prefix + "CONSOLE OUTPUT END =====================" );
   }
   reset_console();
   auto end = fc::time_point::now();
   dlog("[(${a},${n})->${r}] elapsed ${e}", ("a", act.contract_id)("n", std::string(act.method_name))("r", receiver)("e", end - start));
}

void apply_context::exec()
{
    // adjust balance
    if (amount.valid()) {
        // amount always >= 0
        auto amnt = *amount;
        db().adjust_balance(account_uid_type(act.sender), -amnt);
        db().adjust_balance(account_uid_type(act.contract_id), amnt);
    }

    exec_one();

    for (const auto &inline_operation : _inline_operations) {
        trx_context.dispatch_operation(inline_operation);
    }
}

void apply_context::reset_console()
{
    if (contract_log_to_console) {
        _pending_console_output = std::ostringstream();
        _pending_console_output.setf(std::ios::scientific, std::ios::floatfield);
    }
}

void apply_context::execute_inline(inter_contract_call_operation &&op)
{
    _inline_operations.emplace_back(std::move(op));
}

void apply_context::check_payer_permission(account_name& payer)
{
    FC_ASSERT(0 == payer || payer == sender || payer == trx_context.get_trx_origin() || payer == receiver,
              "db access violation, apayer ${p} not in {0, sender ${s}, origin ${o}, receiver ${r}}",
              ("p", payer)("s", sender)("o", trx_context.get_trx_origin())("r", receiver));
    if (payer == 0) {
        payer = receiver;
    }
}

int apply_context::db_store_i64(uint64_t scope, uint64_t table, account_name payer, uint64_t id, const char *buffer, size_t buffer_size)
{
    check_payer_permission(payer);
    return db_store_i64(receiver, scope, table, payer, id, buffer, buffer_size);
}

int apply_context::db_store_i64(uint64_t code, uint64_t scope, uint64_t table, account_name payer, uint64_t id, const char *buffer, size_t buffer_size)
{
    check_payer_permission(payer);

    const auto &tab = find_or_create_table(code, scope, table, payer);
    auto tableid = tab.id;

    const auto& new_obj = _db->create<key_value_object>([&](key_value_object& o) {
        o.t_id = tableid;
        o.primary_key = id;
        o.value.resize(buffer_size);
        o.payer = payer;
        memcpy(o.value.data(), buffer, buffer_size);
    });

    // update_db_usage
    int64_t ram_delta = (int64_t)(buffer_size + config::billable_size_v<key_value_object>);
    trx_context.update_ram_statistics(payer, ram_delta);
    _db->modify(tab, [&](table_id_object& t) {
      ++t.count;
    });
   

    keyval_cache.cache_table(tab);
    return keyval_cache.add(new_obj);
}

void apply_context::db_update_i64(int iterator, account_name payer, const char *buffer, size_t buffer_size)
{
    check_payer_permission(payer);

    const key_value_object &obj = keyval_cache.get(iterator);

    // validate
    const auto &table_obj = keyval_cache.get_table(obj.t_id);
    FC_ASSERT(table_obj.code == receiver, "db access violation");

    // update db usage
    const int64_t overhead = config::billable_size_v<key_value_object>;
    int64_t old_size = (int64_t)(obj.value.size() + overhead);
    int64_t new_size = (int64_t)(buffer_size + overhead);
    if (obj.payer != payer) {
        // refund the existing payer
        trx_context.update_ram_statistics(obj.payer,  -(old_size));
        // charge the new payer
        trx_context.update_ram_statistics(payer, new_size);
    } else if (old_size != new_size) {
        // charge/refund the existing payer the difference
        trx_context.update_ram_statistics(obj.payer, new_size - old_size);
    }

    _db->modify(obj, [&](key_value_object &o) {
        o.value.resize(buffer_size);
        memcpy(o.value.data(), buffer, buffer_size);
        o.payer = payer;
    });
}

void apply_context::db_remove_i64(int iterator)
{
    const key_value_object &obj = keyval_cache.get(iterator);

    const auto &table_obj = keyval_cache.get_table(obj.t_id);
    FC_ASSERT(table_obj.code == receiver, "db access violation");

    int64_t ram_delta = -(int64_t)(obj.value.size() + config::billable_size_v<key_value_object>);
   
    trx_context.update_ram_statistics(obj.payer, ram_delta);

    _db->modify(table_obj, [&](table_id_object& t) {
       --t.count;
    });
    

    _db->remove(obj);
    keyval_cache.remove(iterator);
}

int apply_context::db_get_i64(int iterator, char *buffer, size_t buffer_size)
{
    const key_value_object &obj = keyval_cache.get(iterator);

    auto s = obj.value.size();
    if (buffer_size == 0) return s;

    auto copy_size = std::min(buffer_size, s);
    memcpy(buffer, obj.value.data(), copy_size);

    return copy_size;
}

int apply_context::db_next_i64(int iterator, uint64_t &primary)
{
    if (iterator < -1) return -1; // cannot increment past end iterator of table

    const auto &obj = keyval_cache.get(iterator); // Check for iterator != -1 happens in this call
    const auto& kv_idx = _db->get_index_type<key_value_index>().indices().get<by_scope_primary>();

    auto itr = kv_idx.iterator_to(obj);
    ++itr;

    if (itr == kv_idx.end() || itr->t_id != obj.t_id) return keyval_cache.get_end_iterator_by_table_id(obj.t_id);

    primary = itr->primary_key;
    return keyval_cache.add(*itr);
}

int apply_context::db_previous_i64(int iterator, uint64_t &primary)
{
    const auto& idx = _db->get_index_type<key_value_index>().indices().get<by_scope_primary>();

    // is end iterator
    if (iterator < -1) {
        auto tab = keyval_cache.find_table_by_end_iterator(iterator);
        FC_ASSERT(tab, "not a valid end iterator");

        auto itr = idx.upper_bound(tab->id);
        if (idx.begin() == idx.end() || itr == idx.begin()) return -1; // Empty table

        --itr;

        if (itr->t_id != tab->id) return -1; // Empty table

        primary = itr->primary_key;
        return keyval_cache.add(*itr);
    }

    const auto &obj = keyval_cache.get(iterator); // Check for iterator != -1 happens in this call

    auto itr = idx.iterator_to(obj);
    if (itr == idx.begin()) return -1; // cannot decrement past beginning iterator of table

    --itr;

    if (itr->t_id != obj.t_id) return -1; // cannot decrement past beginning iterator of table

    primary = itr->primary_key;
    return keyval_cache.add(*itr);
}

int apply_context::db_find_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id)
{
    const auto *tab = find_table(code, scope, table);
    if (!tab) return -1;

    auto table_end_itr = keyval_cache.cache_table(*tab);

    const auto& kv_idx = _db->get_index_type<key_value_index>().indices().get<by_scope_primary>();
    auto iter = kv_idx.find(boost::make_tuple(tab->id, id));
    if (iter == kv_idx.end()) return table_end_itr;

    return keyval_cache.add(*iter);
}

int apply_context::db_lowerbound_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id)
{
    const auto *tab = find_table(code, scope, table);
    if (!tab) return -1;

    auto table_end_itr = keyval_cache.cache_table(*tab);

    const auto& idx = _db->get_index_type<key_value_index>().indices().get<by_scope_primary>();
    auto itr = idx.lower_bound(boost::make_tuple(tab->id, id));
    if (itr == idx.end()) return table_end_itr;
    if (itr->t_id != tab->id) return table_end_itr;

    return keyval_cache.add(*itr);
}

int apply_context::db_upperbound_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id)
{
    const auto *tab = find_table(code, scope, table);
    if (!tab) return -1;

    auto table_end_itr = keyval_cache.cache_table(*tab);

    const auto& idx = _db->get_index_type<key_value_index>().indices().get<by_scope_primary>();
    auto itr = idx.upper_bound(boost::make_tuple(tab->id, id));
    if (itr == idx.end()) return table_end_itr;
    if (itr->t_id != tab->id) return table_end_itr;

    return keyval_cache.add(*itr);
}

int apply_context::db_end_i64(uint64_t code, uint64_t scope, uint64_t table)
{
    const auto *tab = find_table(code, scope, table);
    if (!tab) return -1;

    return keyval_cache.cache_table(*tab);
}

const table_id_object* apply_context::find_table(uint64_t code, name scope, name table)
{
    const auto& table_idx = _db->get_index_type<table_id_multi_index>().indices().get<by_code_scope_table>();
    auto existing_tid = table_idx.find(boost::make_tuple(code, scope, table));
    if (existing_tid != table_idx.end()) {
        return &(*existing_tid);
    }

    return nullptr;
}

const table_id_object &apply_context::find_or_create_table(uint64_t code, name scope, name table, account_name payer)
{
    const auto& table_idx = _db->get_index_type<table_id_multi_index>().indices().get<by_code_scope_table>();
    auto existing_tid = table_idx.find(boost::make_tuple(code, scope, table));
    if (existing_tid != table_idx.end()) {
        return *existing_tid;
    }

    // update db usage
    trx_context.update_ram_statistics(payer, config::billable_size_v<table_id_object>);

    return _db->create<table_id_object>([&](table_id_object &t_id){
            t_id.code = code;
            t_id.scope = scope;
            t_id.table = table;
            t_id.payer = payer;
            });
}

void apply_context::remove_table(const table_id_object &tid)
{
    trx_context.update_ram_statistics(tid.payer, -config::billable_size_v<table_id_object>);
    _db->remove(tid);
}

} } /// graphene::chain
