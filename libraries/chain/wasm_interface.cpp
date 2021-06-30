#include <graphene/chain/wasm_interface.hpp>
#include <graphene/chain/apply_context.hpp>
#include <graphene/chain/transaction_context.hpp>
#include <graphene/chain/wasm_interface_private.hpp>
#include <graphene/chain/wasm_validation.hpp>
#include <graphene/chain/wasm_injection.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/asset_object.hpp>


#include <fc/exception/exception.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/sha1.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/io/raw.hpp>

#include <softfloat.hpp>
#include <compiler_builtins.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/core/ignore_unused.hpp>
#include <fstream>

namespace graphene { namespace chain {
   using namespace webassembly;
   using namespace webassembly::common;

   wasm_interface::wasm_interface(vm_type vm) : my( new wasm_interface_impl(vm) ) {}

   wasm_interface::~wasm_interface() {}

   void wasm_interface::validate(const bytes& code) {
      Module module;
      try {
         Serialization::MemoryInputStream stream((U8*)code.data(), code.size());
         WASM::serialize(stream, module);
      } catch(const Serialization::FatalSerializationException& e) {
         GRAPHENE_ASSERT(false, wasm_serialization_error, e.message.c_str());
      } catch(const IR::ValidationException& e) {
         GRAPHENE_ASSERT(false, wasm_serialization_error, e.message.c_str());
      }

      wasm_validations::wasm_binary_validation validator(module);
      validator.validate();

      root_resolver resolver(true);
      LinkResult link_result = linkModule(module, resolver);

      //there are a couple opportunties for improvement here--
      //Easy: Cache the Module created here so it can be reused for instantiaion
      //Hard: Kick off instantiation in a separate thread at this location
	   }

   void wasm_interface::apply( const digest_type& code_id, const bytes& code, apply_context& context ) {
      my->get_instantiated_module(code_id, code, context.trx_context)->apply(context);
   }

   wasm_instantiated_module_interface::~wasm_instantiated_module_interface() {}
   wasm_runtime_interface::~wasm_runtime_interface() {}

#if defined(assert)
   #undef assert
#endif

class context_aware_api {
   public:
      context_aware_api(apply_context& ctx, bool context_free = false )
      :context(ctx)
      {
      }

      void checktime() {
          context.trx_context.checktime();
      }

   protected:
      apply_context&             context;

};

class call_depth_api : public context_aware_api {
   public:
      call_depth_api( apply_context& ctx )
      :context_aware_api(ctx,true){}
      void call_depth_assert() {
         FC_THROW_EXCEPTION(wasm_execution_error, "Exceeded call depth maximum");
      }
};

class action_api : public context_aware_api {
   public:
   action_api( apply_context& ctx )
      :context_aware_api(ctx,true){}

      int read_action_data(array_ptr<char> memory, size_t buffer_size) {
         auto s = context.act.data.size();
         if( buffer_size == 0 ) return s;

         auto copy_size = std::min( buffer_size, s );
         memcpy( memory, context.act.data.data(), copy_size );

         return copy_size;
      }

      int action_data_size() {
         return context.act.data.size();
      }

      name current_receiver() {
         return context.receiver;
      }

      uint64_t get_action_asset_id() {
          if (context.amount.valid()) {
              return context.amount->asset_id;
          }
          else {
              return 0xFFFFFFFF;
          }
      }

      int64_t get_action_asset_amount() {
          if (context.amount.valid()) {
              return context.amount->amount.value;
          }
          else {
              return 0;
          }
      }
};

class global_api : public context_aware_api
{
  public:
    explicit global_api(apply_context &ctx)
        : context_aware_api(ctx, true)
    {
    }

    // get head block header
    int64_t get_head_block_num()
    {
        const auto& dpo = context.db().get_dynamic_global_properties();
        return dpo.head_block_number;
    }

    // get head block hash
    void get_head_block_id(block_id_type& block_id)
    {
        const auto& dpo = context.db().get_dynamic_global_properties();
        block_id = dpo.head_block_id;
    }

    void get_block_id_for_num(block_id_type &block_id, uint32_t block_num)
    {
        int64_t head_block_num = get_head_block_num();
        FC_ASSERT(block_num <= head_block_num && block_num > 0, "block_num to large, can not big than head block num:${x}", ("x", head_block_num));
        block_id = context.db().get_block_id_for_num(block_num);
    }

    // get head block time
    int64_t get_head_block_time()
    {
        return static_cast<uint64_t>(context.db().head_block_time().sec_since_epoch());
    }

    // get sender of trx
    uint64_t get_trx_sender()
    {
        // get op payer
        return context.sender;
    }

    // get origin of trx
    uint64_t get_trx_origin()
    {
        // get op payer
        return context.trx_context.get_trx_origin();
    }

    int64_t get_account_name_by_id(array_ptr<char> data, size_t buffer_size, int64_t account_id)
    {
        FC_ASSERT(account_id >= 0, "account_id ${a} must > 0", ("a", account_id));
        auto &d = context.db();
        auto obj = d.find_account_by_uid(account_id);
        if (obj && obj->name.size() <= buffer_size) {
            string account_name = obj->name;
            memcpy(data, account_name.c_str(), account_name.size());
            return 0;
        }
        // account not exist, return -1
        return -1;
    }

    int64_t get_account_id(array_ptr<char> data, size_t datalen)
    {
        std::string account_name(data, datalen);
        const auto& idx = context.db().get_index_type<account_index>().indices().get<by_name>();
        auto itr = idx.find(account_name);
        if (itr != idx.end())
            return (itr->get_id()).instance;
        return -1;
    }

    int64_t get_asset_id(array_ptr<char> data, size_t datalen)
    {
        std::string symbol(data, datalen);
        const auto& idx = context.db().get_index_type<asset_index>().indices().get<by_symbol>();
        auto itr = idx.find(symbol);
        if (itr != idx.end())
            return (itr->get_id()).instance;
        return -1;
    }

};

class crypto_api : public context_aware_api {
   public:
      explicit crypto_api( apply_context& ctx )
      :context_aware_api(ctx,true){}

      void assert_recover_key(const fc::sha256 &digest,
                              const fc::ecc::compact_signature &sig,
                              array_ptr<char> pub, size_t publen)
      {
          public_key_type pk;
          datastream<const char *> pubds(pub, publen);
          fc::raw::unpack(pubds, pk);

          auto check = public_key_type(fc::ecc::public_key(sig, digest, true));
          FC_ASSERT(check == pk, "Error expected key different than recovered key");
      }

      //deprecated
      bool verify_signature(array_ptr<char> data, size_t datalen, const fc::ecc::compact_signature& sig, array_ptr<char> pub_key, size_t pub_keylen)
      {
          digest_type::encoder enc;
          fc::raw::pack(enc, std::string(data.value));

          public_key_type pk{std::string(pub_key.value)};
          return public_key_type(fc::ecc::public_key(sig, enc.result(), true)) == pk;
      }

      template<class Encoder> auto encode(char* data, size_t datalen) {
         Encoder e;
         const size_t bs = 10*1024;
         while ( datalen > bs ) {
            e.write( data, bs );
            data += bs;
            datalen -= bs;
            context.trx_context.checktime();
         }
         e.write( data, datalen );
         return e.result();
      }

      void assert_sha256(array_ptr<char> data, size_t datalen, const fc::sha256& hash_val) {
         auto result = encode<fc::sha256::encoder>( data, datalen );
         FC_ASSERT( result == hash_val, "hash mismatch" );
      }

      void assert_sha1(array_ptr<char> data, size_t datalen, const fc::sha1& hash_val) {
         auto result = encode<fc::sha1::encoder>( data, datalen );
         FC_ASSERT( result == hash_val, "hash mismatch" );
      }

      void assert_sha512(array_ptr<char> data, size_t datalen, const fc::sha512& hash_val) {
         auto result = encode<fc::sha512::encoder>( data, datalen );
         FC_ASSERT( result == hash_val, "hash mismatch" );
      }

      void assert_ripemd160(array_ptr<char> data, size_t datalen, const fc::ripemd160& hash_val) {
         auto result = encode<fc::ripemd160::encoder>( data, datalen );
         FC_ASSERT( result == hash_val, "hash mismatch" );
      }

      void sha1(array_ptr<char> data, size_t datalen, fc::sha1& hash_val) {
         hash_val = encode<fc::sha1::encoder>( data, datalen );
      }

      void sha256(array_ptr<char> data, size_t datalen, fc::sha256& hash_val) {
         hash_val = encode<fc::sha256::encoder>( data, datalen );
      }

      void sha512(array_ptr<char> data, size_t datalen, fc::sha512& hash_val) {
         hash_val = encode<fc::sha512::encoder>( data, datalen );
      }

      void ripemd160(array_ptr<char> data, size_t datalen, fc::ripemd160& hash_val) {
         hash_val = encode<fc::ripemd160::encoder>( data, datalen );
      }
};

class context_free_system_api : public context_aware_api
{
  public:
    explicit context_free_system_api(apply_context &ctx)
        : context_aware_api(ctx, true) {}

    void abort()
    {
        edump(("abort() called"));
        FC_ASSERT(false, "abort() called");
    }

