/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <algorithm>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <fc/io/raw.hpp>


#define MAX_FEE_STABILIZATION_ITERATION 4

namespace graphene { namespace chain {
   fee_schedule::fee_schedule()
   {
   }

   fee_schedule fee_schedule::get_default()
   {
      fee_schedule result;
      for( int i = 0; i < fee_parameters().count(); ++i )
      {
         fee_parameters x; x.set_which(i);
         result.parameters.insert(x);
      }
      return result;
   }

   struct fee_schedule_validate_visitor
   {
      typedef void result_type;

      template<typename T>
      void operator()( const T& p )const
      {
         //p.validate();
      }
   };

   void fee_schedule::validate()const
   {
      for( const auto& f : parameters )
         f.visit( fee_schedule_validate_visitor() );
   }

   struct calc_fee_visitor
   {
      typedef uint64_t result_type;

      const fee_parameters& param;
      calc_fee_visitor( const fee_parameters& p ):param(p){}

      template<typename OpType>
      result_type operator()(  const OpType& op )const
      {
         return op.calculate_fee( param.get<typename OpType::fee_parameters_type>() ).value;
      }
   };

   struct calc_fee_pair_visitor
   {
      typedef std::pair<share_type,share_type> result_type;

      const fee_parameters& param;
      calc_fee_pair_visitor( const fee_parameters& p ):param(p){}

      template<typename OpType>
      result_type operator()( const OpType& op )const
      {
         const auto& fee_param = param.get<typename OpType::fee_parameters_type>();
         share_type fee = op.calculate_fee( fee_param );
         return op.calculate_fee_pair( fee, fee_param );
      }
   };

   BOOST_TTI_HAS_MEMBER_DATA(fee)

   struct set_fee_visitor
   {
      typedef void result_type;
      asset _fee;

      set_fee_visitor( asset f ):_fee(f){}

      template<typename OpType>
      void set_fee( OpType& op, std::true_type )const
      {
         op.fee = _fee;
      }

      template<typename OpType>
      void set_fee( OpType& op, std::false_type )const
      {
         op.fee = fee_type(_fee);
      }

      template<typename OpType>
      void set_fee( OpType& op )const
      {
         const bool b = has_member_data_fee<OpType,asset>::value;
         set_fee( op, std::integral_constant<bool, b>() );
      }

      template<typename OpType>
      void operator()( OpType& op )const
      {
         set_fee( op );
      }
   };

   struct set_fee_with_csaf_visitor
   {
      typedef void result_type;
      std::pair<share_type,share_type> _fee_pair;

      set_fee_with_csaf_visitor( const std::pair<share_type,share_type>& fp ):_fee_pair(fp){}

      template<typename OpType>
      void set_fee_with_csaf( OpType& op, std::true_type )const
      {
         op.fee = asset( _fee_pair.first, GRAPHENE_CORE_ASSET_AID );
      }

      template<typename OpType>
      void set_fee_with_csaf( OpType& op, std::false_type )const
      {
         fee_type f( asset( _fee_pair.first, GRAPHENE_CORE_ASSET_AID ) );

         share_type max_csaf = _fee_pair.first - _fee_pair.second;
         if( max_csaf > 0 )
         {
            extension<fee_extension_type> fex;
            fex.value.from_csaf = asset( max_csaf, GRAPHENE_CORE_ASSET_AID );
            if( max_csaf < _fee_pair.first )
               fex.value.from_balance = asset( _fee_pair.first - max_csaf.value, GRAPHENE_CORE_ASSET_AID );
            f.options = fex;
         }

         op.fee = f;
      }

      template<typename OpType>
      void set_fee_with_csaf( OpType& op )const
      {
         const bool b = has_member_data_fee<OpType,asset>::value;
         set_fee_with_csaf( op, std::integral_constant<bool, b>() );
      }

      template<typename OpType>
      void operator()( OpType& op )const
      {
         set_fee_with_csaf( op );
      }
   };

   struct zero_fee_visitor
   {
      typedef void result_type;

      template<typename ParamType>
      result_type operator()(  ParamType& op )const
      {
         memset( (char*)&op, 0, sizeof(op) );
      }
   };

   void fee_schedule::zero_all_fees()
   {
      *this = get_default();
      for( fee_parameters& i : parameters )
         i.visit( zero_fee_visitor() );
      this->scale = 0;
   }

   asset fee_schedule::calculate_fee( const operation& op, const price& core_exchange_rate )const
   {
      //idump( (op)(core_exchange_rate) );
      fee_parameters params; params.set_which(op.which());
      auto itr = parameters.find(params);
      if( itr != parameters.end() ) params = *itr;
      auto base_value = op.visit( calc_fee_visitor( params ) );
      auto scaled = fc::uint128_t(base_value) * scale;
      scaled /= GRAPHENE_100_PERCENT;
      FC_ASSERT( scaled <= GRAPHENE_MAX_SHARE_SUPPLY );
      //idump( (base_value)(scaled)(core_exchange_rate) );
      auto result = asset(static_cast<int64_t>(scaled), GRAPHENE_CORE_ASSET_AID ) * core_exchange_rate;
      //FC_ASSERT( result * core_exchange_rate >= asset( scaled.to_uint64()) );

      while( result * core_exchange_rate < asset(static_cast<int64_t>(scaled)) )
        result.amount++;

      FC_ASSERT( result.amount <= GRAPHENE_MAX_SHARE_SUPPLY );
      return result;
   }

   std::pair<share_type,share_type> fee_schedule::calculate_fee_pair( const operation& op )const
   {
      fee_parameters params; params.set_which(op.which());
      auto itr = parameters.find(params);
      if( itr != parameters.end() ) params = *itr;
      return op.visit( calc_fee_pair_visitor( params ) );
   }

   void fee_schedule::set_fee_with_csaf( operation& op )const
   {
      auto fp = calculate_fee_pair( op );
      op.visit( set_fee_with_csaf_visitor( fp ) );
   }

   asset fee_schedule::set_fee( operation& op, const price& core_exchange_rate )const
   {
      auto f = calculate_fee( op, core_exchange_rate );
      auto f_max = f;
      for( int i=0; i<MAX_FEE_STABILIZATION_ITERATION; i++ )
      {
         op.visit( set_fee_visitor( f_max ) );
         auto f2 = calculate_fee( op, core_exchange_rate );
         if( f == f2 )
            break;
         f_max = std::max( f_max, f2 );
         f = f2;
         if( i == 0 )
         {
            // no need for warnings on later iterations
            wlog( "set_fee requires multiple iterations to stabilize with core_exchange_rate ${p} on operation ${op}",
               ("p", core_exchange_rate) ("op", op) );
         }
      }
      return f_max;
   }

} } // graphene::chain
