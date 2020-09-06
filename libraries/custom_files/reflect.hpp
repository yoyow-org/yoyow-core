#pragma once
/**
 * @file fc/reflect/reflect.hpp
 *
 * @brief Defines types and macros used to provide reflection.
 *
 */

#include <fc/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/seq/seq.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <stdint.h>
#include <string.h>
#include <type_traits>

#include <fc/reflect/typename.hpp>
#include <fc/reflect/typelist.hpp>

namespace fc {

template<typename> struct reflector;
namespace member_names {
/// A template which stores the name of the native member at a given index in a given class
template<typename Class, std::size_t index> struct member_name {
   constexpr static const char* value = "Unknown member";
};
}

/**
 *  @brief A template to store compile-time information about a field in a reflected struct
 *
 *  @tparam Container The type of the struct or class containing the field
 *  @tparam Member The type of the field
 *  @tparam field A pointer-to-member for the reflected field
 */
template<std::size_t Index, typename Container, typename Member, Member Container::*field>
struct field_reflection {
   using container = Container;
   using type = Member;
   using reflector = fc::reflector<type>;
   constexpr static std::size_t index = Index;
   constexpr static bool is_derived = false;
   constexpr static type container::*pointer = field;

   /// @brief Given a reference to the container type, get a reference to the field
   static type& get(container& c) { return c.*field; }
   static const type& get(const container& c) { return c.*field; }
   /// @brief Get the name of the field
   static const char* get_name() { return fc::member_names::member_name<container, index>::value; }
};
/// Basically the same as @ref field_reflection, but for inherited fields
/// Note that inherited field reflections do not have an index field; indexes are for native fields only
template<std::size_t IndexInBase, typename Base, typename Derived, typename Member, Member Base::*field>
struct inherited_field_reflection {
   using container = Derived;
   using field_container = Base;
   using type = Member;
   using reflector = fc::reflector<type>;
   constexpr static std::size_t index_in_base = IndexInBase;
   constexpr static bool is_derived = true;
   constexpr static type field_container::*pointer = field;

   static type& get(container& c) {
      // And we need a distinct inherited_field_reflection type because this conversion can't be done statically
      type container::* derived_field = field;
      return c.*derived_field;
   }
   static const type& get(const container& c) {
      type container::* derived_field = field;
      return c.*derived_field;
   }
   static const char* get_name() {
      using Reflector = typename fc::reflector<Base>::native_members::template at<IndexInBase>;
      return Reflector::get_name();
   }
};

namespace impl {
/// Helper template to create a @ref field_reflection without any commas (makes it macro-friendly)
template<typename Container>
struct Reflect_type {
   template<typename Member>
   struct with_field_type {
      template<std::size_t Index>
      struct at_index {
         template<Member Container::*field>
         struct with_field_pointer {
            using type = field_reflection<Index, Container, Member, field>;
         };
      };
   };
};
/// Template to make a transformer of a @ref field_reflection from a base class to a derived class
template<typename Derived>
struct Derivation_reflection_transformer {
   template<typename> struct transform;
   template<std::size_t IndexInBase, typename BaseContainer, typename Member, Member BaseContainer::*field>
   struct transform<field_reflection<IndexInBase, BaseContainer, Member, field>> {
       using type = inherited_field_reflection<IndexInBase, BaseContainer, Derived, Member, field>;
   };
   template<std::size_t IndexInBase, typename BaseContainer, typename IntermediateContainer,
            typename Member, Member BaseContainer::*field>
   struct transform<inherited_field_reflection<IndexInBase, BaseContainer, IntermediateContainer, Member, field>> {
       using type = inherited_field_reflection<IndexInBase, BaseContainer, Derived, Member, field>;
   };
};
} // namespace impl

/// Macro to transform reflected fields of a base class to a derived class and concatenate them to a type list
#define FC_CONCAT_BASE_MEMBER_REFLECTIONS(r, derived, base) \
   ::add_list<typelist::transform<reflector<base>::members, impl::Derivation_reflection_transformer<derived>>>
/// Macro to concatenate a new @ref field_reflection to a typelist
#define FC_CONCAT_MEMBER_REFLECTION(r, container, idx, member) \
   ::add<typename impl::Reflect_type<container>::template with_field_type<decltype(container::member)> \
                                               ::template at_index<idx> \
                                               ::template with_field_pointer<&container::member>::type>
#define FC_REFLECT_MEMBER_NAME(r, container, idx, member) \
   template<> struct member_name<container, idx> { constexpr static const char* value = BOOST_PP_STRINGIZE(member); };
#define FC_REFLECT_TEMPLATE_MEMBER_NAME(r, data, idx, member) \
   template<BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_ELEM(0, data))> struct member_name<BOOST_PP_SEQ_ELEM(1, data), idx> { \
    constexpr static const char* value = BOOST_PP_STRINGIZE(member); };
