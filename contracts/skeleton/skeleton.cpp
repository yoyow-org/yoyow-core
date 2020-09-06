#include <graphenelib/contract.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/print.hpp>
#include <graphenelib/types.h>

using namespace graphene;

class skeleton : public contract
{
  public:
    skeleton(uint64_t id)
        : contract(id)
    {
    }

    /// @abi action
    void hi(std::string user)
    {
        for (int i = 0; i < 2; ++i) {
            print("hi, ", user, "\n");
        }
    }
};

GRAPHENE_ABI(skeleton, (hi))
