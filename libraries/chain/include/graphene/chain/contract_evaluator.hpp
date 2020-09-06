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
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/chain/protocol/contract_receipt.hpp>
#include <graphene/chain/evaluator.hpp>

namespace graphene { namespace chain {

class contract_deploy_evaluator : public evaluator<contract_deploy_evaluator>
{
  public:
    typedef contract_deploy_operation operation_type;

    void_result do_evaluate(const contract_deploy_operation &op);
    object_id_type do_apply(const contract_deploy_operation &op);
};

class contract_update_evaluator : public evaluator<contract_update_evaluator>
{
  public:
    typedef contract_update_operation operation_type;

    void_result do_evaluate(const contract_update_operation &op);
    void_result do_apply(const contract_update_operation &op);
    string code_hash;
};

class contract_call_evaluator : public evaluator<contract_call_evaluator>
{
  public:
    typedef contract_call_operation operation_type;

    void_result do_evaluate(const contract_call_operation &op);
    contract_receipt do_apply(const contract_call_operation &op);

  private:
    contract_call_operation::fee_parameters_type get_contract_call_fee_parameter(const database &db);
    void charge_ram_fee_by_account(account_receipt &r, database &db, const contract_call_operation &op);

  private:
    account_uid_type ram_account_id;
    contract_call_operation::fee_parameters_type fee_param;
};

class inter_contract_call_evaluator : public evaluator<inter_contract_call_evaluator>
{
  public:
    typedef inter_contract_call_operation operation_type;

    void_result do_evaluate(const inter_contract_call_operation &op);
    void_result do_apply(const inter_contract_call_operation &op);
};

} }
