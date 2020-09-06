#include <graphene/chain/wasm_constraints.hpp>
#include <graphene/chain/wasm_injection.hpp>
#include <graphene/chain/wasm_binary_ops.hpp>
#include <fc/exception/exception.hpp>
#include <graphene/chain/exceptions.hpp>
#include "IR/Module.h"
#include "IR/Operators.h"
#include "WASM/WASM.h"

namespace graphene { namespace chain { namespace wasm_injections {
using namespace IR;
using namespace graphene::chain::wasm_constraints;

std::map<std::vector<uint16_t>, uint32_t> injector_utils::type_slots;
std::map<std::string, uint32_t>           injector_utils::registered_injected;
std::map<uint32_t, uint32_t>              injector_utils::injected_index_mapping;
uint32_t                                  injector_utils::next_injected_index;


void noop_injection_visitor::inject( Module& m ) { /* just pass */ }
void noop_injection_visitor::initializer() { /* just pass */ }

void memories_injection_visitor::inject( Module& m ) {
}
void memories_injection_visitor::initializer() {
}

void data_segments_injection_visitor::inject( Module& m ) {
}
void data_segments_injection_visitor::initializer() {
}
void max_memory_injection_visitor::inject( Module& m ) {
   if(m.memories.defs.size() && m.memories.defs[0].type.size.max > maximum_linear_memory/wasm_page_size)
      m.memories.defs[0].type.size.max = maximum_linear_memory/wasm_page_size;
}
void max_memory_injection_visitor::initializer() {}

// float injections
const char* inject_which_op( uint16_t opcode ) {
  switch ( opcode ) {
     case wasm_ops::f32_add_code:
        return u8"_yy_f32_add";
     case wasm_ops::f32_sub_code:
        return u8"_yy_f32_sub";
     case wasm_ops::f32_mul_code:
        return u8"_yy_f32_mul";
     case wasm_ops::f32_div_code:
        return u8"_yy_f32_div";
     case wasm_ops::f32_min_code:
        return u8"_yy_f32_min";
     case wasm_ops::f32_max_code:
        return u8"_yy_f32_max";
     case wasm_ops::f32_copysign_code:
        return u8"_yy_f32_copysign";
     case wasm_ops::f32_abs_code:
        return u8"_yy_f32_abs";
     case wasm_ops::f32_neg_code:
        return u8"_yy_f32_neg";
     case wasm_ops::f32_sqrt_code:
        return u8"_yy_f32_sqrt";
     case wasm_ops::f32_ceil_code:
        return u8"_yy_f32_ceil";
     case wasm_ops::f32_floor_code:
        return u8"_yy_f32_floor";
     case wasm_ops::f32_trunc_code:
        return u8"_yy_f32_trunc";
     case wasm_ops::f32_nearest_code:
        return u8"_yy_f32_nearest";
     case wasm_ops::f32_eq_code:
        return u8"_yy_f32_eq";
     case wasm_ops::f32_ne_code:
        return u8"_yy_f32_ne";
     case wasm_ops::f32_lt_code:
        return u8"_yy_f32_lt";
     case wasm_ops::f32_le_code:
        return u8"_yy_f32_le";
     case wasm_ops::f32_gt_code:
        return u8"_yy_f32_gt";
     case wasm_ops::f32_ge_code:
        return u8"_yy_f32_ge";
     case wasm_ops::f64_add_code:
        return u8"_yy_f64_add";
     case wasm_ops::f64_sub_code:
        return u8"_yy_f64_sub";
     case wasm_ops::f64_mul_code:
        return u8"_yy_f64_mul";
     case wasm_ops::f64_div_code:
        return u8"_yy_f64_div";
     case wasm_ops::f64_min_code:
        return u8"_yy_f64_min";
     case wasm_ops::f64_max_code:
        return u8"_yy_f64_max";
     case wasm_ops::f64_copysign_code:
        return u8"_yy_f64_copysign";
     case wasm_ops::f64_abs_code:
        return u8"_yy_f64_abs";
     case wasm_ops::f64_neg_code:
        return u8"_yy_f64_neg";
     case wasm_ops::f64_sqrt_code:
        return u8"_yy_f64_sqrt";
     case wasm_ops::f64_ceil_code:
        return u8"_yy_f64_ceil";
     case wasm_ops::f64_floor_code:
        return u8"_yy_f64_floor";
     case wasm_ops::f64_trunc_code:
        return u8"_yy_f64_trunc";
     case wasm_ops::f64_nearest_code:
        return u8"_yy_f64_nearest";
     case wasm_ops::f64_eq_code:
        return u8"_yy_f64_eq";
     case wasm_ops::f64_ne_code:
        return u8"_yy_f64_ne";
     case wasm_ops::f64_lt_code:
        return u8"_yy_f64_lt";
     case wasm_ops::f64_le_code:
        return u8"_yy_f64_le";
     case wasm_ops::f64_gt_code:
        return u8"_yy_f64_gt";
     case wasm_ops::f64_ge_code:
        return u8"_yy_f64_ge";
     case wasm_ops::f64_promote_f32_code:
        return u8"_yy_f32_promote";
     case wasm_ops::f32_demote_f64_code:
        return u8"_yy_f64_demote";
     case wasm_ops::i32_trunc_u_f32_code:
        return u8"_yy_f32_trunc_i32u";
     case wasm_ops::i32_trunc_s_f32_code:
        return u8"_yy_f32_trunc_i32s";
     case wasm_ops::i32_trunc_u_f64_code:
        return u8"_yy_f64_trunc_i32u";
     case wasm_ops::i32_trunc_s_f64_code:
        return u8"_yy_f64_trunc_i32s";
     case wasm_ops::i64_trunc_u_f32_code:
        return u8"_yy_f32_trunc_i64u";
     case wasm_ops::i64_trunc_s_f32_code:
        return u8"_yy_f32_trunc_i64s";
     case wasm_ops::i64_trunc_u_f64_code:
        return u8"_yy_f64_trunc_i64u";
     case wasm_ops::i64_trunc_s_f64_code:
        return u8"_yy_f64_trunc_i64s";
     case wasm_ops::f32_convert_s_i32_code:
        return u8"_yy_i32_to_f32";
     case wasm_ops::f32_convert_u_i32_code:
        return u8"_yy_ui32_to_f32";
     case wasm_ops::f32_convert_s_i64_code:
        return u8"_yy_i64_f32";
     case wasm_ops::f32_convert_u_i64_code:
        return u8"_yy_ui64_to_f32";
     case wasm_ops::f64_convert_s_i32_code:
        return u8"_yy_i32_to_f64";
     case wasm_ops::f64_convert_u_i32_code:
        return u8"_yy_ui32_to_f64";
     case wasm_ops::f64_convert_s_i64_code:
        return u8"_yy_i64_to_f64";
     case wasm_ops::f64_convert_u_i64_code:
        return u8"_yy_ui64_to_f64";

     default:
        FC_THROW_EXCEPTION( graphene::chain::wasm_execution_error, "Error, unknown opcode in injection ${op}", ("op", opcode));
  }
}


int32_t  call_depth_check::global_idx = -1;
uint32_t instruction_counter::icnt = 0;
uint32_t instruction_counter::tcnt = 0;
uint32_t instruction_counter::bcnt = 0;
std::queue<uint32_t> instruction_counter::fcnts;

int32_t  checktime_injection::idx = 0;
int32_t  checktime_injection::chktm_idx = 0;
std::stack<size_t>                   checktime_block_type::block_stack;
std::stack<size_t>                   checktime_block_type::type_stack;
std::queue<std::vector<size_t>>      checktime_block_type::orderings;
std::queue<std::map<size_t, size_t>> checktime_block_type::bcnt_tables;
size_t  checktime_function_end::fcnt = 0;

}}} // namespace graphene, chain, injectors