    void graphene_assert(bool condition, null_terminated_ptr msg)
    {
        if (BOOST_UNLIKELY(!condition)) {
            std::string message(msg);
            edump((message));
            GRAPHENE_THROW(graphene_assert_message_exception, "assertion failure with message: ${s}", ("s", message));
        }
    }

    void graphene_assert_message(bool condition, array_ptr<const char> msg, size_t msg_len)
    {
        if (BOOST_UNLIKELY(!condition)) {
            std::string message(msg, msg_len);
            edump((message));
            GRAPHENE_THROW(graphene_assert_message_exception, "assertion failure with message: ${s}", ("s", message));
        }
    }

    void graphene_assert_code(bool condition, uint64_t error_code)
    {
        if (BOOST_UNLIKELY(!condition)) {
            edump((error_code));
            GRAPHENE_THROW(graphene_assert_code_exception,
                      "assertion failure with error code: ${error_code}", ("error_code", error_code));
        }
    }

    void graphene_exit(int32_t code)
    {
        throw wasm_exit{code};
    }

};

class softfloat_api : public context_aware_api
{
  public:
    // TODO add traps on truncations for special cases (NaN or outside the range which rounds to an integer)
    softfloat_api(apply_context &ctx)
        : context_aware_api(ctx, true)
    {
    }

    // float binops
    float _yy_f32_add(float a, float b)
    {
        float32_t ret = f32_add(to_softfloat32(a), to_softfloat32(b));
        return *reinterpret_cast<float *>(&ret);
    }
    float _yy_f32_sub(float a, float b)
    {
        float32_t ret = f32_sub(to_softfloat32(a), to_softfloat32(b));
        return *reinterpret_cast<float *>(&ret);
    }
    float _yy_f32_div(float a, float b)
    {
        float32_t ret = f32_div(to_softfloat32(a), to_softfloat32(b));
        return *reinterpret_cast<float *>(&ret);
    }
    float _yy_f32_mul(float a, float b)
    {
        float32_t ret = f32_mul(to_softfloat32(a), to_softfloat32(b));
        return *reinterpret_cast<float *>(&ret);
    }
    float _yy_f32_min(float af, float bf)
    {
        float32_t a = to_softfloat32(af);
        float32_t b = to_softfloat32(bf);
        if (is_nan(a)) {
            return af;
         }
         if (is_nan(b)) {
            return bf;
         }
         if ( sign_bit(a) != sign_bit(b) ) {
            return sign_bit(a) ? af : bf;
         }
         return f32_lt(a,b) ? af : bf;
    }
    float _yy_f32_max(float af, float bf)
    {
        float32_t a = to_softfloat32(af);
        float32_t b = to_softfloat32(bf);
        if (is_nan(a)) {
            return af;
         }
         if (is_nan(b)) {
            return bf;
         }
         if ( sign_bit(a) != sign_bit(b) ) {
            return sign_bit(a) ? bf : af;
         }
         return f32_lt( a, b ) ? bf : af;
      }
      float _yy_f32_copysign(float af, float bf)
      {
          float32_t a = to_softfloat32(af);
          float32_t b = to_softfloat32(bf);
          uint32_t sign_of_b = b.v >> 31;
          a.v &= ~(1 << 31);             // clear the sign bit
          a.v = a.v | (sign_of_b << 31); // add the sign of b
          return from_softfloat32(a);
      }
      // float unops
      float _yy_f32_abs(float af)
      {
          float32_t a = to_softfloat32(af);
          a.v &= ~(1 << 31);
          return from_softfloat32(a);
      }
      float _yy_f32_neg(float af)
      {
          float32_t a = to_softfloat32(af);
          uint32_t sign = a.v >> 31;
          a.v &= ~(1 << 31);
          a.v |= (!sign << 31);
          return from_softfloat32(a);
      }
      float _yy_f32_sqrt(float a)
      {
          float32_t ret = f32_sqrt(to_softfloat32(a));
          return from_softfloat32(ret);
      }
      // ceil, floor, trunc and nearest are lifted from libc
      float _yy_f32_ceil(float af)
      {
          float32_t a = to_softfloat32(af);
          int e = (int) (a.v >> 23 & 0xFF) - 0X7F;
          uint32_t m;
          if (e >= 23)
              return af;
          if (e >= 0) {
              m = 0x007FFFFF >> e;
              if ((a.v & m) == 0)
                  return af;
              if (a.v >> 31 == 0)
                  a.v += m;
              a.v &= ~m;
          } else {
              if (a.v >> 31)
                  a.v = 0x80000000; // return -0.0f
              else if (a.v << 1)
                  a.v = 0x3F800000; // return 1.0f
          }

          return from_softfloat32(a);
      }
      float _yy_f32_floor(float af)
      {
          float32_t a = to_softfloat32(af);
          int e = (int) (a.v >> 23 & 0xFF) - 0X7F;
          uint32_t m;
          if (e >= 23)
              return af;
          if (e >= 0) {
              m = 0x007FFFFF >> e;
              if ((a.v & m) == 0)
                  return af;
              if (a.v >> 31)
                  a.v += m;
              a.v &= ~m;
          } else {
              if (a.v >> 31 == 0)
                  a.v = 0;
              else if (a.v << 1)
                  a.v = 0xBF800000; // return -1.0f
          }
          return from_softfloat32(a);
      }
      float _yy_f32_trunc(float af)
      {
          float32_t a = to_softfloat32(af);
          int e = (int) (a.v >> 23 & 0xff) - 0x7f + 9;
          uint32_t m;
          if (e >= 23 + 9)
              return af;
          if (e < 9)
              e = 1;
          m = -1U >> e;
          if ((a.v & m) == 0)
              return af;
          a.v &= ~m;
          return from_softfloat32(a);
      }
      float _yy_f32_nearest(float af)
      {
          float32_t a = to_softfloat32(af);
          int e = a.v >> 23 & 0xff;
          int s = a.v >> 31;
          float32_t y;
          if (e >= 0x7f + 23)
              return af;
          if (s)
              y = f32_add(f32_sub(a, float32_t{inv_float_eps}), float32_t{inv_float_eps});
          else
              y = f32_sub(f32_add(a, float32_t{inv_float_eps}), float32_t{inv_float_eps});
          if (f32_eq(y, {0}))
              return s ? -0.0f : 0.0f;
          return from_softfloat32(y);
      }

      // float relops
      bool _yy_f32_eq(float a, float b) { return f32_eq(to_softfloat32(a), to_softfloat32(b)); }
      bool _yy_f32_ne(float a, float b) { return !f32_eq(to_softfloat32(a), to_softfloat32(b)); }
      bool _yy_f32_lt(float a, float b) { return f32_lt(to_softfloat32(a), to_softfloat32(b)); }
      bool _yy_f32_le(float a, float b) { return f32_le(to_softfloat32(a), to_softfloat32(b)); }
      bool _yy_f32_gt(float af, float bf)
      {
          float32_t a = to_softfloat32(af);
          float32_t b = to_softfloat32(bf);
          if (is_nan(a))
              return false;
          if (is_nan(b))
              return false;
          return !f32_le(a, b);
      }
      bool _yy_f32_ge(float af, float bf)
      {
          float32_t a = to_softfloat32(af);
          float32_t b = to_softfloat32(bf);
          if (is_nan(a))
              return false;
          if (is_nan(b))
              return false;
          return !f32_lt(a, b);
      }

      // double binops
      double _yy_f64_add(double a, double b)
      {
          float64_t ret = f64_add(to_softfloat64(a), to_softfloat64(b));
          return from_softfloat64(ret);
      }
      double _yy_f64_sub(double a, double b)
      {
          float64_t ret = f64_sub(to_softfloat64(a), to_softfloat64(b));
          return from_softfloat64(ret);
      }
      double _yy_f64_div(double a, double b)
      {
          float64_t ret = f64_div(to_softfloat64(a), to_softfloat64(b));
          return from_softfloat64(ret);
      }
      double _yy_f64_mul(double a, double b)
      {
          float64_t ret = f64_mul(to_softfloat64(a), to_softfloat64(b));
          return from_softfloat64(ret);
      }
      double _yy_f64_min(double af, double bf)
      {
          float64_t a = to_softfloat64(af);
          float64_t b = to_softfloat64(bf);
          if (is_nan(a))
              return af;
          if (is_nan(b))
              return bf;
          if (sign_bit(a) != sign_bit(b))
              return sign_bit(a) ? af : bf;
          return f64_lt(a, b) ? af : bf;
      }
      double _yy_f64_max(double af, double bf)
      {
          float64_t a = to_softfloat64(af);
          float64_t b = to_softfloat64(bf);
          if (is_nan(a))
              return af;
          if (is_nan(b))
              return bf;
          if (sign_bit(a) != sign_bit(b))
              return sign_bit(a) ? bf : af;
          return f64_lt(a, b) ? bf : af;
      }
      double _yy_f64_copysign(double af, double bf)
      {
          float64_t a = to_softfloat64(af);
          float64_t b = to_softfloat64(bf);
          uint64_t sign_of_b = b.v >> 63;
          a.v &= ~(uint64_t(1) << 63);   // clear the sign bit
          a.v = a.v | (sign_of_b << 63); // add the sign of b
          return from_softfloat64(a);
      }

