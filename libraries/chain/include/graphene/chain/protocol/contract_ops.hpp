/*
    Copyright (C) 2020 yoyow

    This file is part of yoyow-core.

    yoyow-core is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    yoyow-core is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with yoyow-core.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/action.hpp>
#include <graphene/chain/abi_def.hpp>


namespace graphene { namespace chain {
struct contract_deploy_operation : public base_operation {
    struct fee_parameters_type {
        uint64_t fee = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
        uint64_t min_real_fee     = 0;
		uint16_t min_rf_percent   = 0;
		uint32_t price_per_kbyte  = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
		extensions_type   extensions;
    };

    fee_type                        fee;
    account_uid_type                contract_id;
    fc::string                      vm_type;
    fc::string                      vm_version;
    bytes                           code;
    abi_def                         abi;
    extensions_type                 extensions;

    account_uid_type fee_payer_uid() const
    {
        return contract_id;
    }

    void validate() const
    {
        validate_op_fee( fee, "contract_deploy" );
		validate_account_uid(contract_id,"contract_id ");
        FC_ASSERT(code.size() > 0, "contract code cannot be empty");
		FC_ASSERT(abi.actions.size() > 0, "contract has no actions");
    }

    share_type calculate_fee(const fee_parameters_type &k) const
    {
        auto core_fee_required = k.fee;
		auto bSize = vm_type.size() + vm_version.size() + code.size()+ fc::raw::pack_size(abi);
        auto data_fee = calculate_data_fee(bSize, k.price_per_kbyte);
        core_fee_required += data_fee;
        return core_fee_required;
    }
};

struct contract_update_operation : public base_operation {
    struct fee_parameters_type {
        uint64_t fee = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
        uint64_t min_real_fee     = 0;
		uint16_t min_rf_percent   = 0;
		uint32_t price_per_kbyte  = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
		extensions_type   extensions;
    };

    fee_type                        fee;
    account_uid_type                contract_id;
    bytes                           code;
    abi_def                         abi;
    extensions_type                 extensions;

    account_uid_type fee_payer_uid() const
    {
        return contract_id;
    }

    void validate() const
    {
        validate_op_fee( fee, "contract_update" );
        FC_ASSERT(code.size() > 0, "contract code cannot be empty");
		FC_ASSERT(abi.actions.size() > 0, "contract has no actions");
		

		validate_account_uid(contract_id,"contract_id ");
    }

    share_type calculate_fee(const fee_parameters_type &k) const
    {
        auto core_fee_required = k.fee;
		auto bSize = code.size() + fc::raw::pack_size(abi);
        auto data_fee = calculate_data_fee(bSize, k.price_per_kbyte);
        core_fee_required += data_fee;
        return core_fee_required;
    }
};

struct contract_call_operation : public base_operation {
    struct fee_parameters_type {
        uint64_t fee =  GRAPHENE_BLOCKCHAIN_PRECISION / 10;
        uint64_t price_per_kbyte_ram =  GRAPHENE_BLOCKCHAIN_PRECISION / 2; //fee of ram should pay from balance directly. not include in fee_type fee
        uint64_t price_per_ms_cpu = GRAPHENE_BLOCKCHAIN_PRECISION;
		uint64_t min_real_fee     = 0;
		uint16_t min_rf_percent   = 0;
		extensions_type   extensions;		
    };

    fee_type                                fee;
    account_uid_type                        account;
    account_uid_type                        contract_id;
    fc::optional<asset>                     amount;
    action_name                             method_name;
    bytes                                   data;
    extensions_type                         extensions;

    account_uid_type fee_payer_uid() const { return account; }

    void validate() const
    {
        validate_op_fee( fee, "contract_call" );
		validate_account_uid(contract_id,"contract_id ");
        FC_ASSERT(data.size() >= 0);
		if (amount.valid()) {
        	FC_ASSERT(amount->amount > 0, "amount must > 0");
		}
    }

    share_type calculate_fee(const fee_parameters_type &k) const
    {
        // just return basic fee, real fee will be calculated after runing
        return k.fee;
    }
};

struct inter_contract_call_operation : public base_operation {
    struct fee_parameters_type {
        uint64_t fee = 0;
    };

    fee_type                                   fee;
    account_uid_type                         sender_contract;
    account_uid_type                         contract_id;
    fc::optional<asset>                     amount;
    action_name                             method_name;
    bytes                                   data;
    extensions_type                         extensions;

    account_uid_type fee_payer_uid() const { return sender_contract; }

    void validate() const
    {
        FC_ASSERT(false,"virtual operation");
    }

    share_type calculate_fee(const fee_parameters_type &k) const
    {
        // just return basic fee, real fee will be calculated after runing
        return k.fee;
    }
};


} } // graphene::chain

FC_REFLECT(graphene::chain::contract_deploy_operation::fee_parameters_type, (fee)(min_real_fee)(min_rf_percent)(price_per_kbyte)(extensions) )
FC_REFLECT(graphene::chain::contract_deploy_operation,
            (fee)
            (contract_id)
            (vm_type)
            (vm_version)
            (code)
            (abi)
            (extensions))
            
FC_REFLECT(graphene::chain::contract_update_operation::fee_parameters_type, (fee)(min_real_fee)(min_rf_percent)(price_per_kbyte)(extensions) )
FC_REFLECT(graphene::chain::contract_update_operation,
            (fee)
            (contract_id)
            (code)
            (abi)
            (extensions))

FC_REFLECT(graphene::chain::contract_call_operation::fee_parameters_type,
            (fee)(price_per_kbyte_ram)(price_per_ms_cpu)(min_real_fee)(min_rf_percent)(extensions) )
FC_REFLECT(graphene::chain::contract_call_operation,
            (fee)
            (account)
            (contract_id)
            (amount)
            (method_name)
            (data)
            (extensions))

FC_REFLECT(graphene::chain::inter_contract_call_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::inter_contract_call_operation,
            (fee)
            (sender_contract)
            (contract_id)
            (amount)
            (method_name)
            (data)
            (extensions))
