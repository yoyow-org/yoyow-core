#pragma once
#include <tuple>

namespace graphene { namespace chain {

extern const int64_t scaled_precision_lut[];

struct contract_asset {
    contract_asset(int64_t a = 0, uint64_t id = 0)
        : amount(a)
        , asset_id(id)
    {
        FC_ASSERT(is_amount_within_range(), "magnitude of asset amount must be less than 2^62");
    }

    int64_t     amount;
    uint64_t    asset_id;

    static constexpr int64_t max_amount = (1LL << 62) - 1;

    bool is_amount_within_range() const { return -max_amount <= amount && amount <= max_amount; }

    contract_asset &operator+=(const contract_asset &o)
    {
        FC_ASSERT(asset_id == o.asset_id);
        amount += o.amount;
        FC_ASSERT(-max_amount <= amount, "subtraction underflow");
        FC_ASSERT(amount <= max_amount, "subtraction overflow");
        return *this;
    }
    contract_asset &operator-=(const contract_asset &o)
    {
        FC_ASSERT(asset_id == o.asset_id);
        amount -= o.amount;
        FC_ASSERT(-max_amount <= amount, "subtraction underflow");
        FC_ASSERT(amount <= max_amount, "subtraction overflow");
        return *this;
    }
    contract_asset operator-() const { return contract_asset(-amount, asset_id); }

    friend bool operator==(const contract_asset &a, const contract_asset &b)
    {
        return std::tie(a.asset_id, a.amount) == std::tie(b.asset_id, b.amount);
    }
    friend bool operator<(const contract_asset &a, const contract_asset &b)
    {
        FC_ASSERT(a.asset_id == b.asset_id);
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
        FC_ASSERT(a.asset_id == b.asset_id);
        return contract_asset(a.amount - b.amount, a.asset_id);
    }
    friend contract_asset operator+(const contract_asset &a, const contract_asset &b)
    {
        FC_ASSERT(a.asset_id == b.asset_id);
        return contract_asset(a.amount + b.amount, a.asset_id);
    }

    static int64_t scaled_precision(uint8_t precision)
    {
        FC_ASSERT(precision < 19);
        return scaled_precision_lut[precision];
    }
};

} }

FC_REFLECT(graphene::chain::contract_asset, (amount)(asset_id))