      // double unops
      double _yy_f64_abs(double af)
      {
          float64_t a = to_softfloat64(af);
          a.v &= ~(uint64_t(1) << 63);
          return from_softfloat64(a);
      }
      double _yy_f64_neg(double af)
      {
          float64_t a = to_softfloat64(af);
          uint64_t sign = a.v >> 63;
          a.v &= ~(uint64_t(1) << 63);
          a.v |= (uint64_t(!sign) << 63);
          return from_softfloat64(a);
      }
      double _yy_f64_sqrt(double a)
      {
          float64_t ret = f64_sqrt(to_softfloat64(a));
          return from_softfloat64(ret);
      }
      // ceil, floor, trunc and nearest are lifted from libc
      double _yy_f64_ceil(double af)
      {
          float64_t a = to_softfloat64(af);
          float64_t ret;
          int e = a.v >> 52 & 0x7ff;
          float64_t y;
          if (e >= 0x3ff + 52 || f64_eq(a, {0}))
              return af;
          /* y = int(x) - x, where int(x) is an integer neighbor of x */
          if (a.v >> 63)
              y = f64_sub(f64_add(f64_sub(a, float64_t{inv_double_eps}), float64_t{inv_double_eps}), a);
          else
              y = f64_sub(f64_sub(f64_add(a, float64_t{inv_double_eps}), float64_t{inv_double_eps}), a);
          /* special case because of non-nearest rounding modes */
          if (e <= 0x3ff - 1) {
              return a.v >> 63 ? -0.0 : 1.0; //float64_t{0x8000000000000000} : float64_t{0xBE99999A3F800000}; //either -0.0 or 1
          }
          if (f64_lt(y, to_softfloat64(0))) {
              ret = f64_add(f64_add(a, y), to_softfloat64(1)); // 0xBE99999A3F800000 } ); // plus 1
              return from_softfloat64(ret);
          }
          ret = f64_add(a, y);
          return from_softfloat64(ret);
      }
      double _yy_f64_floor(double af)
      {
          float64_t a = to_softfloat64(af);
          float64_t ret;
          int e = a.v >> 52 & 0x7FF;
          float64_t y;
          if (a.v == 0x8000000000000000) {
              return af;
          }
          if (e >= 0x3FF + 52 || a.v == 0) {
              return af;
          }
          if (a.v >> 63)
              y = f64_sub(f64_add(f64_sub(a, float64_t{inv_double_eps}), float64_t{inv_double_eps}), a);
          else
              y = f64_sub(f64_sub(f64_add(a, float64_t{inv_double_eps}), float64_t{inv_double_eps}), a);
          if (e <= 0x3FF - 1) {
              return a.v >> 63 ? -1.0 : 0.0; //float64_t{0xBFF0000000000000} : float64_t{0}; // -1 or 0
          }
          if (!f64_le(y, float64_t{0})) {
              ret = f64_sub(f64_add(a, y), to_softfloat64(1.0));
              return from_softfloat64(ret);
          }
          ret = f64_add(a, y);
          return from_softfloat64(ret);
      }
      double _yy_f64_trunc(double af)
      {
          float64_t a = to_softfloat64(af);
          int e = (int) (a.v >> 52 & 0x7ff) - 0x3ff + 12;
          uint64_t m;
          if (e >= 52 + 12)
              return af;
          if (e < 12)
              e = 1;
          m = -1ULL >> e;
          if ((a.v & m) == 0)
              return af;
          a.v &= ~m;
          return from_softfloat64(a);
      }

      double _yy_f64_nearest(double af)
      {
          float64_t a = to_softfloat64(af);
          int e = (a.v >> 52 & 0x7FF);
          int s = a.v >> 63;
          float64_t y;
          if (e >= 0x3FF + 52)
              return af;
          if (s)
              y = f64_add(f64_sub(a, float64_t{inv_double_eps}), float64_t{inv_double_eps});
          else
              y = f64_sub(f64_add(a, float64_t{inv_double_eps}), float64_t{inv_double_eps});
          if (f64_eq(y, float64_t{0}))
              return s ? -0.0 : 0.0;
          return from_softfloat64(y);
      }

      // double relops
      bool _yy_f64_eq( double a, double b ) { return f64_eq( to_softfloat64(a), to_softfloat64(b) ); }
      bool _yy_f64_ne(double a, double b) { return !f64_eq(to_softfloat64(a), to_softfloat64(b)); }
      bool _yy_f64_lt(double a, double b) { return f64_lt(to_softfloat64(a), to_softfloat64(b)); }
      bool _yy_f64_le(double a, double b) { return f64_le(to_softfloat64(a), to_softfloat64(b)); }
      bool _yy_f64_gt(double af, double bf)
      {
          float64_t a = to_softfloat64(af);
          float64_t b = to_softfloat64(bf);
          if (is_nan(a))
              return false;
          if (is_nan(b))
              return false;
          return !f64_le(a, b);
      }
      bool _yy_f64_ge(double af, double bf)
      {
          float64_t a = to_softfloat64(af);
          float64_t b = to_softfloat64(bf);
          if (is_nan(a))
              return false;
          if (is_nan(b))
              return false;
          return !f64_lt(a, b);
      }

      // float and double conversions
      double _yy_f32_promote(float a)
      {
          return from_softfloat64(f32_to_f64(to_softfloat32(a)));
      }
      float _yy_f64_demote(double a)
      {
          return from_softfloat32(f64_to_f32(to_softfloat64(a)));
      }
      int32_t _yy_f32_trunc_i32s(float af)
      {
          float32_t a = to_softfloat32(af);
          if (_yy_f32_ge(af, 2147483648.0f) || _yy_f32_lt(af, -2147483648.0f))
              FC_THROW_EXCEPTION(graphene::chain::wasm_execution_error, "Error, f32.convert_s/i32 overflow");

          if (is_nan(a))
              FC_THROW_EXCEPTION(graphene::chain::wasm_execution_error, "Error, f32.convert_s/i32 unrepresentable");
          return f32_to_i32(to_softfloat32(_yy_f32_trunc(af)), 0, false);
      }
      int32_t _yy_f64_trunc_i32s(double af)
      {
          float64_t a = to_softfloat64(af);
          if (_yy_f64_ge(af, 2147483648.0) || _yy_f64_lt(af, -2147483648.0))
              FC_THROW_EXCEPTION(graphene::chain::wasm_execution_error, "Error, f64.convert_s/i32 overflow");
          if (is_nan(a))
              FC_THROW_EXCEPTION(graphene::chain::wasm_execution_error, "Error, f64.convert_s/i32 unrepresentable");
          return f64_to_i32(to_softfloat64(_yy_f64_trunc(af)), 0, false);
      }
      uint32_t _yy_f32_trunc_i32u(float af)
      {
          float32_t a = to_softfloat32(af);
          if (_yy_f32_ge(af, 4294967296.0f) || _yy_f32_le(af, -1.0f))
              FC_THROW_EXCEPTION(graphene::chain::wasm_execution_error, "Error, f32.convert_u/i32 overflow");
          if (is_nan(a))
              FC_THROW_EXCEPTION(graphene::chain::wasm_execution_error, "Error, f32.convert_u/i32 unrepresentable");
          return f32_to_ui32(to_softfloat32(_yy_f32_trunc(af)), 0, false);
      }
      uint32_t _yy_f64_trunc_i32u(double af)
      {
          float64_t a = to_softfloat64(af);
          if (_yy_f64_ge(af, 4294967296.0) || _yy_f64_le(af, -1.0))
              FC_THROW_EXCEPTION(graphene::chain::wasm_execution_error, "Error, f64.convert_u/i32 overflow");
          if (is_nan(a))
              FC_THROW_EXCEPTION(graphene::chain::wasm_execution_error, "Error, f64.convert_u/i32 unrepresentable");
          return f64_to_ui32(to_softfloat64(_yy_f64_trunc(af)), 0, false);
      }
      int64_t _yy_f32_trunc_i64s(float af)
      {
          float32_t a = to_softfloat32(af);
          if (_yy_f32_ge(af, 9223372036854775808.0f) || _yy_f32_lt(af, -9223372036854775808.0f))
              FC_THROW_EXCEPTION(graphene::chain::wasm_execution_error, "Error, f32.convert_s/i64 overflow");
          if (is_nan(a))
              FC_THROW_EXCEPTION(graphene::chain::wasm_execution_error, "Error, f32.convert_s/i64 unrepresentable");
          return f32_to_i64(to_softfloat32(_yy_f32_trunc(af)), 0, false);
      }
      int64_t _yy_f64_trunc_i64s(double af)
      {
          float64_t a = to_softfloat64(af);
          if (_yy_f64_ge(af, 9223372036854775808.0) || _yy_f64_lt(af, -9223372036854775808.0))
              FC_THROW_EXCEPTION(graphene::chain::wasm_execution_error, "Error, f64.convert_s/i64 overflow");
          if (is_nan(a))
              FC_THROW_EXCEPTION(graphene::chain::wasm_execution_error, "Error, f64.convert_s/i64 unrepresentable");

          return f64_to_i64(to_softfloat64(_yy_f64_trunc(af)), 0, false);
      }
      uint64_t _yy_f32_trunc_i64u(float af)
      {
          float32_t a = to_softfloat32(af);
          if (_yy_f32_ge(af, 18446744073709551616.0f) || _yy_f32_le(af, -1.0f))
              FC_THROW_EXCEPTION(graphene::chain::wasm_execution_error, "Error, f32.convert_u/i64 overflow");
          if (is_nan(a))
              FC_THROW_EXCEPTION(graphene::chain::wasm_execution_error, "Error, f32.convert_u/i64 unrepresentable");
          return f32_to_ui64(to_softfloat32(_yy_f32_trunc(af)), 0, false);
      }
      uint64_t _yy_f64_trunc_i64u(double af)
      {
          float64_t a = to_softfloat64(af);
          if (_yy_f64_ge(af, 18446744073709551616.0) || _yy_f64_le(af, -1.0))
              FC_THROW_EXCEPTION(graphene::chain::wasm_execution_error, "Error, f64.convert_u/i64 overflow");
          if (is_nan(a))
              FC_THROW_EXCEPTION(graphene::chain::wasm_execution_error, "Error, f64.convert_u/i64 unrepresentable");
          return f64_to_ui64(to_softfloat64(_yy_f64_trunc(af)), 0, false);
      }
      float _yy_i32_to_f32(int32_t a)
      {
          return from_softfloat32(i32_to_f32(a));
      }
      float _yy_i64_to_f32(int64_t a)
      {
          return from_softfloat32(i64_to_f32(a));
      }
      float _yy_ui32_to_f32(uint32_t a)
      {
          return from_softfloat32(ui32_to_f32(a));
      }
      float _yy_ui64_to_f32(uint64_t a)
      {
          return from_softfloat32(ui64_to_f32(a));
      }
      double _yy_i32_to_f64(int32_t a)
      {
          return from_softfloat64(i32_to_f64(a));
      }
      double _yy_i64_to_f64(int64_t a)
      {
          return from_softfloat64(i64_to_f64(a));
      }
      double _yy_ui32_to_f64(uint32_t a)
      {
          return from_softfloat64(ui32_to_f64(a));
      }
      double _yy_ui64_to_f64(uint64_t a)
      {
          return from_softfloat64(ui64_to_f64(a));
      }

