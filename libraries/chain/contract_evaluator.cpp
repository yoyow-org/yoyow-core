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


#include <graphene/chain/contract_evaluator.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <graphene/chain/protocol/name.hpp>
#include <graphene/chain/protocol/contract_receipt.hpp>

#include <graphene/chain/apply_context.hpp>
#include <graphene/chain/transaction_context.hpp>
#include <graphene/chain/wast_to_wasm.hpp>
#include <graphene/chain/abi_serializer.hpp>
#include <graphene/chain/wasm_interface.hpp>

#include <algorithm>

namespace graphene { namespace chain {

void_result contract_deploy_evaluator::do_evaluate(const contract_deploy_operation &op)
{ try {
    database &d = db();

	FC_ASSERT(d.head_block_time()>=HARDFORK_2_0_TIME,"contract is not enabled before HARDFORK_2_0_TIME");

	auto& acnt_indx = d.get_index_type<account_index>();
	{
	  auto current_account_itr = acnt_indx.indices().get<by_uid>().find(op.contract_id );
	  FC_ASSERT( current_account_itr == acnt_indx.indices().get<by_uid>().end(), "account uid already exists." );
	}
	{
	  auto current_account_itr = acnt_indx.indices().get<by_name>().find( op.name );
	  FC_ASSERT( current_account_itr == acnt_indx.indices().get<by_name>().end(), "account name already exists." );
	}

	wasm_interface::validate(op.code);

    return void_result();
} FC_CAPTURE_AND_RETHROW((op)) }

object_id_type contract_deploy_evaluator::do_apply(const contract_deploy_operation &op)
{ try {
    const auto &new_acnt_object = db().create<account_object>([&](account_object &obj) {
            obj.reg_info.registrar = op.owner;
            obj.uid                  = op.contract_id;

            obj.name = op.name;
			obj.create_time          = db().head_block_time();
         	obj.last_update_time     = db().head_block_time();
		 
            obj.vm_type = op.vm_type;
            obj.vm_version = op.vm_version;
            obj.code = op.code;
            obj.code_version = fc::sha256::hash(op.code);
            obj.abi = op.abi;
            obj.statistics = db().create<_account_statistics_object>([&](_account_statistics_object& s){s.owner = obj.uid;}).id;
            });

    return new_acnt_object.id;
} FC_CAPTURE_AND_RETHROW((op)) }

void_result contract_update_evaluator::do_evaluate(const contract_update_operation &op)
{ try {
    database &d = db();

    const account_object& contract_obj = d.get_account_by_uid(op.contract_id);
    FC_ASSERT(op.owner == contract_obj.reg_info.registrar, "only owner can update contract, current owner: ${o}", ("o", contract_obj.reg_info.registrar));
    FC_ASSERT(contract_obj.code.size() > 0, "can not update a normal account: ${a}", ("a", op.contract_id));

    code_hash = fc::sha256::hash(op.code);
    FC_ASSERT(code_hash != contract_obj.code_version, "code not updated");

	wasm_interface::validate(op.code);

    if(op.new_owner.valid()) {
    	FC_ASSERT(d.find_account_by_uid(*op.new_owner), "new owner not exist");
    }

    return void_result();
} FC_CAPTURE_AND_RETHROW((op)) }

void_result contract_update_evaluator::do_apply(const contract_update_operation &op)
{ try {
    database &d = db();
    const account_object& contract_obj = d.get_account_by_uid(op.contract_id);
    db().modify(contract_obj, [&](account_object &obj) {
		 if(op.new_owner.valid()) {
            obj.reg_info.registrar = *op.new_owner;
        }
        obj.code = op.code;
        obj.code_version = code_hash;
        obj.abi = op.abi;
    });

    return void_result();
} FC_CAPTURE_AND_RETHROW((op)) }

void_result contract_call_evaluator::do_evaluate(const contract_call_operation &op)
{ try {
    database& d = db();
    const account_object& contract_obj = d.get_account_by_uid(op.contract_id);
    FC_ASSERT(contract_obj.code.size() > 0, "contract has no code, contract_id ${n}", ("n", op.contract_id));

    // check method_name
    const auto& actions = contract_obj.abi.actions;
    auto iter = std::find_if(actions.begin(), actions.end(),
            [&](const action_def& act) { return act.name == op.method_name; });
    FC_ASSERT(iter != actions.end(), "method_name ${m} not found in abi", ("m", op.method_name));
    if (op.amount.valid()) {
        // check method_name, must be payable
        FC_ASSERT(iter->payable, "method_name ${m} not payable", ("m", op.method_name));

        // check balance
        bool sufficient_balance = d.get_balance(op.account, op.amount->asset_id).amount >= op.amount->amount;
        FC_ASSERT(sufficient_balance,
                  "insufficient balance: ${balance}, unable to deposit '${total_transfer}' from account '${a}' to '${t}'",
                  ("a", op.account)("t", contract_obj.id)("total_transfer", d.to_pretty_string(op.amount->amount))
                  ("balance", d.to_pretty_string(d.get_balance(op.account, op.amount->asset_id))));
    }

    if (op.fee.total.amount > 0) {
        // fee_from_account is calculated by evaluator::evaluate()
        // prepare_fee --> do_evaluate -> convert_fee -> pay_fee -> do_apply
        // if cpu_fee charged, this check may fail for cpu time may different for the same opertion
        FC_ASSERT(op.fee.total >= fee_from_account, "insufficient fee paid in trx, ${a} needed", ("a", d.to_pretty_string(fee_from_account)));
    }

    // ram-account must exists
    const auto &account_idx = d.get_index_type<account_index>().indices().get<by_name>();
    const auto &account_itr = account_idx.find("ramaccount");
    FC_ASSERT(account_itr != account_idx.end(), "ramaccount not exist");
    ram_account_id = account_itr->uid;

    return void_result();
} FC_CAPTURE_AND_RETHROW((op)) }

contract_receipt contract_call_evaluator::do_apply(const contract_call_operation &op)
{ try {
    // do apply:
    //  1. run contract code
    //  2. charge base fee
    //  3. charge ram fee by account

    // charge base fee:
    // 1. calculate base_fee (basic fee + cpu fee)
    // 2. convert base_fee to core asset
    //      2.1 call prepare_fee to calculate fee_from_account and core_fee_paid
    //      2.2 call convert_fee to adjust UIA fee_pool (UIA: user-issued assets)
    // 3. deposit cashback
    // 4. adjust fee_payer's balance

    auto &d = db();

	fc::microseconds max_trx_cpu_us = fc::seconds(3);
    if (_billed_cpu_time_us == 0)
        max_trx_cpu_us = fc::microseconds(std::min(d.get_global_extension_params().trx_cpu_limit, d.get_max_trx_cpu_time()));

    action act{op.account, op.contract_id, op.method_name, op.data};
    if (op.amount.valid()) {
        act.amount.amount = op.amount->amount.value;
        act.amount.asset_id = op.amount->asset_id;
    }

    // run contract code
    transaction_context trx_context(d, op.fee_payer_uid(), max_trx_cpu_us);
    apply_context ctx{d, trx_context, act};
    ctx.exec();

	fee_param = get_contract_call_fee_parameter(d);
	uint32_t cpu_time_us = _billed_cpu_time_us > 0? _billed_cpu_time_us : trx_context.get_cpu_usage();
	if(cpu_time_us < 1000)
	{
		auto cpu_fee = uint64_t(cpu_time_us + 999) / 1000 * fee_param.price_per_ms_cpu;
		d.adjust_balance(op.account, - asset(cpu_fee,GRAPHENE_CORE_ASSET_AID));
	}

    contract_receipt receipt;
    receipt.billed_cpu_time_us = cpu_time_us;
    receipt.fee = fee_from_account;

    account_receipt r;
	
    auto ram_statistics = trx_context.get_ram_statistics();
    for (const auto &ram : ram_statistics) {
        // map<account, ram_bytes>
        r.account = account_uid_type(ram.first);
        r.ram_bytes = ram.second;

        // charge and set ram_fee
        charge_ram_fee_by_account(r, d, op);
        receipt.ram_receipts.push_back(r);
    }

    return receipt;    
} FC_CAPTURE_AND_RETHROW((op)) }


contract_call_operation::fee_parameters_type contract_call_evaluator::get_contract_call_fee_parameter(const database &db)
{
    auto fp = contract_call_operation::fee_parameters_type();
    const auto& p = db.get_global_properties().parameters;
    for (auto& param : p.current_fees->parameters) {
        if (param.which() == operation::tag<contract_call_operation>::value) {
            fp = param.get<contract_call_operation::fee_parameters_type>();
            break;
        }
    }
    return fp;
}


void contract_call_evaluator::charge_ram_fee_by_account(account_receipt &r, database &db, const contract_call_operation &op)
{
    if(0 == r.ram_bytes) {
        r.ram_fee = asset{0,GRAPHENE_CORE_ASSET_AID};
        return;
    }

    int64_t ram_fee_core = ceil(1.0 * r.ram_bytes * fee_param.price_per_kbyte_ram / 1024);
    //make sure ram-account have enough YOYO to refund
    if (ram_fee_core < 0) {
        asset ram_account_balance = db.get_balance(ram_account_id, asset_aid_type(GRAPHENE_CORE_ASSET_AID));
        ram_fee_core = -std::min(ram_account_balance.amount.value, -ram_fee_core);
    }

    r.ram_fee = asset{ram_fee_core, asset_aid_type(GRAPHENE_CORE_ASSET_AID)};
    db.adjust_balance(r.account, -r.ram_fee);
    db.adjust_balance(ram_account_id, r.ram_fee);
}

void_result inter_contract_call_evaluator::do_evaluate(const inter_contract_call_operation &op)
{ try {
    database &d = db();
    FC_ASSERT(d.get_contract_transaction_ctx() != nullptr, "contract_transaction_ctx invalid");
    return void_result();
} FC_CAPTURE_AND_RETHROW((op)) }

void_result inter_contract_call_evaluator::do_apply(const inter_contract_call_operation &op)
{ try {
    database &d = db();
	transaction_context* contract_transaction_ctx = d.get_contract_transaction_ctx();

    action act{op.sender_contract, op.contract_id, op.method_name, op.data};
    if (op.amount.valid()) {
        act.amount.amount = op.amount->amount.value;
        act.amount.asset_id = op.amount->asset_id;
    }

    apply_context ctx{d, *contract_transaction_ctx, act};
    ctx.exec();
    return void_result();
} FC_CAPTURE_AND_RETHROW((op)) }

} } // graphene::chain
