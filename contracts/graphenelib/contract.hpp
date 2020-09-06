#pragma once
#include <stdint.h>

namespace graphene {
#define ACTION void
#define PAYABLE void 
#define TABLE struct 
#define CONTRACT class

class contract
{
  public:
    contract(uint64_t account_id)
        : _self(account_id)
    {
    }

    inline uint64_t get_self() const { return _self; }

  protected:
    uint64_t _self;
};

}