      static bool is_nan(const float32_t f)
      {
          return ((f.v & 0x7FFFFFFF) > 0x7F800000);
      }
      static bool is_nan(const float64_t f)
      {
          return ((f.v & 0x7FFFFFFFFFFFFFFF) > 0x7FF0000000000000);
      }
      static bool is_nan(const float128_t &f)
      {
          return (((~(f.v[1]) & uint64_t(0x7FFF000000000000)) == 0) && (f.v[0] || ((f.v[1]) & uint64_t(0x0000FFFFFFFFFFFF))));
      }
      static float32_t to_softfloat32(float f)
      {
          return *reinterpret_cast<float32_t *>(&f);
      }
      static float64_t to_softfloat64(double d)
      {
          return *reinterpret_cast<float64_t *>(&d);
      }
      static float from_softfloat32(float32_t f)
      {
          return *reinterpret_cast<float *>(&f);
      }
      static double from_softfloat64(float64_t d)
      {
          return *reinterpret_cast<double *>(&d);
      }
      static constexpr uint32_t inv_float_eps = 0x4B000000;
      static constexpr uint64_t inv_double_eps = 0x4330000000000000;

      static bool sign_bit(float32_t f) { return f.v >> 31; }
      static bool sign_bit(float64_t f) { return f.v >> 63; }
};

#define DB_API_METHOD_WRAPPERS_SIMPLE_SECONDARY(IDX, TYPE)\
      int db_##IDX##_store( uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, const TYPE& secondary ) {\
         return context.IDX.store( scope, table, payer, id, secondary );\
      }\
      void db_##IDX##_update( int iterator, uint64_t payer, const TYPE& secondary ) {\
         return context.IDX.update( iterator, payer, secondary );\
      }\
      void db_##IDX##_remove( int iterator ) {\
         return context.IDX.remove( iterator );\
      }\
      int db_##IDX##_find_secondary( uint64_t code, uint64_t scope, uint64_t table, const TYPE& secondary, uint64_t& primary ) {\
         return context.IDX.find_secondary(code, scope, table, secondary, primary);\
      }\
      int db_##IDX##_find_primary( uint64_t code, uint64_t scope, uint64_t table, TYPE& secondary, uint64_t primary ) {\
         return context.IDX.find_primary(code, scope, table, secondary, primary);\
      }\
      int db_##IDX##_lowerbound( uint64_t code, uint64_t scope, uint64_t table,  TYPE& secondary, uint64_t& primary ) {\
         return context.IDX.lowerbound_secondary(code, scope, table, secondary, primary);\
      }\
      int db_##IDX##_upperbound( uint64_t code, uint64_t scope, uint64_t table,  TYPE& secondary, uint64_t& primary ) {\
         return context.IDX.upperbound_secondary(code, scope, table, secondary, primary);\
      }\
      int db_##IDX##_end( uint64_t code, uint64_t scope, uint64_t table ) {\
         return context.IDX.end_secondary(code, scope, table);\
      }\
      int db_##IDX##_next( int iterator, uint64_t& primary  ) {\
         return context.IDX.next_secondary(iterator, primary);\
      }\
      int db_##IDX##_previous( int iterator, uint64_t& primary ) {\
         return context.IDX.previous_secondary(iterator, primary);\
      }

#define DB_API_METHOD_WRAPPERS_ARRAY_SECONDARY(IDX, ARR_SIZE, ARR_ELEMENT_TYPE)\
      int db_##IDX##_store( uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, array_ptr<const ARR_ELEMENT_TYPE> data, size_t data_len) {\
         FC_ASSERT( data_len == ARR_SIZE,\
                    "invalid size of secondary key array for " #IDX ": given ${given} bytes but expected ${expected} bytes",\
                    ("given",data_len)("expected",ARR_SIZE) );\
         return context.IDX.store(scope, table, payer, id, data.value);\
      }\
      void db_##IDX##_update( int iterator, uint64_t payer, array_ptr<const ARR_ELEMENT_TYPE> data, size_t data_len ) {\
         FC_ASSERT( data_len == ARR_SIZE,\
                    "invalid size of secondary key array for " #IDX ": given ${given} bytes but expected ${expected} bytes",\
                    ("given",data_len)("expected",ARR_SIZE) );\
         return context.IDX.update(iterator, payer, data.value);\
      }\
      void db_##IDX##_remove( int iterator ) {\
         return context.IDX.remove(iterator);\
      }\
      int db_##IDX##_find_secondary( uint64_t code, uint64_t scope, uint64_t table, array_ptr<const ARR_ELEMENT_TYPE> data, size_t data_len, uint64_t& primary ) {\
         FC_ASSERT( data_len == ARR_SIZE,\
                    "invalid size of secondary key array for " #IDX ": given ${given} bytes but expected ${expected} bytes",\
                    ("given",data_len)("expected",ARR_SIZE) );\
         return context.IDX.find_secondary(code, scope, table, data, primary);\
      }\
      int db_##IDX##_find_primary( uint64_t code, uint64_t scope, uint64_t table, array_ptr<ARR_ELEMENT_TYPE> data, size_t data_len, uint64_t primary ) {\
         FC_ASSERT( data_len == ARR_SIZE,\
                    "invalid size of secondary key array for " #IDX ": given ${given} bytes but expected ${expected} bytes",\
                    ("given",data_len)("expected",ARR_SIZE) );\
         return context.IDX.find_primary(code, scope, table, data.value, primary);\
      }\
      int db_##IDX##_lowerbound( uint64_t code, uint64_t scope, uint64_t table, array_ptr<ARR_ELEMENT_TYPE> data, size_t data_len, uint64_t& primary ) {\
         FC_ASSERT( data_len == ARR_SIZE,\
                    "invalid size of secondary key array for " #IDX ": given ${given} bytes but expected ${expected} bytes",\
                    ("given",data_len)("expected",ARR_SIZE) );\
         return context.IDX.lowerbound_secondary(code, scope, table, data.value, primary);\
      }\
      int db_##IDX##_upperbound( uint64_t code, uint64_t scope, uint64_t table, array_ptr<ARR_ELEMENT_TYPE> data, size_t data_len, uint64_t& primary ) {\
         FC_ASSERT( data_len == ARR_SIZE,\
                    "invalid size of secondary key array for " #IDX ": given ${given} bytes but expected ${expected} bytes",\
                    ("given",data_len)("expected",ARR_SIZE) );\
         return context.IDX.upperbound_secondary(code, scope, table, data.value, primary);\
      }\
      int db_##IDX##_end( uint64_t code, uint64_t scope, uint64_t table ) {\
         return context.IDX.end_secondary(code, scope, table);\
      }\
      int db_##IDX##_next( int iterator, uint64_t& primary  ) {\
         return context.IDX.next_secondary(iterator, primary);\
      }\
      int db_##IDX##_previous( int iterator, uint64_t& primary ) {\
         return context.IDX.previous_secondary(iterator, primary);\
      }

