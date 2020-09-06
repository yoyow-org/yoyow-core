#pragma once
#include <graphenelib/print.hpp>
#include <graphenelib/serialize.hpp>
#include <graphenelib/system.h>
#include <graphenelib/types.hpp>
#include <tuple>

namespace graphene {

extern const int64_t scaled_precision_lut[];

struct contract_asset {
    contract_asset(int64_t a = 0, uint64_t id = 0)
        : amount(a)
        , asset_id(id & GRAPHENE_DB_MAX_INSTANCE_ID)
    {
        graphene_assert(is_amount_within_range(), "magnitude of asset amount must be less than 2^62");
    }

    int64_t     amount;
    uint64_t    asset_id;

    static constexpr int64_t max_amount = (1LL << 62) - 1;

    bool is_amount_within_range() const { return -max_amount <= amount && amount <= max_amount; }

    contract_asset &operator+=(const contract_asset &o)
    {
        graphene_assert(asset_id == o.asset_id, "different asset_id");
        amount += o.amount;
        graphene_assert(-max_amount <= amount, "subtraction underflow");
        graphene_assert(amount <= max_amount, "subtraction overflow");
        return *this;
    }
    contract_asset &operator-=(const contract_asset &o)
    {
        graphene_assert(asset_id == o.asset_id, "different asset_id");
        amount -= o.amount;
        graphene_assert(-max_amount <= amount, "subtraction underflow");
        graphene_assert(amount <= max_amount, "subtraction overflow");
        return *this;
    }
    contract_asset operator-() const { return contract_asset(-amount, asset_id); }

    contract_asset &operator*=(int64_t a)
    {
        int128_t tmp = (int128_t)amount * (int128_t)a;
        graphene_assert( tmp <= max_amount, "multiplication overflow" );
        graphene_assert( tmp >= -max_amount, "multiplication underflow" );
        amount = (int64_t)tmp;
        return *this;
    }

    friend contract_asset operator*(const contract_asset &a, int64_t b)
    {
        contract_asset result = a;
        result *= b;
        return result;
    }

    friend contract_asset operator*(int64_t b, const contract_asset &a)
    {
        contract_asset result = a;
        result *= b;
        return result;
    }

    contract_asset &operator/=(int64_t a)
    {
        graphene_assert(a != 0, "divide by zero");
        graphene_assert(!(amount == std::numeric_limits<int64_t>::min() && a == -1), "signed division overflow");
        amount /= a;
        return *this;
    }

    friend contract_asset operator/(const contract_asset &a, int64_t b)
    {
        contract_asset result = a;
        result /= b;
        return result;
    }


    friend bool operator==(const contract_asset &a, const contract_asset &b)
    {
        return std::tie(a.asset_id, a.amount) == std::tie(b.asset_id, b.amount);
    }
    friend bool operator<(const contract_asset &a, const contract_asset &b)
    {
        graphene_assert(a.asset_id == b.asset_id, "different asset_id");
        return a.amount < b.amount;
    }
    friend bool operator<=(const contract_asset &a, const contract_asset &b)
    {
        return (a == b) || (a < b);
    }

    friend bool operator!=(const contract_asset &a, const contract_asset &b)
    {
        return !(a == b);
    }
    friend bool operator>(const contract_asset &a, const contract_asset &b)
    {
        return !(a <= b);
    }
    friend bool operator>=(const contract_asset &a, const contract_asset &b)
    {
        return !(a < b);
    }

    friend contract_asset operator-(const contract_asset &a, const contract_asset &b)
    {
        graphene_assert(a.asset_id == b.asset_id, "different asset_id");
        return contract_asset(a.amount - b.amount, a.asset_id);
    }
    friend contract_asset operator+(const contract_asset &a, const contract_asset &b)
    {
        graphene_assert(a.asset_id == b.asset_id, "different asset_id");
        return contract_asset(a.amount + b.amount, a.asset_id);
    }

    static int64_t scaled_precision(uint8_t precision)
    {
        graphene_assert(precision < 19, "precision < 19");
        return scaled_precision_lut[precision];
    }

    GRAPHENE_SERIALIZE(contract_asset, (amount)(asset_id))
};

}
