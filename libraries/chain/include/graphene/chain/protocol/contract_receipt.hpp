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
#include <graphene/chain/protocol/asset.hpp>

namespace graphene { namespace chain {

   struct account_receipt {
	   account_uid_type account;
       int64_t          ram_bytes;
       asset            ram_fee;

       explicit operator std::string() const
       {
           return "{\"account\":" + std::to_string(account) +
				  ", \"ram_bytes\":" + std::to_string(ram_bytes) +
                  ", \"ram_fee\":{\"asset_id\":" + std::to_string(ram_fee.asset_id) +
                  ", \"amount\":"+ std::to_string(ram_fee.amount.value) + "}}";
       }
   };

   struct contract_receipt {
       uint32_t                billed_cpu_time_us = 0;
       asset                   fee;
       vector<account_receipt> ram_receipts;


       string to_string() const
       {
            string receipts_str = "{\"billed_cpu_time_us\":" + std::to_string(billed_cpu_time_us) +
                                  ",\"fee\":{\"asset_id\":" + std::to_string(fee.asset_id) +
                                  ",\"amount\":"+ std::to_string(fee.amount.value) + "},\"ram_receipts\":[";
            for(const auto &receipt : ram_receipts)
               receipts_str += ((string)receipt + ",");
            receipts_str += "]}";
            return receipts_str;
       }
   };

} }  // graphene::chain


FC_REFLECT(graphene::chain::account_receipt,
                            (account)
                            (ram_bytes)
                            (ram_fee))

FC_REFLECT(graphene::chain::contract_receipt,
                            (billed_cpu_time_us)
							(fee)
							(ram_receipts))