#define DB_API_METHOD_WRAPPERS_FLOAT_SECONDARY(IDX, TYPE)\
      int db_##IDX##_store( uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, const TYPE& secondary ) {\
         GRAPHENE_ASSERT( !softfloat_api::is_nan( secondary ), transaction_exception, "NaN is not an allowed value for a secondary key" );\
         return context.IDX.store( scope, table, payer, id, secondary );\
      }\
      void db_##IDX##_update( int iterator, uint64_t payer, const TYPE& secondary ) {\
         GRAPHENE_ASSERT( !softfloat_api::is_nan( secondary ), transaction_exception, "NaN is not an allowed value for a secondary key" );\
         return context.IDX.update( iterator, payer, secondary );\
      }\
      void db_##IDX##_remove( int iterator ) {\
         return context.IDX.remove( iterator );\
      }\
      int db_##IDX##_find_secondary( uint64_t code, uint64_t scope, uint64_t table, const TYPE& secondary, uint64_t& primary ) {\
         GRAPHENE_ASSERT( !softfloat_api::is_nan( secondary ), transaction_exception, "NaN is not an allowed value for a secondary key" );\
         return context.IDX.find_secondary(code, scope, table, secondary, primary);\
      }\
      int db_##IDX##_find_primary( uint64_t code, uint64_t scope, uint64_t table, TYPE& secondary, uint64_t primary ) {\
         return context.IDX.find_primary(code, scope, table, secondary, primary);\
      }\
      int db_##IDX##_lowerbound( uint64_t code, uint64_t scope, uint64_t table,  TYPE& secondary, uint64_t& primary ) {\
         GRAPHENE_ASSERT( !softfloat_api::is_nan( secondary ), transaction_exception, "NaN is not an allowed value for a secondary key" );\
         return context.IDX.lowerbound_secondary(code, scope, table, secondary, primary);\
      }\
      int db_##IDX##_upperbound( uint64_t code, uint64_t scope, uint64_t table,  TYPE& secondary, uint64_t& primary ) {\
         GRAPHENE_ASSERT( !softfloat_api::is_nan( secondary ), transaction_exception, "NaN is not an allowed value for a secondary key" );\
         return context.IDX.upperbound_secondary(code, scope, table, secondary, primary);\
      }\
      int db_##IDX##_end( uint64_t code, uint64_t scope, uint64_t table ) {\
         return context.IDX.end_secondary(code, scope, table);\
      }\
      int db_##IDX##_next( int iterator, uint64_t& primary  ) {\
         return context.IDX.next_secondary(iterator, primary);\
      }\
      int db_##IDX##_previous( int iterator, uint64_t& primary ) {\
         return context.IDX.previous_secondary(iterator, primary);\
      }

class database_api : public context_aware_api {
   public:
      using context_aware_api::context_aware_api;

      int db_store_i64( uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, array_ptr<const char> buffer, size_t buffer_size ) {
         return context.db_store_i64( scope, table, payer, id, buffer, buffer_size );
      }
      void db_update_i64( int itr, uint64_t payer, array_ptr<const char> buffer, size_t buffer_size ) {
         context.db_update_i64( itr, payer, buffer, buffer_size );
      }
      void db_remove_i64( int itr ) {
         context.db_remove_i64( itr );
      }
      int db_get_i64( int itr, array_ptr<char> buffer, size_t buffer_size ) {
         return context.db_get_i64( itr, buffer, buffer_size );
      }
      int db_next_i64( int itr, uint64_t& primary ) {
         return context.db_next_i64(itr, primary);
      }
      int db_previous_i64( int itr, uint64_t& primary ) {
         return context.db_previous_i64(itr, primary);
      }
      int db_find_i64( uint64_t code, uint64_t scope, uint64_t table, uint64_t id ) {
         return context.db_find_i64( code, scope, table, id );
      }
      int db_lowerbound_i64( uint64_t code, uint64_t scope, uint64_t table, uint64_t id ) {
         return context.db_lowerbound_i64( code, scope, table, id );
      }
      int db_upperbound_i64( uint64_t code, uint64_t scope, uint64_t table, uint64_t id ) {
         return context.db_upperbound_i64( code, scope, table, id );
      }
      int db_end_i64( uint64_t code, uint64_t scope, uint64_t table ) {
         return context.db_end_i64( code, scope, table );
      }

      DB_API_METHOD_WRAPPERS_SIMPLE_SECONDARY(idx64,  uint64_t)
};


class memory_api : public context_aware_api
{
  public:
    memory_api(apply_context &ctx)
        : context_aware_api(ctx, true)
    {
    }

    char *memcpy(array_ptr<char> dest, array_ptr<const char> src, size_t length)
    {
        GRAPHENE_ASSERT((std::abs((ptrdiff_t) dest.value - (ptrdiff_t) src.value)) >= length,
                overlapping_memory_error, "memcpy can only accept non-aliasing pointers");
        return (char *) ::memcpy(dest, src, length);
    }

    char *memmove(array_ptr<char> dest, array_ptr<const char> src, size_t length)
    {
        return (char *) ::memmove(dest, src, length);
    }

    int memcmp(array_ptr<const char> dest, array_ptr<const char> src, size_t length)
    {
        int ret = ::memcmp(dest, src, length);
        if (ret < 0)
            return -1;
        if(ret > 0)
            return 1;
        return 0;
    }

    char *memset(array_ptr<char> dest, int value, size_t length)
    {
        return (char *)::memset( dest, value, length );
    }
};

class transaction_api : public context_aware_api {
   public:
      using context_aware_api::context_aware_api;

      void send_inline(array_ptr<char> data, size_t data_len) {
         uint32_t max_inline_action_size = context.trx_context.get_inter_contract_calling_params().max_inline_action_size;
         FC_ASSERT(data_len <= max_inline_action_size, "inline action too big, max size=${s} bytes", ("s", max_inline_action_size));

         context.trx_context.check_inter_contract_depth();

         action act;
         fc::raw::unpack<action>(data, data_len, act, 20);

         // check action sender
         FC_ASSERT(act.sender == context.receiver,
                 "the sender must be current contract, actually act.sender=${s}, current receiver=${r}", ("s", act.sender)("r", context.receiver));
         // check amount
         FC_ASSERT(act.amount.amount >=0, "action amount must >= 0, actual amount: ${a}", ("a", act.amount.amount));

         // check action contract code
         const account_object& contract_obj = context._db->get_account_by_uid(act.contract_id);
         FC_ASSERT(contract_obj.code.size() > 0, "inline action's code account ${account} does not exist", ("account", act.contract_id));

         // check method_name, must be payable
         const auto &actions = contract_obj.abi.actions;
         auto iter = std::find_if(actions.begin(), actions.end(),
                 [&](const action_def &act_def) { return act_def.name == act.method_name; });
         FC_ASSERT(iter != actions.end(), "method_name ${m} not found in abi", ("m", act.method_name));
         if (act.amount.amount > 0) {
             FC_ASSERT(iter->payable, "method_name ${m} not payable", ("m", act.method_name));
         }

         inter_contract_call_operation op;
         op.fee = asset{0, context._db->get_core_asset().asset_id};
         if(act.amount.amount > 0)
             op.amount = asset{act.amount.amount, asset_aid_type(act.amount.asset_id)};
         op.contract_id = account_uid_type(act.contract_id);
         op.data = act.data;
         op.method_name = act.method_name;
         op.sender_contract = account_uid_type(context.receiver);
         context.execute_inline(std::move(op));
      }
};

class context_free_transaction_api : public context_aware_api {
   public:
      context_free_transaction_api( apply_context& ctx )
      :context_aware_api(ctx,true){}

      int read_transaction(array_ptr<char> data, size_t buffer_size)
      {
          const transaction* cur_trx = context.db().get_cur_trx();
          FC_ASSERT(nullptr != cur_trx, "current transaction not set");
          bytes trx = fc::raw::pack(*cur_trx);

          auto s = trx.size();
          if (buffer_size == 0) return s;

          auto copy_size = std::min(buffer_size, s);
          memcpy(data, trx.data(), copy_size);

          return copy_size;
      }

      int transaction_size() {
          const transaction* trx = context.db().get_cur_trx();
          FC_ASSERT(nullptr != trx, "current transaction not set");
          return fc::raw::pack(*trx).size();
      }

      uint64_t expiration() {
          return context.db().get_cur_trx()->expiration.sec_since_epoch();
      }

      int tapos_block_num() {
        return context.db().get_cur_trx()->ref_block_num;
      }
      uint64_t tapos_block_prefix() {
        return context.db().get_cur_trx()->ref_block_prefix;
      }
};

class compiler_builtins : public context_aware_api {
   public:
      compiler_builtins( apply_context& ctx )
      :context_aware_api(ctx,true){}

      void __ashlti3(__int128& ret, uint64_t low, uint64_t high, uint32_t shift) {
         fc::uint128_t i=fc::uint128(high, low);
         i <<= shift;
         ret = (unsigned __int128)i;
      }

      void __ashrti3(__int128& ret, uint64_t low, uint64_t high, uint32_t shift) {
         // retain the signedness
         ret = high;
         ret <<= 64;
         ret |= low;
         ret >>= shift;
      }

