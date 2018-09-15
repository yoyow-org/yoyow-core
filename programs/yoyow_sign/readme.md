# Ubunt16.04, GCC/G++ 5.4 And macOS, Clang 9.0.0

## CMakeLists.txt:
   ```
   set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11 -Wall" )
   ```
   to
   ```
   set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11 -Wall -fPIC" )
   ```
## libraries/chain/protocol/fee_schedule.cpp:(Under macOS, you only need to modify it here.)
   ```
   //template graphene::chain::fee_schedule& smart_ref<graphene::chain::fee_schedule>::operator=(smart_ref<graphene::chain::fee_schedule>&&);
   //template graphene::chain::fee_schedule& smart_ref<graphene::chain::fee_schedule>::operator=(U&&);
   //template graphene::chain::fee_schedule& smart_ref<graphene::chain::fee_schedule>::operator=(const smart_ref&);
   //template smart_ref<graphene::chain::fee_schedule>::smart_ref();
   //template const graphene::chain::fee_schedule& smart_ref<graphene::chain::fee_schedule>::operator*() const;
   ```
   to
   ```
   template smart_ref<graphene::chain::fee_schedule>::smart_ref();
   template smart_ref<graphene::chain::fee_schedule>::smart_ref(smart_ref<graphene::chain::fee_schedule>&&);
   template smart_ref<graphene::chain::fee_schedule>::smart_ref(smart_ref<graphene::chain::fee_schedule> const&);
   template graphene::chain::fee_schedule& smart_ref<graphene::chain::fee_schedule>::operator*();
   template const graphene::chain::fee_schedule& smart_ref<graphene::chain::fee_schedule>::operator*() const;
   ```
## libraries/fc/include/fc/smart_ref_fwd.hpp:
   ```
   template<typename U> smart_ref( U&& u );
   ```
   to
   ```
   //template<typename U> smart_ref( U&& u );
   ```

## libraries/fc/include/fc/smart_ref_impl.hpp:
   ```
   template<typename T>
   template<typename U>
   smart_ref<T>::smart_ref( U&& u ) {
      impl = new (this) T( fc::forward<U>(u) );
   }
   ```
   to
   ```
   /* template<typename T>
    template<typename U>
    smart_ref<T>::smart_ref( U&& u ) {
      impl = new (this) T( fc::forward<U>(u) );
    } */
   ```

## boost:
   ```
   GCCJAM=`find ./ -name gcc.jam`
   sed -i "s/if \$(link) = shared/if \$(link) = shared \|\| \$(link) = static/g" $GCCJAM
   ./bootstrap.sh --prefix=/opt/boost
   ./b2 link=static runtime-link=static variant=release install
   ```

## openssl:
   ```./config -fPIC --prefix=/opt/openssl```

## libraries/fc/vendor/secp256k1-zkp/configure.ac:
   ```
   CFLAGS="$CFLAGS -W"
   ```
   to
   ```
   CFLAGS="$CFLAGS -W -fPIC"
   ```

# build:
   ```
   mkdir build
   cd build
   cmake -DBOOST_ROOT=/opt/boost -DOPENSSL_ROOT_DIR=/opt/openssl -DCMAKE_BUILD_TYPE=Release ../
   make yoyow_sign
   ```

# build test:
   ```
   cd test
   g++ sign.cpp -I../ -L../../../build/programs/yoyow_sign -lyoyow_sign -o sign
   ```