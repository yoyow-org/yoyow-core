#pragma once
#include <boost/endian/buffers.hpp>

#include <fc/io/raw_variant.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/io/datastream.hpp>
#include <fc/io/varint.hpp>
#include <fc/optional.hpp>
#include <fc/fwd.hpp>
#include <fc/time.hpp>
#include <fc/filesystem.hpp>
#include <fc/exception/exception.hpp>
#include <fc/io/raw_fwd.hpp>
#include <algorithm>
#include <array>
#include <map>
#include <deque>

namespace fc {
    namespace raw {

    template<typename Stream, typename Arg0, typename... Args>
    inline void pack( Stream& s, const Arg0& a0, Args... args, uint32_t _max_depth ) {
       FC_ASSERT( _max_depth > 0 );
       --_max_depth;
       pack( s, a0, _max_depth );
       pack( s, args..., _max_depth );
    }

    template<typename Stream>
    inline void pack( Stream& s, const uint128_t& v, uint32_t _max_depth )
    {
        boost::endian::little_uint64_buf_at hilo[2];
        hilo[0] = uint128_hi64( v );
        hilo[1] = uint128_lo64( v );
        s.write( hilo[0].data(), sizeof(hilo) );
    }
    template<typename Stream>
    inline void unpack( Stream& s, uint128_t& v, uint32_t _max_depth )
    {
        boost::endian::little_uint64_buf_at hilo[2];
        s.read( (char*) hilo, sizeof(hilo) );
        v = uint128( hilo[0].value(), hilo[1].value() );
    }

    template<typename Stream>
    inline void pack( Stream& s, const fc::exception& e, uint32_t _max_depth )
    {
       FC_ASSERT( _max_depth > 0 );
       --_max_depth;
       fc::raw::pack( s, e.code(), _max_depth );
       fc::raw::pack( s, std::string(e.name()), _max_depth );
       fc::raw::pack( s, std::string(e.what()), _max_depth );
       fc::raw::pack( s, e.get_log(), _max_depth );
    }
    template<typename Stream>
    inline void unpack( Stream& s, fc::exception& e, uint32_t _max_depth )
    {
       FC_ASSERT( _max_depth > 0 );
       --_max_depth;
       int64_t code;
       std::string name, what;
       log_messages msgs;

       fc::raw::unpack( s, code, _max_depth );
       fc::raw::unpack( s, name, _max_depth );
       fc::raw::unpack( s, what, _max_depth );
       fc::raw::unpack( s, msgs, _max_depth );

       e = fc::exception( std::move(msgs), code, name, what );
    }

    template<typename Stream>
    inline void pack( Stream& s, const fc::log_message& msg, uint32_t _max_depth )
    {
       FC_ASSERT( _max_depth > 0 );
       --_max_depth;
       fc::raw::pack( s, variant( msg, std::min( _max_depth, uint32_t(FC_MAX_LOG_OBJECT_DEPTH) ) ), _max_depth );
    }
    template<typename Stream>
    inline void unpack( Stream& s, fc::log_message& msg, uint32_t _max_depth )
    {
       FC_ASSERT( _max_depth > 0 );
       fc::variant vmsg;
       --_max_depth;
       fc::raw::unpack( s, vmsg, _max_depth );
       msg = vmsg.as<log_message>( std::min( _max_depth, uint32_t(FC_MAX_LOG_OBJECT_DEPTH) ) );
    }

    template<typename Stream>
    inline void pack( Stream& s, const fc::path& tp, uint32_t _max_depth )
    {
       FC_ASSERT( _max_depth > 0 );
       fc::raw::pack( s, tp.generic_string(), _max_depth - 1 );
    }

    template<typename Stream>
    inline void unpack( Stream& s, fc::path& tp, uint32_t _max_depth )
    {
       FC_ASSERT( _max_depth > 0 );
       std::string p;
       fc::raw::unpack( s, p, _max_depth - 1 );
       tp = p;
    }

    template<typename Stream>
    inline void pack( Stream& s, const fc::time_point_sec& tp, uint32_t _max_depth )
    {
       pack( s, tp.sec_since_epoch(), _max_depth );
    }

    template<typename Stream>
    inline void unpack( Stream& s, fc::time_point_sec& tp, uint32_t _max_depth )
    { try {
       uint32_t sec;
       unpack( s, sec, _max_depth );
       tp = fc::time_point() + fc::seconds(sec);
    } FC_RETHROW_EXCEPTIONS( warn, "" ) }

