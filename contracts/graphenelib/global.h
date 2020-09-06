#pragma once
#include <graphenelib/types.h>

extern "C" {
    // return head block num
    int64_t get_head_block_num();
    // return head block id
    void get_head_block_id(checksum160* hash);
    void get_block_id_for_num(checksum160* hash, uint32_t block_num);
    // return head block time
    int64_t get_head_block_time();
    // return trx sender
    uint64_t get_trx_sender();
    // return original trx sender
    uint64_t get_trx_origin();
    // get account_id by name, return  -1 if name not exists, return account_id if succeed
    int64_t get_account_id(const char *data, uint32_t length);
    // get asset_id by name, return -1 if asset not exists, return asset id if succeeed
    int64_t get_asset_id(const char *data, uint32_t length);
    // get asset_precision, return -1 if asset not exists, retrun precision if succeed
    int64_t get_asset_precision(const char *data, uint32_t datalen);
    // dump current transaction of current contract calling to dst in binary form
    int read_transaction(char* dst, uint32_t dst_size);
    // get current transaction size of current contract calling
    int transaction_size();
    // return current transaction expiration
    uint64_t expiration();
    // return ref block num(block_id.hash[0])
    // eg. "block_id": "00000fa00f4dd71912f56dd6e23f03bf2af87be5" --> 00000fa0
    int tapos_block_num();
    // return ref block prefix(block_id.hash[1])
    //eg. "block_id": "00000fa00f4dd71912f56dd6e23f03bf2af87be5" --> 0f4dd719
    uint64_t tapos_block_prefix();
    // get account_name by id, return -1 if fail, return 0 if success
    int64_t get_account_name_by_id(char* data, uint32_t datalen, int64_t account_id);
}