      void __lshlti3(__int128& ret, uint64_t low, uint64_t high, uint32_t shift) {
         fc::uint128_t i=fc::uint128(high, low);
         i <<= shift;
         ret = (unsigned __int128)i;
      }

      void __lshrti3(__int128& ret, uint64_t low, uint64_t high, uint32_t shift) {
         fc::uint128_t i=fc::uint128(high, low);
         i >>= shift;
         ret = (unsigned __int128)i;
      }

      void __divti3(__int128& ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) {
         __int128 lhs = ha;
         __int128 rhs = hb;

         lhs <<= 64;
         lhs |=  la;

         rhs <<= 64;
         rhs |=  lb;

         FC_ASSERT(rhs != 0, "divide by zero");

         lhs /= rhs;

         ret = lhs;
      }

      void __udivti3(unsigned __int128& ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) {
         unsigned __int128 lhs = ha;
         unsigned __int128 rhs = hb;

         lhs <<= 64;
         lhs |=  la;

         rhs <<= 64;
         rhs |=  lb;

         FC_ASSERT(rhs != 0, "divide by zero");

         lhs /= rhs;
         ret = lhs;
      }

      void __multi3(__int128& ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) {
         __int128 lhs = ha;
         __int128 rhs = hb;

         lhs <<= 64;
         lhs |=  la;

         rhs <<= 64;
         rhs |=  lb;

         lhs *= rhs;
         ret = lhs;
      }

      void __modti3(__int128& ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) {
         __int128 lhs = ha;
         __int128 rhs = hb;

         lhs <<= 64;
         lhs |=  la;

         rhs <<= 64;
         rhs |=  lb;

         FC_ASSERT(rhs != 0, "divide by zero");

         lhs %= rhs;
         ret = lhs;
      }

      void __umodti3(unsigned __int128& ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) {
         unsigned __int128 lhs = ha;
         unsigned __int128 rhs = hb;

         lhs <<= 64;
         lhs |=  la;

         rhs <<= 64;
         rhs |=  lb;

         FC_ASSERT(rhs != 0, "divide by zero");

         lhs %= rhs;
         ret = lhs;
      }

      // arithmetic long double
      void __addtf3( float128_t& ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
         float128_t a = {{ la, ha }};
         float128_t b = {{ lb, hb }};
         ret = f128_add( a, b );
      }
      void __subtf3( float128_t& ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
         float128_t a = {{ la, ha }};
         float128_t b = {{ lb, hb }};
         ret = f128_sub( a, b );
      }
      void __multf3( float128_t& ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
         float128_t a = {{ la, ha }};
         float128_t b = {{ lb, hb }};
         ret = f128_mul( a, b );
      }
      void __divtf3( float128_t& ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
         float128_t a = {{ la, ha }};
         float128_t b = {{ lb, hb }};
         ret = f128_div( a, b );
      }
      void __negtf2( float128_t& ret, uint64_t la, uint64_t ha ) {
         ret = {{ la, (ha ^ (uint64_t)1 << 63) }};
      }

      // conversion long double
      void __extendsftf2( float128_t& ret, float f ) {
         ret = f32_to_f128( softfloat_api::to_softfloat32(f) );
      }
      void __extenddftf2( float128_t& ret, double d ) {
         ret = f64_to_f128( softfloat_api::to_softfloat64(d) );
      }
      double __trunctfdf2( uint64_t l, uint64_t h ) {
         float128_t f = {{ l, h }};
         return softfloat_api::from_softfloat64(f128_to_f64( f ));
      }
      float __trunctfsf2( uint64_t l, uint64_t h ) {
         float128_t f = {{ l, h }};
         return softfloat_api::from_softfloat32(f128_to_f32( f ));
      }
      int32_t __fixtfsi( uint64_t l, uint64_t h ) {
         float128_t f = {{ l, h }};
         return f128_to_i32( f, 0, false );
      }
      int64_t __fixtfdi( uint64_t l, uint64_t h ) {
         float128_t f = {{ l, h }};
         return f128_to_i64( f, 0, false );
      }
      void __fixtfti( __int128& ret, uint64_t l, uint64_t h ) {
         float128_t f = {{ l, h }};
         ret = ___fixtfti( f );
      }
      uint32_t __fixunstfsi( uint64_t l, uint64_t h ) {
         float128_t f = {{ l, h }};
         return f128_to_ui32( f, 0, false );
      }
      uint64_t __fixunstfdi( uint64_t l, uint64_t h ) {
         float128_t f = {{ l, h }};
         return f128_to_ui64( f, 0, false );
      }
      void __fixunstfti( unsigned __int128& ret, uint64_t l, uint64_t h ) {
         float128_t f = {{ l, h }};
         ret = ___fixunstfti( f );
      }
      void __fixsfti( __int128& ret, float a ) {
         ret = ___fixsfti( softfloat_api::to_softfloat32(a).v );
      }
      void __fixdfti( __int128& ret, double a ) {
         ret = ___fixdfti( softfloat_api::to_softfloat64(a).v );
      }
      void __fixunssfti( unsigned __int128& ret, float a ) {
         ret = ___fixunssfti( softfloat_api::to_softfloat32(a).v );
      }
      void __fixunsdfti( unsigned __int128& ret, double a ) {
         ret = ___fixunsdfti( softfloat_api::to_softfloat64(a).v );
      }
      double __floatsidf( int32_t i ) {
         return softfloat_api::from_softfloat64(i32_to_f64(i));
      }
      void __floatsitf( float128_t& ret, int32_t i ) {
         ret = i32_to_f128(i);
      }
      void __floatditf( float128_t& ret, uint64_t a ) {
         ret = i64_to_f128( a );
      }
      void __floatunsitf( float128_t& ret, uint32_t i ) {
         ret = ui32_to_f128(i);
      }
      void __floatunditf( float128_t& ret, uint64_t a ) {
         ret = ui64_to_f128( a );
      }
      double __floattidf( uint64_t l, uint64_t h ) {
         fc::uint128_t v=fc::uint128(h, l);
         unsigned __int128 val = (unsigned __int128)v;
         return ___floattidf( *(__int128*)&val );
      }
      double __floatuntidf( uint64_t l, uint64_t h ) {
         fc::uint128_t v=fc::uint128(h, l);
         return ___floatuntidf( (unsigned __int128)v );
      }
      int ___cmptf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb, int return_value_if_nan ) {
         float128_t a = {{ la, ha }};
         float128_t b = {{ lb, hb }};
         if ( __unordtf2(la, ha, lb, hb) )
            return return_value_if_nan;
         if ( f128_lt( a, b ) )
            return -1;
         if ( f128_eq( a, b ) )
            return 0;
         return 1;
      }
      int __eqtf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
         return ___cmptf2(la, ha, lb, hb, 1);
      }
      int __netf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
         return ___cmptf2(la, ha, lb, hb, 1);
      }
      int __getf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
         return ___cmptf2(la, ha, lb, hb, -1);
      }
      int __gttf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
         return ___cmptf2(la, ha, lb, hb, 0);
      }
      int __letf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
         return ___cmptf2(la, ha, lb, hb, 1);
      }
      int __lttf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
         return ___cmptf2(la, ha, lb, hb, 0);
      }
      int __cmptf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
         return ___cmptf2(la, ha, lb, hb, 1);
      }
      int __unordtf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
         float128_t a = {{ la, ha }};
         float128_t b = {{ lb, hb }};
         if ( softfloat_api::is_nan(a) || softfloat_api::is_nan(b) )
            return 1;
         return 0;
      }

      static constexpr uint32_t SHIFT_WIDTH = (sizeof(uint64_t)*8)-1;
};

class console_api : public context_aware_api
{
  public:
    console_api(apply_context &ctx)
        : context_aware_api(ctx, true)
        , ignore(false)
    {
    }

    // Kept as intrinsic rather than implementing on WASM side (using prints_l and strlen) because strlen is faster on native side.
    void prints(null_terminated_ptr str)
    {
        if (!ignore) {
            context.console_append<const char*>(str);
         }
      }

      void prints_l(array_ptr<const char> str, size_t str_len ) {
         if ( !ignore ) {
            context.console_append(string(str, str_len));
         }
      }

      void printi(int64_t val) {
         if ( !ignore ) {
            context.console_append(val);
         }
      }

      void printui(uint64_t val) {
         if ( !ignore ) {
            context.console_append(val);
         }
      }

      void printi128(const __int128& val) {
         if ( !ignore ) {
            bool is_negative = (val < 0);
            unsigned __int128 val_magnitude;

            if( is_negative )
               val_magnitude = static_cast<unsigned __int128>(-val); // Works even if val is at the lowest possible value of a int128_t
            else
               val_magnitude = static_cast<unsigned __int128>(val);

			fc::bigint v = static_cast<uint64_t>(val_magnitude>>64);
			v <<=64;
			v +=static_cast<uint64_t>(val_magnitude);

            if( is_negative ) {
               context.console_append("-");
            }

            context.console_append(fc::variant(v).get_string());
         }
      }

      void printui128(const unsigned __int128& val) {
         if ( !ignore ) {
            fc::bigint v = static_cast<uint64_t>(val>>64);
			v <<=64;
			v +=static_cast<uint64_t>(val) ;
            context.console_append(fc::variant(v).get_string());
         }
      }