    template<typename Stream>
    inline void pack( Stream& s, const fc::time_point& tp, uint32_t _max_depth )
    {
       pack( s, tp.time_since_epoch().count(), _max_depth );
    }

    template<typename Stream>
    inline void unpack( Stream& s, fc::time_point& tp, uint32_t _max_depth )
    { try {
       uint64_t usec;
       unpack( s, usec, _max_depth );
       tp = fc::time_point() + fc::microseconds(usec);
    } FC_RETHROW_EXCEPTIONS( warn, "" ) }

    template<typename Stream>
    inline void pack( Stream& s, const fc::microseconds& usec, uint32_t _max_depth )
    {
       pack( s, usec.count(), _max_depth );
    }

    template<typename Stream>
    inline void unpack( Stream& s, fc::microseconds& usec, uint32_t _max_depth )
    { try {
       uint64_t usec_as_int64;
       unpack( s, usec_as_int64, _max_depth );
       usec = fc::microseconds(usec_as_int64);
    } FC_RETHROW_EXCEPTIONS( warn, "" ) }

    template<typename Stream, size_t N>
    inline void pack( Stream& s, const std::array<char,N>& v, uint32_t _max_depth ) {
       s.write( v.data(), N );
    }
    template<typename Stream, size_t N>
    inline void pack( Stream& s, const std::array<unsigned char,N>& v, uint32_t _max_depth ) {
       s.write( (char*)v.data(), N );
    }

    template<typename Stream, size_t N>
    inline void unpack( Stream& s, std::array<char,N>& v, uint32_t _max_depth ) { try {
       s.read( v.data(), N );
    } FC_RETHROW_EXCEPTIONS( warn, "std::array<char,${length}>", ("length",N) ) }
    template<typename Stream, size_t N>
    inline void unpack( Stream& s, std::array<unsigned char,N>& v, uint32_t _max_depth ) { try {
       s.read( (char*)v.data(), N );
    } FC_RETHROW_EXCEPTIONS( warn, "std::array<unsigned char,${length}>", ("length",N) ) }

    template<typename Stream, typename T>
    inline void pack( Stream& s, const std::shared_ptr<T>& v, uint32_t _max_depth )
    {
       FC_ASSERT( _max_depth > 0 );
       fc::raw::pack( s, *v, _max_depth - 1 );
    }

    template<typename Stream, typename T>
    inline void unpack( Stream& s, std::shared_ptr<T>& v, uint32_t _max_depth )
    { try {
       FC_ASSERT( _max_depth > 0 );
       v = std::make_shared<T>();
       fc::raw::unpack( s, *v, _max_depth - 1 );
    } FC_RETHROW_EXCEPTIONS( warn, "std::shared_ptr<${type}>", ("type",fc::get_typename<T>::name()) ) }

    template<typename Stream, typename T>
    inline void pack( Stream& s, const std::shared_ptr<const T>& v, uint32_t _max_depth )
    {
       FC_ASSERT( _max_depth > 0 );
       fc::raw::pack( s, *v, _max_depth - 1 );
    }

    template<typename Stream, typename T>
    inline void unpack( Stream& s, std::shared_ptr<const T>& v, uint32_t _max_depth )
    { try {
       FC_ASSERT( _max_depth > 0 );
       T tmp;
       fc::raw::unpack( s, tmp, _max_depth - 1 );
       v = std::make_shared<const T>(std::move(tmp));
    } FC_RETHROW_EXCEPTIONS( warn, "std::shared_ptr<const T>", ("type",fc::get_typename<T>::name()) ) }

    template<typename Stream> inline void pack( Stream& s, const unsigned_int& v, uint32_t _max_depth ) {
      uint64_t val = v.value;
      do {
        uint8_t b = uint8_t(val) & 0x7f;
        val >>= 7;
        b |= ((val > 0) << 7);
        s.write((char*)&b,1);//.put(b);
      }while( val );
    }

    template<typename Stream> inline void unpack( Stream& s, unsigned_int& vi, uint32_t _max_depth ) {
      uint64_t v = 0; char b = 0; uint8_t by = 0;
      do {
          s.get(b);
          if( by >= 64 || (by == 63 && uint8_t(b) > 1) )
             FC_THROW_EXCEPTION( overflow_exception, "Invalid packed unsigned_int!" );
          v |= uint64_t(uint8_t(b) & 0x7f) << by;
          by += 7;
      } while( uint8_t(b) & 0x80 );
      vi.value = static_cast<uint64_t>(v);
    }

