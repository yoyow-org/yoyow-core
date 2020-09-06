#pragma once

#include <graphenelib/types.h>
extern "C" {

/**
 *  @defgroup databaseC Database C API
 *  @brief C APIs for interfacing with the database.
 *  @ingroup database
 */

int32_t db_store_i64(uint64_t scope, table_name table, uint64_t payer, uint64_t id,  const void* data, uint32_t len);
void db_update_i64(int32_t iterator, uint64_t payer, const void* data, uint32_t len);
void db_remove_i64(int32_t iterator);
int32_t db_get_i64(int32_t iterator, const void* data, uint32_t len);
int32_t db_next_i64(int32_t iterator, uint64_t* primary);
int32_t db_previous_i64(int32_t iterator, uint64_t* primary);
int32_t db_find_i64(uint64_t code, uint64_t scope, table_name table, uint64_t id);
int32_t db_lowerbound_i64(uint64_t code, uint64_t scope, table_name table, uint64_t id);
int32_t db_upperbound_i64(uint64_t code, uint64_t scope, table_name table, uint64_t id);
int32_t db_end_i64(uint64_t code, uint64_t scope, table_name table);

int32_t db_idx64_store(uint64_t scope, table_name table, uint64_t payer, uint64_t id, const uint64_t* secondary);
void db_idx64_update(int32_t iterator, uint64_t payer, const uint64_t* secondary);
void db_idx64_remove(int32_t iterator);
int32_t db_idx64_next(int32_t iterator, uint64_t* primary);
int32_t db_idx64_previous(int32_t iterator, uint64_t* primary);
int32_t db_idx64_find_primary(uint64_t code, uint64_t scope, table_name table, uint64_t* secondary, uint64_t primary);
int32_t db_idx64_find_secondary(uint64_t code, uint64_t scope, table_name table, const uint64_t* secondary, uint64_t* primary);
int32_t db_idx64_lowerbound(uint64_t code, uint64_t scope, table_name table, uint64_t* secondary, uint64_t* primary);
int32_t db_idx64_upperbound(uint64_t code, uint64_t scope, table_name table, uint64_t* secondary, uint64_t* primary);
int32_t db_idx64_end(uint64_t code, uint64_t scope, table_name table);

int32_t db_idx128_store(uint64_t scope, table_name table, uint64_t payer, uint64_t id, const uint128_t* secondary);
void db_idx128_update(int32_t iterator, uint64_t payer, const uint128_t* secondary);
void db_idx128_remove(int32_t iterator);
int32_t db_idx128_next(int32_t iterator, uint64_t* primary);
int32_t db_idx128_previous(int32_t iterator, uint64_t* primary);
int32_t db_idx128_find_primary(uint64_t code, uint64_t scope, table_name table, uint128_t* secondary, uint64_t primary);
int32_t db_idx128_find_secondary(uint64_t code, uint64_t scope, table_name table, const uint128_t* secondary, uint64_t* primary);
int32_t db_idx128_lowerbound(uint64_t code, uint64_t scope, table_name table, uint128_t* secondary, uint64_t* primary);
int32_t db_idx128_upperbound(uint64_t code, uint64_t scope, table_name table, uint128_t* secondary, uint64_t* primary);
int32_t db_idx128_end(uint64_t code, uint64_t scope, table_name table);

int32_t db_idx256_store(uint64_t scope, table_name table, uint64_t payer, uint64_t id, const void* data, uint32_t data_len );
void db_idx256_update(int32_t iterator, uint64_t payer, const void* data, uint32_t data_len);
void db_idx256_remove(int32_t iterator);
int32_t db_idx256_next(int32_t iterator, uint64_t* primary);
int32_t db_idx256_previous(int32_t iterator, uint64_t* primary);
int32_t db_idx256_find_primary(uint64_t code, uint64_t scope, table_name table, void* data, uint32_t data_len, uint64_t primary);
int32_t db_idx256_find_secondary(uint64_t code, uint64_t scope, table_name table, const void* data, uint32_t data_len, uint64_t* primary);
int32_t db_idx256_lowerbound(uint64_t code, uint64_t scope, table_name table, void* data, uint32_t data_len, uint64_t* primary);
int32_t db_idx256_upperbound(uint64_t code, uint64_t scope, table_name table, void* data, uint32_t data_len, uint64_t* primary);
int32_t db_idx256_end(uint64_t code, uint64_t scope, table_name table);

int32_t db_idx_double_store(uint64_t scope, table_name table, uint64_t payer, uint64_t id, const double* secondary);
void db_idx_double_update(int32_t iterator, uint64_t payer, const double* secondary);
void db_idx_double_remove(int32_t iterator);
int32_t db_idx_double_next(int32_t iterator, uint64_t* primary);
int32_t db_idx_double_previous(int32_t iterator, uint64_t* primary);
int32_t db_idx_double_find_primary(uint64_t code, uint64_t scope, table_name table, double* secondary, uint64_t primary);
int32_t db_idx_double_find_secondary(uint64_t code, uint64_t scope, table_name table, const double* secondary, uint64_t* primary);
int32_t db_idx_double_lowerbound(uint64_t code, uint64_t scope, table_name table, double* secondary, uint64_t* primary);
int32_t db_idx_double_upperbound(uint64_t code, uint64_t scope, table_name table, double* secondary, uint64_t* primary);
int32_t db_idx_double_end(uint64_t code, uint64_t scope, table_name table);

int32_t db_idx_long_double_store(uint64_t scope, table_name table, uint64_t payer, uint64_t id, const long double* secondary);
void db_idx_long_double_update(int32_t iterator, uint64_t payer, const long double* secondary);
void db_idx_long_double_remove(int32_t iterator);
int32_t db_idx_long_double_next(int32_t iterator, uint64_t* primary);
int32_t db_idx_long_double_previous(int32_t iterator, uint64_t* primary);
int32_t db_idx_long_double_find_primary(uint64_t code, uint64_t scope, table_name table, long double* secondary, uint64_t primary);
int32_t db_idx_long_double_find_secondary(uint64_t code, uint64_t scope, table_name table, const long double* secondary, uint64_t* primary);
int32_t db_idx_long_double_lowerbound(uint64_t code, uint64_t scope, table_name table, long double* secondary, uint64_t* primary);
int32_t db_idx_long_double_upperbound(uint64_t code, uint64_t scope, table_name table, long double* secondary, uint64_t* primary);
int32_t db_idx_long_double_end(uint64_t code, uint64_t scope, table_name table);

}