/// Macro to concatenate a new type to a typelist
#define FC_CONCAT_TYPE(r, x, TYPE) ::add<TYPE>

/**
 *  @brief defines visit functions for T
 *  Unless this is specialized, visit() will not be defined for T.
 *
 *  @tparam T - the type that will be visited.
 *
 *  The @ref FC_REFLECT(TYPE,MEMBERS) or FC_STATIC_REFLECT_DERIVED(TYPE,BASES,MEMBERS) macro is used to specialize this
 *  class for your type.
 */
template<typename T>
struct reflector{
    typedef T type;
    typedef std::false_type is_defined;
    /// A typelist with a @ref field_reflection for each native member (non-inherited) of the struct
    using native_members = typelist::list<>;
    /// A typelist with a @ref field_reflection for each inherited member of the struct
    using inherited_members = typelist::list<>;
    /// A typelist with a @ref field_reflection for each member of the struct, starting with inherited members
    using members = typelist::list<>;
    /// A typelist of base classes for this type
    using base_classes = typelist::list<>;

    /**
     *  @tparam Visitor a function object of the form:
     *
     *    @code
     *     struct functor {
     *        template<typename Member, class Class, Member (Class::*member)>
     *        void operator()( const char* name )const;
     *     };
     *    @endcode
     *
     *  If T is an enum then the functor has the following form:
     *    @code
     *     struct functor {
     *        template<int Value>
     *        void operator()( const char* name )const;
     *     };
     *    @endcode
     *
     *  @param v a functor that will be called for each member on T
     *
     *  @note - this method is not defined for non-reflected types.
     */
    #ifdef DOXYGEN
    template<typename Visitor>
    static inline void visit( const Visitor& v );
    #endif // DOXYGEN
};

void throw_bad_enum_cast( int64_t i, const char* e );
void throw_bad_enum_cast( const char* k, const char* e );

template <typename Class>
struct reflector_verifier_visitor {
   explicit reflector_verifier_visitor( Class& c )
     : obj(c) {}

   void reflector_verify() {
      reflect_verify( obj );
   }

 private:

   // int matches 0 if reflector_verify exists SFINAE
   template<class T>
   auto verify_imp(const T& t, int) -> decltype(t.reflector_verify(), void()) {
      t.reflector_verify();
   }

   // if no reflector_verify method exists (SFINAE), 0 matches long
   template<class T>
   auto verify_imp(const T& t, long) -> decltype(t, void()) {}

   template<typename T>
   auto reflect_verify(const T& t) -> decltype(verify_imp(t, 0), void()) {
      verify_imp(t, 0);
   }

 protected:
   Class& obj;
};

} // namespace fc


#ifndef DOXYGEN

#define FC_REFLECT_VISIT_BASE(r, visitor, base) \
  fc::reflector<base>::visit( visitor );


#ifndef _MSC_VER
  #define TEMPLATE template
#else
  // Disable warning C4482: nonstandard extention used: enum 'enum_type::enum_value' used in qualified name
  #pragma warning( disable: 4482 )
  #define TEMPLATE
#endif

#define FC_REFLECT_VISIT_MEMBER( r, visitor, elem ) \
{ typedef decltype(((type*)nullptr)->elem) member_type;  \
  visitor.TEMPLATE operator()<member_type,type,&type::elem>( BOOST_PP_STRINGIZE(elem) ); \
}

#define FC_REFLECT_VISIT_MEMBER_I( r, visitor, I, elem ) \
   case I: FC_REFLECT_VISIT_MEMBER( r, visitor, elem ) break;


#define FC_REFLECT_DERIVED_IMPL_INLINE( TYPE, INHERITS, MEMBERS ) \
template<typename Visitor>\
static inline void visit( const Visitor& v ) { \
    BOOST_PP_SEQ_FOR_EACH( FC_REFLECT_VISIT_BASE, v, INHERITS ) \
    BOOST_PP_SEQ_FOR_EACH( FC_REFLECT_VISIT_MEMBER, v, MEMBERS ) \
}

#endif // DOXYGEN


#define FC_REFLECT_VISIT_ENUM( r, enum_type, elem ) \
  v.operator()(BOOST_PP_STRINGIZE(elem), int64_t(enum_type::elem) );
#define FC_REFLECT_ENUM_TO_STRING( r, enum_type, elem ) \
   case enum_type::elem: return BOOST_PP_STRINGIZE(elem);
#define FC_REFLECT_ENUM_TO_FC_STRING( r, enum_type, elem ) \
   case enum_type::elem: return std::string(BOOST_PP_STRINGIZE(elem));