    template<typename Stream, typename T> inline void unpack( Stream& s, const T& vi, uint32_t _max_depth )
    {
       FC_ASSERT( _max_depth > 0 );
       T tmp;
       fc::raw::unpack( s, tmp, _max_depth - 1 );
       FC_ASSERT( vi == tmp );
    }

    template<typename Stream> inline void pack( Stream& s, const char* v, uint32_t _max_depth )
    {
       FC_ASSERT( _max_depth > 0 );
       fc::raw::pack( s, std::string(v), _max_depth - 1 );
    }

    template<typename Stream, typename T>
    void pack( Stream& s, const safe<T>& v, uint32_t _max_depth )
    {
       FC_ASSERT( _max_depth > 0 );
       fc::raw::pack( s, v.value, _max_depth - 1 );
    }

    template<typename Stream, typename T>
    void unpack( Stream& s, fc::safe<T>& v, uint32_t _max_depth )
    {
       FC_ASSERT( _max_depth > 0 );
       fc::raw::unpack( s, v.value, _max_depth - 1 );
    }

    template<typename Stream, typename T, unsigned int S, typename Align>
    void pack( Stream& s, const fc::fwd<T,S,Align>& v, uint32_t _max_depth ) {
       FC_ASSERT( _max_depth > 0 );
       fc::raw::pack( *v, _max_depth - 1 ); // TODO not sure about this
    }

    template<typename Stream, typename T, unsigned int S, typename Align>
    void unpack( Stream& s, fc::fwd<T,S,Align>& v, uint32_t _max_depth ) {
       FC_ASSERT( _max_depth > 0 );
       fc::raw::unpack( *v, _max_depth - 1 ); // TODO not sure about this
    }

    // optional
    template<typename Stream, typename T>
    void pack( Stream& s, const fc::optional<T>& v, uint32_t _max_depth ) {
       FC_ASSERT( _max_depth > 0 );
       --_max_depth;
       fc::raw::pack( s, bool(!!v), _max_depth );
       if( !!v ) fc::raw::pack( s, *v, _max_depth );
    }

    template<typename Stream, typename T>
    void unpack( Stream& s, fc::optional<T>& v, uint32_t _max_depth )
    { try {
       FC_ASSERT( _max_depth > 0 );
       --_max_depth;
       bool b; fc::raw::unpack( s, b, _max_depth );
       if( b ) { v = T(); fc::raw::unpack( s, *v, _max_depth ); }
    } FC_RETHROW_EXCEPTIONS( warn, "optional<${type}>", ("type",fc::get_typename<T>::name() ) ) }

    // std::vector<char>
    template<typename Stream> inline void pack( Stream& s, const std::vector<char>& value, uint32_t _max_depth ) {
       FC_ASSERT( _max_depth > 0 );
       fc::raw::pack( s, unsigned_int(value.size()), _max_depth - 1 );
       if( value.size() )
          s.write( &value.front(), value.size() );
    }
    template<typename Stream> inline void unpack( Stream& s, std::vector<char>& value, uint32_t _max_depth ) {
       FC_ASSERT( _max_depth > 0 );
       unsigned_int size; fc::raw::unpack( s, size, _max_depth - 1 );
       FC_ASSERT( size.value < MAX_ARRAY_ALLOC_SIZE );
       value.resize(size.value);
       if( value.size() )
          s.read( value.data(), value.size() );
    }

    // fc::string
    template<typename Stream> inline void pack( Stream& s, const std::string& v, uint32_t _max_depth )  {
       FC_ASSERT( _max_depth > 0 );
       fc::raw::pack( s, unsigned_int(v.size()), _max_depth - 1 );
       if( v.size() ) s.write( v.c_str(), v.size() );
    }

    template<typename Stream> inline void unpack( Stream& s, std::string& v, uint32_t _max_depth )  {
       FC_ASSERT( _max_depth > 0 );
       std::vector<char> tmp; fc::raw::unpack( s, tmp, _max_depth - 1 );
       if( tmp.size() )
          v = std::string( tmp.data(), tmp.data()+tmp.size() );
       else v = std::string();
    }

    // bool
    template<typename Stream> inline void pack( Stream& s, const bool& v, uint32_t _max_depth )
    {
       FC_ASSERT( _max_depth > 0 );
       fc::raw::pack( s, v ? uint8_t(1) : uint8_t(0), _max_depth - 1 );
    }
    template<typename Stream> inline void unpack( Stream& s, bool& v, uint32_t _max_depth )
    {
       FC_ASSERT( _max_depth > 0 );
       uint8_t b;
       fc::raw::unpack( s, b, _max_depth - 1 );
       FC_ASSERT( (b & ~1) == 0 );
       v=(b!=0);
    }

