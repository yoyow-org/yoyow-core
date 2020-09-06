#pragma once

namespace graphene { namespace chain {

   class transaction_context {
      public:
        transaction_context(database &d, uint64_t origin, fc::microseconds max_trx_cpu_us);

        ~transaction_context();

        void pause_billing_timer();

        void resume_billing_timer();

        void checktime() const;

        uint64_t get_cpu_usage() const
        {
            return transaction_cpu_usage_us;
        }

        void update_ram_statistics(uint64_t account_id, int64_t ram_delta)
        {
            if(0 == account_id)//ignore the feeback
                return;
        	ram_statistics[account_id] += ram_delta; //TODO payer maybe is 0,how to feedback the ram fee to which count?the ram-account
        }

        const std::map<uint64_t, int64_t>& get_ram_statistics() const
        {
            return ram_statistics;
        }

        void check_inter_contract_depth()
        {
            // check max_inter_contract_depth
            FC_ASSERT(inter_contract_calling_count <= inter_contract_calling_params.max_inter_contract_depth,
                    "max cross contract calling can not exceed ${m}", ("m", inter_contract_calling_params.max_inter_contract_depth));
            inter_contract_calling_count++;
        }

        const extension_parameter_type& get_inter_contract_calling_params() const {
            return inter_contract_calling_params;
        }

      private:
        void dispatch_operation(const inter_contract_call_operation &op);

      private:
        friend class apply_context;

      public:
        database &db() const { assert(_db); return *_db; }
        uint64_t get_trx_origin() const { return trx_origin;  }

      private:
        database*                              _db;
        uint64_t                               trx_origin;
        uint64_t                               inter_contract_calling_count = 0;
        const extension_parameter_type  &      inter_contract_calling_params;
        std::map<uint64_t, int64_t>            ram_statistics;

        mutable fc::time_point                 start;
        mutable fc::time_point                 _deadline;
        mutable fc::time_point                 pause_time;
        mutable int64_t                        pause_cpu_usage_us = 0;
        mutable int64_t                        transaction_cpu_usage_us = 0;
   };
} }
