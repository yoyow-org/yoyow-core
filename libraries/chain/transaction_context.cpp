#include <graphene/chain/apply_context.hpp>
#include <graphene/chain/transaction_context.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/transaction_object.hpp>

namespace graphene { namespace chain {

   transaction_context::transaction_context(database &d, uint64_t origin, fc::microseconds max_trx_cpu_us) :
        _db(&d),
        trx_origin(origin),
		inter_contract_calling_count(0),
        start(fc::time_point::now()),
        _deadline(start + max_trx_cpu_us),
        inter_contract_calling_params(d.get_global_extension_params()),
        transaction_cpu_usage_us(0)
   {
       _db->set_contract_transaction_ctx(this);
   }

   transaction_context::~transaction_context()
   {
       _db->set_contract_transaction_ctx(nullptr);
   }

   void transaction_context::pause_billing_timer() {
       pause_time = fc::time_point::now();
   }

   void transaction_context::resume_billing_timer() {
       if(pause_time == fc::time_point()) {
           return;
       }
       pause_cpu_usage_us = (fc::time_point::now() - pause_time).count();
       pause_time = fc::time_point();
   }

   void transaction_context::checktime() const
   {
       if(pause_time > fc::time_point())
           return;

       if(pause_cpu_usage_us > 0) {
           const fc::microseconds mss(pause_cpu_usage_us);
           _deadline += mss;
           start += mss;
           pause_cpu_usage_us = 0;
       }

       auto now = fc::time_point::now();
       transaction_cpu_usage_us = (now - start).count();//TODO
       if (BOOST_UNLIKELY(now > _deadline)) {
           GRAPHENE_THROW(tx_cpu_usage_exceeded,
                          "transaction was executing for too long",
                          ("now", now)("deadline", _deadline)("start", start)("billing_timer", now - start));
       }
   }

   void transaction_context::dispatch_operation(const inter_contract_call_operation &op)
   {
       auto &d = db();

       try {
           transaction_evaluation_state op_context(&d);
           op_context.skip_fee_schedule_check = true;
           d.apply_operation(op_context, op);
       } catch (...) {
           wlog("apply_context exec failed");
           throw;
       }
   }

} } /// graphene::chain