    namespace detail {

      template<typename Stream, typename Class>
      struct pack_object_visitor {
        pack_object_visitor( const Class& _c, Stream& _s, uint32_t _max_depth )
        :c(_c),s(_s),max_depth(_max_depth - 1)
        {
           FC_ASSERT( _max_depth > 0 );
        }

        template<typename T, typename C, T(C::*p)>
        void operator()( const char* name )const {
          fc::raw::pack( s, c.*p, max_depth );
        }
        private:
          const Class&   c;
          Stream&        s;
          const uint32_t max_depth;
      };

      template<typename Stream, typename Class>
      struct unpack_object_visitor {
        unpack_object_visitor( Class& _c, Stream& _s, uint32_t _max_depth ) : c(_c),s(_s),max_depth(_max_depth - 1)
        {
           FC_ASSERT( _max_depth > 0 );
        }

        template<typename T, typename C, T(C::*p)>
        inline void operator()( const char* name )const
        { try {
           fc::raw::unpack( s, c.*p, max_depth );
        } FC_RETHROW_EXCEPTIONS( warn, "Error unpacking field ${field}", ("field",name) ) }
        private:
          Class&  c;
          Stream& s;
          const uint32_t max_depth;
      };

      // Default pack/unpack functions for classes are removed due to recursion issue.
      // Classes should implement pack/unpack functions explicitly.
      template<typename T, typename Dummy = void>
      struct if_class;

      template<typename T>
      struct if_class<T, std::enable_if_t<!std::is_class<T>::value>> {
        template<typename Stream>
        static inline void pack( Stream& s, const T v, uint32_t _max_depth )
        {
			s.write( (char*)&v, sizeof(v) );
		};
        template<typename Stream>
        static inline void unpack( Stream& s, T& v, uint32_t _max_depth )
        {
          s.read( (char*)&v, sizeof(v) );
        }
      };

      template<>
      struct if_class<int64_t, void> {
        template<typename Stream>
        static inline void pack( Stream& s, const int64_t v, uint32_t _max_depth ) {
           boost::endian::little_int64_buf_t tmp;
           tmp = v;
           s.write( (char*)&tmp, sizeof(tmp) );
        }
        template<typename Stream>
        static inline void unpack( Stream& s, int64_t& v, uint32_t _max_depth ) {
           boost::endian::little_int64_buf_t tmp;
           s.read( (char*)&tmp, sizeof(tmp) );
           v = tmp.value();
        }
      };

      template<>
      struct if_class<uint64_t, void> {
        template<typename Stream>
        static inline void pack( Stream& s, const uint64_t v, uint32_t _max_depth ) {
           boost::endian::little_uint64_buf_t tmp;
           tmp = v;
           s.write( (char*)&tmp, sizeof(tmp) );
        }
        template<typename Stream>
        static inline void unpack( Stream& s, uint64_t& v, uint32_t _max_depth ) {
           boost::endian::little_uint64_buf_t tmp;
           s.read( (char*)&tmp, sizeof(tmp) );
           v = tmp.value();
        }
      };

      template<>
      struct if_class<int32_t, void> {
        template<typename Stream>
        static inline void pack( Stream& s, const int32_t v, uint32_t _max_depth ) {
           boost::endian::little_int32_buf_t tmp;
           tmp = v;
           s.write( (char*)&tmp, sizeof(tmp) );
        }
        template<typename Stream>
        static inline void unpack( Stream& s, int32_t& v, uint32_t _max_depth ) {
           boost::endian::little_int32_buf_t tmp;
           s.read( (char*)&tmp, sizeof(tmp) );
           v = tmp.value();
        }
      };

      template<>
      struct if_class<uint32_t, void> {
        template<typename Stream>
        static inline void pack( Stream& s, const uint32_t v, uint32_t _max_depth ) {
           boost::endian::little_uint32_buf_t tmp;
           tmp = v;
           s.write( (char*)&tmp, sizeof(tmp) );
        }
        template<typename Stream>
        static inline void unpack( Stream& s, uint32_t& v, uint32_t _max_depth ) {
           boost::endian::little_uint32_buf_t tmp;
           s.read( (char*)&tmp, sizeof(tmp) );
           v = tmp.value();
        }
      };