#define FC_REFLECT_ENUM_FROM_STRING( r, enum_type, elem ) \
  if( strcmp( s, BOOST_PP_STRINGIZE(elem)  ) == 0 ) return enum_type::elem;
#define FC_REFLECT_ENUM_FROM_STRING_CASE( r, enum_type, elem ) \
   case enum_type::elem:

#define FC_REFLECT_ENUM( ENUM, FIELDS ) \
namespace fc { \
template<> struct reflector<ENUM> { \
    typedef std::true_type is_defined; \
    static const char* to_string(ENUM elem) { \
      switch( elem ) { \
        BOOST_PP_SEQ_FOR_EACH( FC_REFLECT_ENUM_TO_STRING, ENUM, FIELDS ) \
        default: \
           fc::throw_bad_enum_cast( fc::to_string(int64_t(elem)).c_str(), BOOST_PP_STRINGIZE(ENUM) ); \
      }\
      return nullptr; \
    } \
    static const char* to_string(int64_t i) { \
      return to_string(ENUM(i)); \
    } \
    static std::string to_fc_string(ENUM elem) { \
      switch( elem ) { \
        BOOST_PP_SEQ_FOR_EACH( FC_REFLECT_ENUM_TO_FC_STRING, ENUM, FIELDS ) \
      } \
      return fc::to_string(int64_t(elem)); \
    } \
    static std::string to_fc_string(int64_t i) { \
      return to_fc_string(ENUM(i)); \
    } \
    static ENUM from_int(int64_t i) { \
      ENUM e = ENUM(i); \
      switch( e ) \
      { \
        BOOST_PP_SEQ_FOR_EACH( FC_REFLECT_ENUM_FROM_STRING_CASE, ENUM, FIELDS ) \
          break; \
        default: \
          fc::throw_bad_enum_cast( i, BOOST_PP_STRINGIZE(ENUM) ); \
      } \
      return e;\
    } \
    static ENUM from_string( const char* s ) { \
        BOOST_PP_SEQ_FOR_EACH( FC_REFLECT_ENUM_FROM_STRING, ENUM, FIELDS ) \
        int64_t i = 0; \
        try \
        { \
           i = boost::lexical_cast<int64_t>(s); \
        } \
        catch( const boost::bad_lexical_cast& ) \
        { \
           fc::throw_bad_enum_cast( s, BOOST_PP_STRINGIZE(ENUM) ); \
        } \
        return from_int(i); \
    } \
    template< typename Visitor > \
    static void visit( Visitor& v ) \
    { \
        BOOST_PP_SEQ_FOR_EACH( FC_REFLECT_VISIT_ENUM, ENUM, FIELDS ) \
    } \
};  \
template<> struct get_typename<ENUM>  { static const char* name()  { return BOOST_PP_STRINGIZE(ENUM);  } }; \
}

/*  Note: FC_REFLECT_ENUM previously defined this function, but I don't think it ever
 *        did what we expected it to do.  I've disabled it for now.
 *
 *  template<typename Visitor> \
 *  static inline void visit( const Visitor& v ) { \
 *      BOOST_PP_SEQ_FOR_EACH( FC_REFLECT_VISIT_ENUM, ENUM, FIELDS ) \
 *  }\
 */

/**
 *  @def FC_REFLECT_DERIVED(TYPE,INHERITS,MEMBERS)
 *
 *  @brief Specializes fc::reflector for TYPE where
 *         type inherits other reflected classes
 *
 *  @param INHERITS - a sequence of base class names (basea)(baseb)(basec)
 *  @param MEMBERS - a sequence of member names.  (field1)(field2)(field3)
 */
#define FC_REFLECT_DERIVED( TYPE, INHERITS, MEMBERS ) \
namespace fc {  \
  template<> struct get_typename<TYPE>  { static const char* name()  { return BOOST_PP_STRINGIZE(TYPE);  } }; \
template<> struct reflector<TYPE> {\
    typedef TYPE type; \
    typedef std::true_type  is_defined; \
    using native_members = \
       typename typelist::builder<>::type \
          BOOST_PP_SEQ_FOR_EACH_I( FC_CONCAT_MEMBER_REFLECTION, TYPE, MEMBERS ) ::finalize; \
    using inherited_members = \
       typename typelist::builder<>::type \
          BOOST_PP_SEQ_FOR_EACH( FC_CONCAT_BASE_MEMBER_REFLECTIONS, TYPE, INHERITS ) ::finalize; \
    using members = typename typelist::concat<inherited_members, native_members>::type; \
    using base_classes = typename typelist::builder<>::type \
          BOOST_PP_SEQ_FOR_EACH( FC_CONCAT_TYPE, x, INHERITS ) ::finalize; \
    enum  member_count_enum {  \
      local_member_count = typelist::length<native_members>(), \
      total_member_count = typelist::length<members>() \
    }; \
    FC_REFLECT_DERIVED_IMPL_INLINE( TYPE, INHERITS, MEMBERS ) \
}; \
namespace member_names { \
BOOST_PP_SEQ_FOR_EACH_I( FC_REFLECT_MEMBER_NAME, TYPE, MEMBERS ) \
} }