      void printsf( float val ) {
         if ( !ignore ) {
            // Assumes float representation on native side is the same as on the WASM side
            auto& console = context.get_console_stream();
            auto orig_prec = console.precision();

            console.precision( std::numeric_limits<float>::digits10 );
            context.console_append(val);

            console.precision( orig_prec );
         }
      }

      void printdf( double val ) {
         if ( !ignore ) {
            // Assumes double representation on native side is the same as on the WASM side
            auto& console = context.get_console_stream();
            auto orig_prec = console.precision();

            console.precision( std::numeric_limits<double>::digits10 );
            context.console_append(val);

            console.precision( orig_prec );
         }
      }

      void printqf( const float128_t& val ) {
         /*
          * Native-side long double uses an 80-bit extended-precision floating-point number.
          * The easiest solution for now was to use the Berkeley softfloat library to round the 128-bit
          * quadruple-precision floating-point number to an 80-bit extended-precision floating-point number
          * (losing precision) which then allows us to simply cast it into a long double for printing purposes.
          *
          * Later we might find a better solution to print the full quadruple-precision floating-point number.
          * Maybe with some compilation flag that turns long double into a quadruple-precision floating-point number,
          * or maybe with some library that allows us to print out quadruple-precision floating-point numbers without
          * having to deal with long doubles at all.
          */

         if ( !ignore ) {
            auto& console = context.get_console_stream();
            auto orig_prec = console.precision();

            console.precision( std::numeric_limits<long double>::digits10 );

            extFloat80_t val_approx;
            f128M_to_extF80M(&val, &val_approx);
            context.console_append( *(long double*)(&val_approx) );

            console.precision( orig_prec );
         }
      }

      void printn(const name& value) {
         if ( !ignore ) {
            context.console_append(value.to_string());
         }
      }

      void printhex(array_ptr<const char> data, size_t data_len ) {
         if ( !ignore ) {
            context.console_append(fc::to_hex(data, data_len));
         }
      }

   private:
      bool ignore;
};

class asset_api : public context_aware_api
{
  public:
    asset_api(apply_context &ctx)
        : context_aware_api(ctx, true)
    {}

    void withdraw_asset(int64_t from, int64_t to, int64_t asset_id, int64_t amount)
    {
        FC_ASSERT(from == context.receiver, "can only withdraw from contract ${c}", ("c", context.receiver));
        FC_ASSERT(from != to, "cannot transfer to self");
        FC_ASSERT(amount > 0, "withdraw  amount ${a} must >= 0", ("a", amount));
        FC_ASSERT(from >= 0, "account id ${a} from must  >= 0", ("a", from));
        FC_ASSERT(to >= 0, "account id ${a} to must >= 0", ("a", to));
        FC_ASSERT(asset_id >= 0, "asset id ${a} must >= 0", ("a", asset_id));

        auto &d = context.db();
        asset a{amount, asset_aid_type(asset_id & GRAPHENE_DB_MAX_INSTANCE_ID)};
        account_uid_type from_account = account_uid_type(from & GRAPHENE_DB_MAX_INSTANCE_ID);
        account_uid_type to_account = account_uid_type(to & GRAPHENE_DB_MAX_INSTANCE_ID);
        FC_ASSERT(d.get_balance(from_account, a.asset_id).amount >= amount, "insufficient balance ${b}, unable to withdraw ${a} from account ${c}", ("b", d.to_pretty_string(d.get_balance(from_account, a.asset_id)))("a", amount)("c", from_account));

        // adjust balance
        transaction_evaluation_state op_context(&d);
        op_context.skip_fee_schedule_check = true;

        inline_transfer_operation op;
        op.amount = a;
        op.from = from_account;
        op.to = to_account;
        op.fee = asset{0, d.get_core_asset().asset_id};
        d.apply_operation(op_context, op);
    }

    void inline_transfer(int64_t from, int64_t to, int64_t asset_id, int64_t amount, array_ptr<char> data, size_t datalen)
    {
        auto &d = context.db();

        FC_ASSERT(from == context.receiver, "can only transfer from contract ${c}", ("c", context.receiver));
        FC_ASSERT(from >= 0, "account id ${a} from must  >= 0", ("a", from));
        FC_ASSERT(to >= 0, "account id ${a} to must >= 0", ("a", to));
		FC_ASSERT(from != to, "cannot transfer to self");
        FC_ASSERT(asset_id >= 0, "asset id ${a} must >= 0", ("a", asset_id));

        asset a{amount, asset_aid_type(asset_id & GRAPHENE_DB_MAX_INSTANCE_ID)};
        account_uid_type from_account = account_uid_type(from & GRAPHENE_DB_MAX_INSTANCE_ID);
        account_uid_type to_account = account_uid_type(to & GRAPHENE_DB_MAX_INSTANCE_ID);

        std::string memo(data, datalen);

        // apply operation
        transaction_evaluation_state op_context(&d);
        op_context.skip_fee_schedule_check = true;
        inline_transfer_operation op;
        op.amount = a;
        op.from = from_account;
        op.to = to_account;
        op.memo = memo;
        op.fee = asset{0, d.get_core_asset().asset_id};
        d.apply_operation(op_context, op);
    }

    // get account balance by asset_id
    int64_t get_balance(int64_t account, int64_t asset_id)
    {
        FC_ASSERT(account >= 0, "account id must > 0");
        FC_ASSERT(asset_id >= 0, "asset id to must > 0");

        auto &d = context.db();
        auto account_id = account_uid_type(account & GRAPHENE_DB_MAX_INSTANCE_ID);
        auto aid = asset_aid_type(asset_id & GRAPHENE_DB_MAX_INSTANCE_ID);
        return d.get_balance(account_id, aid).amount.value;
    }

};

REGISTER_INJECTED_INTRINSICS(call_depth_api,
   (call_depth_assert,  void()               )
);

REGISTER_INTRINSICS(memory_api,
(memcpy,                 int(int, int, int))
(memmove,                int(int, int, int))
(memcmp,                 int(int, int, int))
(memset,                 int(int, int, int))
);

REGISTER_INTRINSICS(transaction_api,
(send_inline,               void(int, int))
);

REGISTER_INTRINSICS(context_free_transaction_api,
(read_transaction,               int(int, int))
(transaction_size,               int())
(expiration,                     int64_t())
(tapos_block_num,                int())
(tapos_block_prefix,             int64_t())
);

REGISTER_INTRINSICS(console_api,
(prints,                void(int)      )
(prints_l,              void(int, int) )
(printi,                void(int64_t)  )
(printui,               void(int64_t)  )
(printi128,             void(int)      )
(printui128,            void(int)      )
(printsf,               void(float)    )
(printdf,               void(double)   )
(printqf,               void(int)      )
(printn,                void(int64_t)  )
(printhex,              void(int, int) )
);

REGISTER_INTRINSICS(context_free_system_api,
(abort,                     void()              )
(graphene_assert,           void(int, int)      )
(graphene_assert_message,   void(int, int, int) )
(graphene_assert_code,      void(int, int64_t)  )
(graphene_exit,             void(int)           )
);

REGISTER_INTRINSICS(global_api,
(get_head_block_num,    int64_t()          )
(get_head_block_id,     void(int)          )
(get_block_id_for_num,  void(int, int)     )
(get_head_block_time,   int64_t()          )
(get_trx_sender,        int64_t()          )
(get_trx_origin,        int64_t()          )
(get_account_name_by_id,int64_t(int, int, int64_t))
(get_account_id,        int64_t(int, int)  )
(get_asset_id,          int64_t(int, int)  )
);

REGISTER_INTRINSICS(crypto_api,
(assert_recover_key,     void(int, int, int, int)      )
(verify_signature,       int(int, int, int, int, int)  )
(assert_sha256,          void(int, int, int)           )
(assert_sha1,            void(int, int, int)           )
(assert_sha512,          void(int, int, int)           )
(assert_ripemd160,       void(int, int, int)           )
(sha1,                   void(int, int, int)           )
(sha256,                 void(int, int, int)           )
(sha512,                 void(int, int, int)           )
(ripemd160,              void(int, int, int)           )
);

REGISTER_INTRINSICS(action_api,
(read_action_data,          int(int, int)  )
(action_data_size,          int()          )
(current_receiver,          int64_t()      )
(get_action_asset_id,       int64_t()      )
(get_action_asset_amount,   int64_t()      )
);

REGISTER_INTRINSICS(asset_api,
(withdraw_asset,                 void(int64_t, int64_t, int64_t, int64_t))
(inline_transfer,                void(int64_t, int64_t, int64_t, int64_t, int, int))
(get_balance,                    int64_t(int64_t, int64_t))
);