      template<>
      struct if_class<int16_t, void> {
        template<typename Stream>
        static inline void pack( Stream& s, const int16_t v, uint32_t _max_depth ) {
           boost::endian::little_int16_buf_t tmp;
           tmp = v;
           s.write( (char*)&tmp, sizeof(tmp) );
        }
        template<typename Stream>
        static inline void unpack( Stream& s, int16_t& v, uint32_t _max_depth ) {
           boost::endian::little_int16_buf_t tmp;
           s.read( (char*)&tmp, sizeof(tmp) );
           v = tmp.value();
        }
      };

      template<>
      struct if_class<uint16_t, void> {
        template<typename Stream>
        static inline void pack( Stream& s, const uint16_t v, uint32_t _max_depth ) {
           boost::endian::little_uint16_buf_t tmp;
           tmp = v;
           s.write( (char*)&tmp, sizeof(tmp) );
        }
        template<typename Stream>
        static inline void unpack( Stream& s, uint16_t& v, uint32_t _max_depth ) {
           boost::endian::little_uint16_buf_t tmp;
           s.read( (char*)&tmp, sizeof(tmp) );
           v = tmp.value();
        }
      };

      template<>
      struct if_class<int8_t, void> {
        template<typename Stream>
        static inline void pack( Stream& s, const int8_t v, uint32_t _max_depth ) {
           s.write( (char*)&v, 1 );
        }
        template<typename Stream>
        static inline void unpack( Stream& s, int8_t& v, uint32_t _max_depth ) {
           s.read( (char*)&v, 1 );
        }
      };

      template<>
      struct if_class<uint8_t, void> {
        template<typename Stream>
        static inline void pack( Stream& s, const uint8_t v, uint32_t _max_depth ) {
           s.write( (char*)&v, 1 );
        }
        template<typename Stream>
        static inline void unpack( Stream& s, uint8_t& v, uint32_t _max_depth ) {
           s.read( (char*)&v, 1 );
        }
      };

      template<typename T, typename Dummy=void>
      struct if_enum;
      template<typename T>
      struct if_enum<T, std::enable_if_t<!std::is_enum<T>::value>> {
        template<typename Stream>
        static inline void pack( Stream& s, const T& v, uint32_t _max_depth ) {
          FC_ASSERT( _max_depth > 0 );
          fc::reflector<T>::visit( pack_object_visitor<Stream,T>( v, s, _max_depth - 1 ) );
        }
        template<typename Stream>
        static inline void unpack( Stream& s, T& v, uint32_t _max_depth ) {
          FC_ASSERT( _max_depth > 0 );
          fc::reflector<T>::visit( unpack_object_visitor<Stream,T>( v, s, _max_depth - 1 ) );
        }
      };
      template<typename T>
      struct if_enum<T, std::enable_if_t<std::is_enum<T>::value>> {
        template<typename Stream>
        static inline void pack( Stream& s, const T& v, uint32_t _max_depth ) {
          FC_ASSERT( _max_depth > 0 );
          fc::raw::pack( s, (int64_t)v, _max_depth - 1 );
        }
        template<typename Stream>
        static inline void unpack( Stream& s, T& v, uint32_t _max_depth ) {
          FC_ASSERT( _max_depth > 0 );
          int64_t temp;
          fc::raw::unpack( s, temp, _max_depth - 1 );
          v = (T)temp;
        }
      };

      template<typename IsReflected=std::false_type>
      struct if_reflected {
        template<typename Stream, typename T>
        static inline void pack( Stream& s, const T& v, uint32_t _max_depth ) {
          FC_ASSERT( _max_depth > 0 );
          if_class<T>::pack( s, v, _max_depth - 1 );
        }
        template<typename Stream, typename T>
        static inline void unpack( Stream& s, T& v, uint32_t _max_depth ) {
          FC_ASSERT( _max_depth > 0 );
          if_class<T>::unpack( s, v, _max_depth - 1 );
        }
      };
      template<>
      struct if_reflected<std::true_type> {
        template<typename Stream, typename T>
        static inline void pack( Stream& s, const T& v, uint32_t _max_depth ) {
          FC_ASSERT( _max_depth > 0 );
          if_enum<T>::pack( s, v, _max_depth - 1 );
        }
        template<typename Stream, typename T>
        static inline void unpack( Stream& s, T& v, uint32_t _max_depth ) {
          FC_ASSERT( _max_depth > 0 );
          if_enum<T>::unpack( s, v, _max_depth - 1 );
        }
      };

    } // namesapce detail