#define FC_REFLECT_DERIVED_TEMPLATE( TEMPLATE_ARGS, TYPE, INHERITS, MEMBERS ) \
namespace fc {  \
  template<BOOST_PP_SEQ_ENUM(TEMPLATE_ARGS)> struct get_typename<TYPE> { \
    static const char* name() { return BOOST_PP_STRINGIZE(TYPE); } \
  }; \
template<BOOST_PP_SEQ_ENUM(TEMPLATE_ARGS)> struct reflector<TYPE> {\
    typedef TYPE type; \
    typedef std::true_type  is_defined; \
    using native_members = \
       typename typelist::builder<>::type \
          BOOST_PP_SEQ_FOR_EACH_I( FC_CONCAT_MEMBER_REFLECTION, TYPE, MEMBERS ) ::finalize; \
    using inherited_members = \
       typename typelist::builder<>::type \
          BOOST_PP_SEQ_FOR_EACH( FC_CONCAT_BASE_MEMBER_REFLECTIONS, TYPE, INHERITS ) ::finalize; \
    using members = typename typelist::concat<inherited_members, native_members>::type; \
    using base_classes = typename typelist::builder<>::type \
          BOOST_PP_SEQ_FOR_EACH( FC_CONCAT_TYPE, x, INHERITS ) ::finalize; \
    enum  member_count_enum {  \
      local_member_count = typelist::length<native_members>(), \
      total_member_count = typelist::length<members>() \
    }; \
    FC_REFLECT_DERIVED_IMPL_INLINE( TYPE, INHERITS, MEMBERS ) \
}; \
namespace member_names { \
BOOST_PP_SEQ_FOR_EACH_I( FC_REFLECT_TEMPLATE_MEMBER_NAME, (TEMPLATE_ARGS)(TYPE), MEMBERS ) \
} }

#define FC_REFLECT_DERIVED_NO_TYPENAME( TYPE, INHERITS, MEMBERS ) \
namespace fc { \
template<> struct reflector<TYPE> {\
    typedef TYPE type; \
    typedef std::true_type is_defined; \
    using native_members = \
       typename typelist::builder<>::type \
          BOOST_PP_SEQ_FOR_EACH_I( FC_CONCAT_MEMBER_REFLECTION, TYPE, MEMBERS ) ::finalize; \
    using inherited_members = \
       typename typelist::builder<>::type \
          BOOST_PP_SEQ_FOR_EACH( FC_CONCAT_BASE_MEMBER_REFLECTIONS, TYPE, INHERITS ) ::finalize; \
    using members = typename typelist::concat<inherited_members, native_members>::type; \
    using base_classes = typename typelist::builder<>::type \
          BOOST_PP_SEQ_FOR_EACH( FC_CONCAT_TYPE, x, INHERITS ) ::finalize; \
    enum  member_count_enum {  \
      local_member_count = typelist::length<native_members>(), \
      total_member_count = typelist::length<members>() \
    }; \
    FC_REFLECT_DERIVED_IMPL_INLINE( TYPE, INHERITS, MEMBERS ) \
}; \
} // fc

/**
 *  @def FC_REFLECT(TYPE,MEMBERS)
 *  @brief Specializes fc::reflector for TYPE
 *
 *  @param MEMBERS - a sequence of member names.  (field1)(field2)(field3)
 *
 *  @see FC_REFLECT_DERIVED
 */
#define FC_REFLECT( TYPE, MEMBERS ) \
   FC_REFLECT_DERIVED( TYPE, BOOST_PP_SEQ_NIL, MEMBERS )


#define FC_REFLECT_TEMPLATE( TEMPLATE_ARGS, TYPE, MEMBERS ) \
    FC_REFLECT_DERIVED_TEMPLATE( TEMPLATE_ARGS, TYPE, BOOST_PP_SEQ_NIL, MEMBERS )

#define FC_REFLECT_EMPTY( TYPE ) \
    FC_REFLECT_DERIVED( TYPE, BOOST_PP_SEQ_NIL, BOOST_PP_SEQ_NIL )

#define FC_REFLECT_TYPENAME( TYPE ) \
namespace fc { \
  template<> struct get_typename<TYPE>  { static const char* name()  { return BOOST_PP_STRINGIZE(TYPE);  } }; \
}