#define DB_SECONDARY_INDEX_METHODS_SIMPLE(IDX) \
   (db_##IDX##_store,          int(int64_t,int64_t,int64_t,int64_t,int))\
   (db_##IDX##_remove,         void(int))\
   (db_##IDX##_update,         void(int,int64_t,int))\
   (db_##IDX##_find_primary,   int(int64_t,int64_t,int64_t,int,int64_t))\
   (db_##IDX##_find_secondary, int(int64_t,int64_t,int64_t,int,int))\
   (db_##IDX##_lowerbound,     int(int64_t,int64_t,int64_t,int,int))\
   (db_##IDX##_upperbound,     int(int64_t,int64_t,int64_t,int,int))\
   (db_##IDX##_end,            int(int64_t,int64_t,int64_t))\
   (db_##IDX##_next,           int(int, int))\
   (db_##IDX##_previous,       int(int, int))

#define DB_SECONDARY_INDEX_METHODS_ARRAY(IDX) \
      (db_##IDX##_store,          int(int64_t,int64_t,int64_t,int64_t,int,int))\
      (db_##IDX##_remove,         void(int))\
      (db_##IDX##_update,         void(int,int64_t,int,int))\
      (db_##IDX##_find_primary,   int(int64_t,int64_t,int64_t,int,int,int64_t))\
      (db_##IDX##_find_secondary, int(int64_t,int64_t,int64_t,int,int,int))\
      (db_##IDX##_lowerbound,     int(int64_t,int64_t,int64_t,int,int,int))\
      (db_##IDX##_upperbound,     int(int64_t,int64_t,int64_t,int,int,int))\
      (db_##IDX##_end,            int(int64_t,int64_t,int64_t))\
      (db_##IDX##_next,           int(int, int))\
      (db_##IDX##_previous,       int(int, int))


REGISTER_INTRINSICS( database_api,
   (db_store_i64,        int(int64_t,int64_t,int64_t,int64_t,int,int))
   (db_update_i64,       void(int,int64_t,int,int))
   (db_remove_i64,       void(int))
   (db_get_i64,          int(int, int, int))
   (db_next_i64,         int(int, int))
   (db_previous_i64,     int(int, int))
   (db_find_i64,         int(int64_t,int64_t,int64_t,int64_t))
   (db_lowerbound_i64,   int(int64_t,int64_t,int64_t,int64_t))
   (db_upperbound_i64,   int(int64_t,int64_t,int64_t,int64_t))
   (db_end_i64,          int(int64_t,int64_t,int64_t))

   DB_SECONDARY_INDEX_METHODS_SIMPLE(idx64)
);

REGISTER_INJECTED_INTRINSICS(transaction_context,
(checktime,      void())
);

REGISTER_INTRINSICS(compiler_builtins,
   (__ashlti3,     void(int, int64_t, int64_t, int)               )
   (__ashrti3,     void(int, int64_t, int64_t, int)               )
   (__lshlti3,     void(int, int64_t, int64_t, int)               )
   (__lshrti3,     void(int, int64_t, int64_t, int)               )
   (__divti3,      void(int, int64_t, int64_t, int64_t, int64_t)  )
   (__udivti3,     void(int, int64_t, int64_t, int64_t, int64_t)  )
   (__modti3,      void(int, int64_t, int64_t, int64_t, int64_t)  )
   (__umodti3,     void(int, int64_t, int64_t, int64_t, int64_t)  )
   (__multi3,      void(int, int64_t, int64_t, int64_t, int64_t)  )
   (__addtf3,      void(int, int64_t, int64_t, int64_t, int64_t)  )
   (__subtf3,      void(int, int64_t, int64_t, int64_t, int64_t)  )
   (__multf3,      void(int, int64_t, int64_t, int64_t, int64_t)  )
   (__divtf3,      void(int, int64_t, int64_t, int64_t, int64_t)  )
   (__eqtf2,       int(int64_t, int64_t, int64_t, int64_t)        )
   (__netf2,       int(int64_t, int64_t, int64_t, int64_t)        )
   (__getf2,       int(int64_t, int64_t, int64_t, int64_t)        )
   (__gttf2,       int(int64_t, int64_t, int64_t, int64_t)        )
   (__lttf2,       int(int64_t, int64_t, int64_t, int64_t)        )
   (__letf2,       int(int64_t, int64_t, int64_t, int64_t)        )
   (__cmptf2,      int(int64_t, int64_t, int64_t, int64_t)        )
   (__unordtf2,    int(int64_t, int64_t, int64_t, int64_t)        )
   (__negtf2,      void (int, int64_t, int64_t)                   )
   (__floatsitf,   void (int, int)                                )
   (__floatunsitf, void (int, int)                                )
   (__floatditf,   void (int, int64_t)                            )
   (__floatunditf, void (int, int64_t)                            )
   (__floattidf,   double (int64_t, int64_t)                      )
   (__floatuntidf, double (int64_t, int64_t)                      )
   (__floatsidf,   double(int)                                    )
   (__extendsftf2, void(int, float)                               )
   (__extenddftf2, void(int, double)                              )
   (__fixtfti,     void(int, int64_t, int64_t)                    )
   (__fixtfdi,     int64_t(int64_t, int64_t)                      )
   (__fixtfsi,     int(int64_t, int64_t)                          )
   (__fixunstfti,  void(int, int64_t, int64_t)                    )
   (__fixunstfdi,  int64_t(int64_t, int64_t)                      )
   (__fixunstfsi,  int(int64_t, int64_t)                          )
   (__fixsfti,     void(int, float)                               )
   (__fixdfti,     void(int, double)                              )
   (__fixunssfti,  void(int, float)                               )
   (__fixunsdfti,  void(int, double)                              )
   (__trunctfdf2,  double(int64_t, int64_t)                       )
   (__trunctfsf2,  float(int64_t, int64_t)                        )
);

REGISTER_INJECTED_INTRINSICS(softfloat_api,
      (_yy_f32_add,       float(float, float)    )
      (_yy_f32_sub,       float(float, float)    )
      (_yy_f32_mul,       float(float, float)    )
      (_yy_f32_div,       float(float, float)    )
      (_yy_f32_min,       float(float, float)    )
      (_yy_f32_max,       float(float, float)    )
      (_yy_f32_copysign,  float(float, float)    )
      (_yy_f32_abs,       float(float)           )
      (_yy_f32_neg,       float(float)           )
      (_yy_f32_sqrt,      float(float)           )
      (_yy_f32_ceil,      float(float)           )
      (_yy_f32_floor,     float(float)           )
      (_yy_f32_trunc,     float(float)           )
      (_yy_f32_nearest,   float(float)           )
      (_yy_f32_eq,        int(float, float)      )
      (_yy_f32_ne,        int(float, float)      )
      (_yy_f32_lt,        int(float, float)      )
      (_yy_f32_le,        int(float, float)      )
      (_yy_f32_gt,        int(float, float)      )
      (_yy_f32_ge,        int(float, float)      )
      (_yy_f64_add,       double(double, double) )
      (_yy_f64_sub,       double(double, double) )
      (_yy_f64_mul,       double(double, double) )
      (_yy_f64_div,       double(double, double) )
      (_yy_f64_min,       double(double, double) )
      (_yy_f64_max,       double(double, double) )
      (_yy_f64_copysign,  double(double, double) )
      (_yy_f64_abs,       double(double)         )
      (_yy_f64_neg,       double(double)         )
      (_yy_f64_sqrt,      double(double)         )
      (_yy_f64_ceil,      double(double)         )
      (_yy_f64_floor,     double(double)         )
      (_yy_f64_trunc,     double(double)         )
      (_yy_f64_nearest,   double(double)         )
      (_yy_f64_eq,        int(double, double)    )
      (_yy_f64_ne,        int(double, double)    )
      (_yy_f64_lt,        int(double, double)    )
      (_yy_f64_le,        int(double, double)    )
      (_yy_f64_gt,        int(double, double)    )
      (_yy_f64_ge,        int(double, double)    )
      (_yy_f32_promote,    double(float)         )
      (_yy_f64_demote,     float(double)         )
      (_yy_f32_trunc_i32s, int(float)            )
      (_yy_f64_trunc_i32s, int(double)           )
      (_yy_f32_trunc_i32u, int(float)            )
      (_yy_f64_trunc_i32u, int(double)           )
      (_yy_f32_trunc_i64s, int64_t(float)        )
      (_yy_f64_trunc_i64s, int64_t(double)       )
      (_yy_f32_trunc_i64u, int64_t(float)        )
      (_yy_f64_trunc_i64u, int64_t(double)       )
      (_yy_i32_to_f32,     float(int32_t)        )
      (_yy_i64_to_f32,     float(int64_t)        )
      (_yy_ui32_to_f32,    float(int32_t)        )
      (_yy_ui64_to_f32,    float(int64_t)        )
      (_yy_i32_to_f64,     double(int32_t)       )
      (_yy_i64_to_f64,     double(int64_t)       )
      (_yy_ui32_to_f64,    double(int32_t)       )
      (_yy_ui64_to_f64,    double(int64_t)       )
);

std::istream& operator>>(std::istream& in, wasm_interface::vm_type& runtime) {
   std::string s;
   in >> s;
   if (s == "wavm")
      runtime = graphene::chain::wasm_interface::vm_type::wavm;
   else if (s == "binaryen")
      runtime = graphene::chain::wasm_interface::vm_type::binaryen;
   else if (s == "wabt")
      runtime = graphene::chain::wasm_interface::vm_type::wabt;
   else
      in.setstate(std::ios_base::failbit);
   return in;
}

} } /// graphene::chain