    template<typename Stream, typename T>
    inline void pack( Stream& s, const std::unordered_set<T>& value, uint32_t _max_depth ) {
       FC_ASSERT( _max_depth > 0 );
       --_max_depth;
       fc::raw::pack( s, unsigned_int(value.size()), _max_depth );
       auto itr = value.begin();
       auto end = value.end();
       while( itr != end ) {
          fc::raw::pack( s, *itr, _max_depth );
          ++itr;
       }
    }
    template<typename Stream, typename T>
    inline void unpack( Stream& s, std::unordered_set<T>& value, uint32_t _max_depth ) {
       FC_ASSERT( _max_depth > 0 );
       --_max_depth;
       unsigned_int size; fc::raw::unpack( s, size, _max_depth );
       value.clear();
       value.reserve( std::min( size.value, static_cast<uint64_t>(FC_MAX_PREALLOC_SIZE) ) );
       for( uint32_t i = 0; i < size.value; ++i )
       {
          T tmp;
          fc::raw::unpack( s, tmp, _max_depth );
          value.insert( std::move(tmp) );
       }
    }


    template<typename Stream, typename K, typename V>
    inline void pack( Stream& s, const std::pair<K,V>& value, uint32_t _max_depth ) {
       FC_ASSERT( _max_depth > 0 );
       --_max_depth;
       fc::raw::pack( s, value.first, _max_depth );
       fc::raw::pack( s, value.second, _max_depth );
    }
    template<typename Stream, typename K, typename V>
    inline void unpack( Stream& s, std::pair<K,V>& value, uint32_t _max_depth )
    {
       FC_ASSERT( _max_depth > 0 );
       --_max_depth;
       fc::raw::unpack( s, value.first,  _max_depth );
       fc::raw::unpack( s, value.second, _max_depth );
    }

   template<typename Stream, typename K, typename V>
    inline void pack( Stream& s, const std::unordered_map<K,V>& value, uint32_t _max_depth ) {
       FC_ASSERT( _max_depth > 0 );
       --_max_depth;
       fc::raw::pack( s, unsigned_int(value.size()), _max_depth );
       auto itr = value.begin();
       auto end = value.end();
       while( itr != end ) {
          fc::raw::pack( s, *itr, _max_depth );
          ++itr;
       }
    }
    template<typename Stream, typename K, typename V>
    inline void unpack( Stream& s, std::unordered_map<K,V>& value, uint32_t _max_depth )
    {
       FC_ASSERT( _max_depth > 0 );
       --_max_depth;
       unsigned_int size; fc::raw::unpack( s, size, _max_depth );
       value.clear();
       value.reserve( std::min( size.value, static_cast<uint64_t>(FC_MAX_PREALLOC_SIZE) ) );
       for( uint32_t i = 0; i < size.value; ++i )
       {
          std::pair<K,V> tmp;
          fc::raw::unpack( s, tmp, _max_depth );
          value.insert( std::move(tmp) );
       }
    }
    template<typename Stream, typename K, typename V>
    inline void pack( Stream& s, const std::map<K,V>& value, uint32_t _max_depth ) {
       FC_ASSERT( _max_depth > 0 );
       --_max_depth;
       fc::raw::pack( s, unsigned_int(value.size()), _max_depth );
       auto itr = value.begin();
       auto end = value.end();
       while( itr != end ) {
          fc::raw::pack( s, *itr, _max_depth );
          ++itr;
       }
    }
    template<typename Stream, typename K, typename V>
    inline void unpack( Stream& s, std::map<K,V>& value, uint32_t _max_depth )
    {
       FC_ASSERT( _max_depth > 0 );
       --_max_depth;
       unsigned_int size; fc::raw::unpack( s, size, _max_depth );
       value.clear();
       for( uint32_t i = 0; i < size.value; ++i )
       {
          std::pair<K,V> tmp;
          fc::raw::unpack( s, tmp, _max_depth );
          value.insert( std::move(tmp) );
       }
    }

    template<typename Stream, typename T>
    inline void pack( Stream& s, const std::deque<T>& value, uint32_t _max_depth ) {
       FC_ASSERT( _max_depth > 0 );
       --_max_depth;
       fc::raw::pack( s, unsigned_int(value.size()), _max_depth );
       auto itr = value.begin();
       auto end = value.end();
       while( itr != end ) {
          fc::raw::pack( s, *itr, _max_depth );
          ++itr;
       }
    }

