/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <fc/exception/exception.hpp>
#include <graphene/chain/protocol/types.hpp>
//#include <graphene/chain/core_symbol.hpp>
#include <string>
#include <functional>

namespace graphene {
   namespace chain {
#define CORE_SYMBOL SY(4,SYS)
#define CORE_SYMBOL_NAME "SYS"

      static constexpr uint64_t string_to_symbol_c(uint8_t precision, const char* str) {
         uint32_t len = 0;
         while (str[len]) ++len;

         uint64_t result = 0;
         // No validation is done at compile time
         for (uint32_t i = 0; i < len; ++i) {
            result |= (uint64_t(str[i]) << (8*(1+i)));
         }

         result |= uint64_t(precision);
         return result;
      }

#define SY(P,X) ::graphene::chain::string_to_symbol_c(P,#X)

      static uint64_t string_to_symbol(uint8_t precision, const char* str) {
         try {
            uint32_t len = 0;
            while(str[len]) ++len;
            uint64_t result = 0;
            for (uint32_t i = 0; i < len; ++i) {
               // All characters must be upper case alphabets
               FC_ASSERT (str[i] >= 'A' && str[i] <= 'Z', "invalid character in symbol name");
               result |= (uint64_t(str[i]) << (8*(i+1)));
            }
            result |= uint64_t(precision);
            return result;
         } FC_CAPTURE_LOG_AND_RETHROW((str))
      }

      struct symbol_code {
         uint64_t value;

         operator uint64_t()const { return value; }
      };

      class symbol {
         public:

            static constexpr uint8_t max_precision = 18;

            explicit symbol(uint8_t p, const char* s): m_value(string_to_symbol(p, s)) {
               FC_ASSERT(valid(), "invalid symbol: ${s}", ("s",s));
            }
            explicit symbol(uint64_t v = CORE_SYMBOL): m_value(v) {
               FC_ASSERT(valid(), "invalid symbol: ${name}", ("name",name()));
            }
            static symbol from_string(const string& from)
            {
               try {
                  string s = fc::trim(from);
                  FC_ASSERT(!s.empty(), "creating symbol from empty string");
                  auto comma_pos = s.find(',');
                  FC_ASSERT(comma_pos != string::npos, "missing comma in symbol");
                  auto prec_part = s.substr(0, comma_pos);
                  uint8_t p = fc::to_int64(prec_part);
                  string name_part = s.substr(comma_pos + 1);
                  FC_ASSERT( p <= max_precision, "precision ${p} should be <= 18", ("p", p));
                  return symbol(string_to_symbol(p, name_part.c_str()));
               } FC_CAPTURE_LOG_AND_RETHROW((from))
            }
            uint64_t value() const { return m_value; }
            bool valid() const
            {
               const auto& s = name();
               return decimals() <= max_precision && valid_name(s);
            }
            static bool valid_name(const string& name)
            {
               return all_of(name.begin(), name.end(), [](char c)->bool { return (c >= 'A' && c <= 'Z'); });
            }

            uint8_t decimals() const { return m_value & 0xFF; }
            uint64_t precision() const
            {
               FC_ASSERT( decimals() <= max_precision, "precision ${p} should be <= 18", ("p", decimals()) );
               uint64_t p10 = 1;
               uint64_t p = decimals();
               while( p > 0  ) {
                  p10 *= 10; --p;
               }
               return p10;
            }
            string name() const
            {
               uint64_t v = m_value;
               v >>= 8;
               string result;
               while (v > 0) {
                  char c = v & 0xFF;
                  result += c;
                  v >>= 8;
               }
               return result;
            }

            symbol_code to_symbol_code()const { return {m_value >> 8}; }

            explicit operator string() const
            {
               uint64_t v = m_value;
               uint8_t p = v & 0xFF;
               string ret = fc::to_string(p);
               ret += ',';
               ret += name();
               return ret;
            }

            string to_string() const { return string(*this); }
            template <typename DataStream>
            friend DataStream& operator<< (DataStream& ds, const symbol& s)
            {
               return ds << s.to_string();
            }

            void reflector_verify()const {
               FC_ASSERT( decimals() <= max_precision, "precision ${p} should be <= 18", ("p", decimals()) );
               FC_ASSERT( valid_name(name()), "invalid symbol: ${name}", ("name",name()));
            }

         private:
            uint64_t m_value;
            friend struct fc::reflector<symbol>;
      }; // class symbol

      struct extended_symbol {
         symbol       sym;
         account_name contract;
      };

      inline bool operator== (const symbol& lhs, const symbol& rhs)
      {
         return lhs.value() == rhs.value();
      }
      inline bool operator!= (const symbol& lhs, const symbol& rhs)
      {
         return lhs.value() != rhs.value();
      }
      inline bool operator< (const symbol& lhs, const symbol& rhs)
      {
         return lhs.value() < rhs.value();
      }
      inline bool operator> (const symbol& lhs, const symbol& rhs)
      {
         return lhs.value() > rhs.value();
      }

   } // namespace chain
} // namespace graphene

namespace fc {
   inline void to_variant(const graphene::chain::symbol& var, fc::variant& vo) { vo = var.to_string(); }
   inline void from_variant(const fc::variant& var, graphene::chain::symbol& vo) {
      vo = graphene::chain::symbol::from_string(var.get_string());
   }
}

namespace fc {
   inline void to_variant(const graphene::chain::symbol_code& var, fc::variant& vo) {
      vo = graphene::chain::symbol(var.value << 8).name();
   }
   inline void from_variant(const fc::variant& var, graphene::chain::symbol_code& vo) {
      vo = graphene::chain::symbol(0, var.get_string().c_str()).to_symbol_code();
   }
}

FC_REFLECT(graphene::chain::symbol_code, (value))
FC_REFLECT(graphene::chain::symbol, (m_value))
FC_REFLECT(graphene::chain::extended_symbol, (sym)(contract))
