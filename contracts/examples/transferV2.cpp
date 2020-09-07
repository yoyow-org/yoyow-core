#include <graphenelib/contract.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/print.hpp>

using namespace graphene;

class _transfer : public contract
{
  public:
    _transfer(uint64_t id)
        : contract(id)
        , offers(_self, _self)
    {
    }

    //@abi action
    void add(const uint64_t& uid, const uint64_t& amount)
    {
        graphene_assert(offers.find(uid) == offers.end(),"item already exists");
        
        offers.emplace(0, [&](auto &o) {
            o.uid = uid;
            o.amount = amount;
        });
    }

    //@abi action
    void transfer(const uint64_t& from,const uint64_t& to,const uint64_t& amount)
    {
        auto itr_from = offers.find(from);
        graphene_assert(itr_from != offers.end(),"from not exists");
        graphene_assert(itr_from->amount > amount,"balance is not enough");
        
        graphene_assert(from == get_trx_sender(),"invalid authorityy");
        
        offers.modify(itr_from,0, [&](auto &o) {
            o.amount -= amount;
        	});
        
        auto itr_to = offers.find(to);
        if(itr_to == offers.end())
      	{
	      	offers.emplace(0, [&](auto &o) {
	            o.uid = to;
	            o.amount = amount;
	        });
      	}
      	else
    		{
    			offers.modify(itr_to,0, [&](auto &o) {
            o.amount += amount;
        	});
    		}
    }

  private:
    //@abi table offer i64
    struct offer {
        uint64_t uid;
        uint64_t amount;

        uint64_t primary_key() const { return uid; }

      
        GRAPHENE_SERIALIZE(offer, (uid)(amount))
    };

    typedef multi_index<N(offer), offer>  offer_index;

    offer_index offers;
};

GRAPHENE_ABI(_transfer, (add)(transfer))