    template<typename Stream, typename T>
    inline void unpack( Stream& s, std::deque<T>& value, uint32_t _max_depth ) {
       FC_ASSERT( _max_depth > 0 );
       --_max_depth;
       unsigned_int size; fc::raw::unpack( s, size, _max_depth );
       value.resize( std::min( size.value, static_cast<uint64_t>(FC_MAX_PREALLOC_SIZE) ) );
       for( uint64_t i = 0; i < size; i++ )
       {
          if( i >= value.size() )
             value.resize( std::min( static_cast<uint64_t>(2*value.size()), size.value ) );
          unpack( s, value[i], _max_depth );
       }
    }

    template<typename Stream, typename T>
    inline void pack( Stream& s, const std::vector<T>& value, uint32_t _max_depth ) {
       FC_ASSERT( _max_depth > 0 );
       --_max_depth;
       fc::raw::pack( s, unsigned_int(value.size()), _max_depth );
       auto itr = value.begin();
       auto end = value.end();
       while( itr != end ) {
          fc::raw::pack( s, *itr, _max_depth );
          ++itr;
       }
    }

    template<typename Stream, typename T>
    inline void unpack( Stream& s, std::vector<T>& value, uint32_t _max_depth ) {
       FC_ASSERT( _max_depth > 0 );
       --_max_depth;
       unsigned_int size; fc::raw::unpack( s, size, _max_depth );
       value.resize( std::min( size.value, static_cast<uint64_t>(FC_MAX_PREALLOC_SIZE) ) );
       for( uint64_t i = 0; i < size; i++ )
       {
          if( i >= value.size() )
             value.resize( std::min( static_cast<uint64_t>(2*value.size()), size.value ) );
          unpack( s, value[i], _max_depth );
       }
    }

    template<typename Stream, typename T>
    inline void pack( Stream& s, const std::set<T>& value, uint32_t _max_depth ) {
       FC_ASSERT( _max_depth > 0 );
       --_max_depth;
       fc::raw::pack( s, unsigned_int(value.size()), _max_depth );
       auto itr = value.begin();
       auto end = value.end();
       while( itr != end ) {
          fc::raw::pack( s, *itr, _max_depth );
          ++itr;
       }
    }

    template<typename Stream, typename T>
    inline void unpack( Stream& s, std::set<T>& value, uint32_t _max_depth ) {
       FC_ASSERT( _max_depth > 0 );
       --_max_depth;
       unsigned_int size; fc::raw::unpack( s, size, _max_depth );
       for( uint64_t i = 0; i < size.value; ++i )
       {
          T tmp;
          fc::raw::unpack( s, tmp, _max_depth );
          value.insert( std::move(tmp) );
       }
    }

    template<typename Stream, boost::endian::order O, class T, std::size_t N, boost::endian::align A>
    void pack( Stream& s, const boost::endian::endian_buffer<O,T,N,A>& v, uint32_t _max_depth )
    {
       FC_ASSERT( _max_depth > 0 );
       s.write( v.data(), sizeof(v) );
    }
    template<typename Stream, boost::endian::order O, class T, std::size_t N, boost::endian::align A>
    void unpack( Stream& s, boost::endian::endian_buffer<O,T,N,A>& v, uint32_t _max_depth )
    {
       FC_ASSERT( _max_depth > 0 );
       s.read( (char*)&v, sizeof(v) );
    }


    template<typename Stream, typename T>
    void pack( Stream& s, const T& v, uint32_t _max_depth ) {
       FC_ASSERT( _max_depth > 0 );
       fc::raw::detail::if_reflected< typename fc::reflector<T>::is_defined >::pack( s, v, _max_depth - 1 );
    }
    template<typename Stream, typename T>
    void unpack( Stream& s, T& v, uint32_t _max_depth )
    { try {
       FC_ASSERT( _max_depth > 0 );
       fc::raw::detail::if_reflected< typename fc::reflector<T>::is_defined >::unpack( s, v, _max_depth - 1 );
    } FC_RETHROW_EXCEPTIONS( warn, "error unpacking ${type}", ("type",fc::get_typename<T>::name() ) ) }

    template<typename T>
    inline size_t pack_size( const T& v )
    {
       datastream<size_t> ps;
       fc::raw::pack( ps, v );
       return ps.tellp();
    }

    template<typename T>
    inline std::vector<char> pack( const T& v, uint32_t _max_depth ) {
       FC_ASSERT( _max_depth > 0 );
       --_max_depth;
       datastream<size_t> ps;
       fc::raw::pack( ps, v, _max_depth );
       std::vector<char> vec(ps.tellp());

       if( vec.size() ) {
          datastream<char*>  ds( vec.data(), size_t(vec.size()) );
          fc::raw::pack( ds, v, _max_depth );
       }
       return vec;
    }

