#pragma once
#include <graphenelib/types.h>

extern "C" {

    void withdraw_asset(uint64_t from, uint64_t to, uint64_t asset_id, int64_t amount);
    int64_t get_balance(int64_t account, int64_t asset_id);
    void inline_transfer(uint64_t from, uint64_t to, uint64_t asset_id, int64_t amount, const char* data, uint32_t length);

}