    template<typename T, typename... Next>
    inline std::vector<char> pack(  const T& v, Next... next, uint32_t _max_depth ) {
       FC_ASSERT( _max_depth > 0 );
       --_max_depth;
       datastream<size_t> ps;
       fc::raw::pack( ps, v, next..., _max_depth );
       std::vector<char> vec(ps.tellp());

       if( vec.size() ) {
          datastream<char*>  ds( vec.data(), size_t(vec.size()) );
          fc::raw::pack( ds, v, next..., _max_depth );
       }
       return vec;
    }


    template<typename T>
    inline T unpack( const std::vector<char>& s, uint32_t _max_depth )
    { try  {
       FC_ASSERT( _max_depth > 0 );
       T tmp;
       if( s.size() ) {
          datastream<const char*>  ds( s.data(), size_t(s.size()) );
          fc::raw::unpack( ds, tmp, _max_depth - 1 );
       }
       return tmp;
    } FC_RETHROW_EXCEPTIONS( warn, "error unpacking ${type}", ("type",fc::get_typename<T>::name() ) ) }

    template<typename T>
    inline void unpack( const std::vector<char>& s, T& tmp, uint32_t _max_depth )
    { try  {
       FC_ASSERT( _max_depth > 0 );
       if( s.size() ) {
          datastream<const char*>  ds( s.data(), size_t(s.size()) );
          fc::raw::unpack( ds, tmp, _max_depth - 1 );
       }
    } FC_RETHROW_EXCEPTIONS( warn, "error unpacking ${type}", ("type",fc::get_typename<T>::name() ) ) }

    template<typename T>
    inline void pack( char* d, uint32_t s, const T& v, uint32_t _max_depth ) {
       FC_ASSERT( _max_depth > 0 );
       datastream<char*> ds(d,s);
       fc::raw::pack( ds, v, _max_depth - 1 );
    }

    template<typename T>
    inline T unpack( const char* d, uint32_t s, uint32_t _max_depth )
    { try {
       FC_ASSERT( _max_depth > 0 );
       T v;
       datastream<const char*>  ds( d, s );
       fc::raw::unpack( ds, v, _max_depth - 1 );
       return v;
    } FC_RETHROW_EXCEPTIONS( warn, "error unpacking ${type}", ("type",fc::get_typename<T>::name() ) ) }

    template<typename T>
    inline void unpack( const char* d, uint32_t s, T& v, uint32_t _max_depth )
    { try {
       FC_ASSERT( _max_depth > 0 );
       datastream<const char*>  ds( d, s );
       fc::raw::unpack( ds, v, _max_depth - 1 );
    } FC_RETHROW_EXCEPTIONS( warn, "error unpacking ${type}", ("type",fc::get_typename<T>::name() ) ) }

   template<typename Stream>
   struct pack_static_variant
   {
      Stream& stream;
      const uint32_t max_depth;
      pack_static_variant( Stream& s, uint32_t _max_depth ):stream(s),max_depth(_max_depth - 1)
      {
         FC_ASSERT( _max_depth > 0 );
      }

      typedef void result_type;
      template<typename T> void operator()( const T& v )const
      {
         fc::raw::pack( stream, v, max_depth );
      }
   };

   template<typename Stream>
   struct unpack_static_variant
   {
      Stream& stream;
      const uint32_t max_depth;
      unpack_static_variant( Stream& s, uint32_t _max_depth ) : stream(s),max_depth(_max_depth - 1)
      {
         FC_ASSERT( _max_depth > 0 );
      }

      typedef void result_type;
      template<typename T> void operator()( T& v )const
      {
         fc::raw::unpack( stream, v, max_depth );
      }
   };


    template<typename Stream, typename... T>
    void pack( Stream& s, const static_variant<T...>& sv, uint32_t _max_depth )
    {
       FC_ASSERT( _max_depth > 0 );
       --_max_depth;
       fc::raw::pack( s, unsigned_int(sv.which()), _max_depth );
       sv.visit( pack_static_variant<Stream>( s, _max_depth ) );
    }

    template<typename Stream, typename... T> void unpack( Stream& s, static_variant<T...>& sv, uint32_t _max_depth )
    {
       FC_ASSERT( _max_depth > 0 );
       --_max_depth;
       unsigned_int w;
       fc::raw::unpack( s, w, _max_depth );
       sv.set_which(w.value);
       sv.visit( unpack_static_variant<Stream>( s, _max_depth ) );
    }

} } // namespace fc::raw

