/**
 *  @file
 *  @description: This file is used to mock the features needed to run and test
 *    smart contracts.
 */
#pragma once
#include <boost/any.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>
#include <fc/crypto/public_key.hpp>

using uint128_t = unsigned __int128;
using int128_t  =  __int128;

// copying "asset.hpp", "symbol.hpp" and "name.hpp" from eosio.cdt (v1.6.3) into "mock_lib"
// inorder to use the correct definitions needed for contract development
#include "mock_lib/asset.hpp"
#include "mock_lib/symbol.hpp"
#include "mock_lib/name.hpp"

#include <stdexcept>
#include <utility>

#include <map>

namespace eosio {
   using eosio::asset;
   using eosio::name;
   using eosio::symbol;
   using eosio::symbol_code;
   using fc::days;
   using fc::microseconds;
   using fc::time_point;
   using fc::crypto::public_key;
} // namespace eosio

using uint128_t = unsigned __int128;


eosio::time_point operator""_tp(const char* str, unsigned long len) { return eosio::time_point::from_iso_string(str); }

uint64_t operator""_tp_usc(const char* str, unsigned long len) {
   return eosio::time_point::from_iso_string(str).time_since_epoch().count();
}

namespace fc {
   doctest::String toString(const time_point& value) { return static_cast<std::string>(value).c_str(); }
} // namespace fc

void print() {}
template <typename T, typename... Args>
void print(T&& first, Args... args) {
   std::cout << first;
   print(args...);
}

using std::string;

static constexpr uint64_t char_to_symbol( char c ) {
   if( c >= 'a' && c <= 'z' )
      return (c - 'a') + 6;
   if( c >= '1' && c <= '5' )
      return (c - '1') + 1;
   return 0;
}

// Each char of the string is encoded into 5-bit chunk and left-shifted
// to its 5-bit slot starting with the highest slot for the first char.
// The 13th char, if str is long enough, is encoded into 4-bit chunk
// and placed in the lowest 4 bits. 64 = 12 * 5 + 4
static constexpr uint64_t string_to_name( const char* str )
{
   uint64_t name = 0;
   int i = 0;
   for ( ; str[i] && i < 12; ++i) {
         // NOTE: char_to_symbol() returns char type, and without this explicit
         // expansion to uint64 type, the compilation fails at the point of usage
         // of string_to_name(), where the usage requires constant (compile time) expression.
         name |= (char_to_symbol(str[i]) & 0x1f) << (64 - 5 * (i + 1));
      }

   // The for-loop encoded up to 60 high bits into uint64 'name' variable,
   // if (strlen(str) > 12) then encode str[12] into the low (remaining)
   // 4 bits of 'name'
   if (i == 12)
         name |= char_to_symbol(str[12]) & 0x0F;
   return name;
}

#define SYSCONTATTRIBUTE
#define SYSCON_TABLE(name)

constexpr uint64_t operator""_n(const char* str, unsigned long len) { return string_to_name(str); }


namespace eosio {
   const name same_payer{};

   // moved to mock_lib/system.hpp
   // void check(bool condition, const char* msg) {
   //    if (!(condition))
   //       throw std::runtime_error(msg);
   // }

   struct enable_multi_index {
      typedef uint64_t scope_t;
      typedef uint64_t table_id_t;

      typedef std::map<name, std::map<table_id_t, std::map<scope_t, boost::any>>> data_store_type;

      static data_store_type& data_store() {
         static data_store_type inst;
         return inst;
      }

      name code;

      enable_multi_index(name c)
         : code(c) {}

      ~enable_multi_index() { data_store()[code].clear(); }
   };

   using boost::multi_index::const_mem_fun;

   template <uint64_t IndexName, typename Extractor>
   struct indexed_by : std::pair<std::integral_constant<uint64_t, IndexName>, Extractor> {};

   template <uint64_t N, typename T, typename... IndexBys>
   struct multi_index {
      typedef decltype(((T*)nullptr)->primary_key()) key_type;
      typedef boost::multi_index::multi_index_container<
          T,
          boost::multi_index::indexed_by<boost::multi_index::ordered_unique<const_mem_fun<T, key_type, &T::primary_key>>,
                                         boost::multi_index::ordered_non_unique<boost::multi_index::tag<typename IndexBys::first_type>,
                                                                                typename IndexBys::second_type>...>>
                                                           impl_t;
      typedef typename impl_t::template nth_index<0>::type primary_index_t;
      typedef typename primary_index_t::const_iterator     const_iterator;

      const_iterator find(key_type key) const { return get_impl().template get<0>().find(key); }
      const T&       get(key_type primary, const char* error_msg = "unable to find key") const {
         auto result = find(primary);
         eosio::check(result != end(), error_msg);
         return *result;
      }
      const_iterator begin() const { return get_impl().template get<0>().begin(); }
      const_iterator end() const { return get_impl().template get<0>().end(); }

      const_iterator upper_bound( key_type primary ) const { return get_impl().template get<0>().upper_bound(primary); }
      const_iterator lower_bound( key_type primary ) const { return get_impl().template get<0>().lower_bound(primary); }

      enum next_primary_key_tags : uint64_t {
         no_available_primary_key = static_cast<uint64_t>(-2), // Must be the smallest uint64_t value compared to all other tags
         unset_next_primary_key = static_cast<uint64_t>(-1)
      };

      mutable uint64_t _next_primary_key;

      uint64_t available_primary_key()const {
         if( _next_primary_key == unset_next_primary_key ) {
            // This is the first time available_primary_key() is called for this multi_index instance.
            if( begin() == end() ) { // empty table
               _next_primary_key = 0;
            } else {
               auto itr = --end(); // last row of table sorted by primary key
               auto pk = itr->primary_key(); // largest primary key currently in table
               if( pk >= no_available_primary_key ) // Reserve the tags
                  _next_primary_key = no_available_primary_key;
               else
                  _next_primary_key = pk + 1;
            }
         }

         eosio::check( _next_primary_key < no_available_primary_key, "next primary key in table is at autoincrement limit");
         return _next_primary_key;
      }

      template <typename IDX>
      struct secondary_index_t {
         IDX* index;
         auto begin() const { return index->begin(); }
         auto end() const { return index->end(); }
         auto rbegin() const { return index->rbegin(); }
         auto rend() const { return index->rend(); }
         template <typename Lambda>
         void modify(typename IDX::const_iterator itr, eosio::name, Lambda&& lambda) {
            index->modify(itr, std::forward<Lambda>(lambda));
         }
         template <typename Lambda>
         void modify(typename IDX::const_reference v, eosio::name, Lambda&& lambda) {
            index->modify(index->iterato_to(v), std::forward<Lambda>(lambda));
         }
         auto find(typename IDX::key_type key) { return index->find(key); }
         auto erase(typename IDX::const_iterator itr) { return index->erase(itr); }
         auto upper_bound(typename IDX::key_type primary) { return index->upper_bound(primary); }
         auto lower_bound(typename IDX::key_type primary) { return index->lower_bound(primary); }
      };

      template <typename IDX>
      static secondary_index_t<IDX> make_secondary_index(IDX& index_impl) {
         return secondary_index_t<IDX>{&index_impl};
      }

      template <uint64_t IndexName>
      auto get_index() {
         return make_secondary_index(get_impl().template get<std::integral_constant<uint64_t, IndexName>>());
      }

      template <typename Lambda>
      const_iterator emplace(eosio::name, Lambda&& lambda) {
         T value;
         lambda(value);
         auto r = get_impl().insert(std::move(value));
         if (r.second)
            return get_impl().template get<0>().iterator_to(*r.first);
         throw std::runtime_error("duplicated key");
      }

      template <typename Lambda>
      void modify(const_iterator itr, eosio::name, Lambda&& lambda) {
         get_impl().template get<0>().modify(itr, lambda);
      }

      const_iterator iterator_to(const T& obj) { return get_impl().template get<0>().iterator_to(obj); }

      const_iterator erase(const_iterator itr) { return get_impl().template get<0>().erase(itr); }

      eosio::name get_code() const { return code; }

      multi_index(eosio::name c, uint64_t s)
         : code(c) {
         auto& storage = enable_multi_index::data_store()[c][N][s];
         if (storage.empty()) {
            storage = impl_t{};
         }
         ptr = boost::any_cast<impl_t>(&storage);
      }
      impl_t& get_impl() { return *ptr; }

      const impl_t&      get_impl() const { return *ptr; }
      eosio::name code;
      impl_t*            ptr;
   };

   template <uint64_t N, typename T>
   struct multi_index<N, T> {
      typedef decltype(((T*)nullptr)->primary_key()) key_type;
      typedef std::map<key_type, T>                  impl_t;

      struct const_iterator {
         typedef typename impl_t::iterator base_type;
         base_type                         base;
         const_iterator(base_type itr)
            : base(itr) {}
         T* operator->() const { return &base->second; }
         T& operator*() const { return base->second; }

         const_iterator& operator--() {
            --base;
            return *this;
         }
         const_iterator operator++(int) { return const_iterator(base++); }
         const_iterator& operator++() {
            ++base;
            return *this;
         }
         const_iterator operator--(int) { return const_iterator(base--); }

         friend bool operator==(const_iterator lhs, const_iterator rhs) { return lhs.base == rhs.base; }
         friend bool operator!=(const_iterator lhs, const_iterator rhs) { return !(lhs == rhs); }
      };

      multi_index(eosio::name c, uint64_t s)
         : code(c) {
         auto& storage = enable_multi_index::data_store()[c][N][s];
         if (storage.empty()) {
            storage = impl_t{};
         }
         ptr = boost::any_cast<impl_t>(&storage);
      }
      impl_t& get_impl() { return *ptr; }

      const impl_t& get_impl() const { return *ptr; }

      template <typename Lambda>
      const_iterator emplace(eosio::name, Lambda&& lambda) {
         T value;
         lambda(value);
         return const_iterator(get_impl().emplace(value.primary_key(), value).first);
      }

      const_iterator find(key_type key) { return const_iterator{get_impl().find(key)}; }
      const T&       get(key_type primary, const char* error_msg = "unable to find key") {
         auto result = find(primary);
         eosio::check(result != end(), error_msg);
         return *result;
      }

      const_iterator begin() { return const_iterator{get_impl().begin()}; }
      const_iterator end() { return const_iterator{get_impl().end()}; }

      template <typename Lambda>
      void modify(const_iterator itr, eosio::name, Lambda&& lambda) {
         lambda(*itr);
      }

      void erase(const_iterator itr) { get_impl().erase(itr.base); }

      eosio::name get_code() const { return code; }
      eosio::name code;
      impl_t*            ptr;
   };

   template<uint64_t SingletonName, typename T>
   struct singleton {
      constexpr static uint64_t pk_value = static_cast<uint64_t>(SingletonName);
      /**
       * Structure of data inside the singleton table
       */
      struct row {
         /**
          * Value to be stored inside the singleton table
          */
         T value;

         /**
          * Get primary key of the data
          *
          * @return uint64_t - Primary Key
          */
         uint64_t primary_key() const { return pk_value; }
      };
      typedef eosio::multi_index<SingletonName, row> table;

      public:
         /**
          * Construct a new singleton object given the table's owner and the scope
          *
          * @param code - The table's owner
          * @param scope - The scope of the table
          */
         singleton( name code, uint64_t scope ) : _t( code, scope ) {}

         /**
          *  Check if the singleton table exists
          *
          * @return true - if exists
          * @return false - otherwise
          */
         bool exists() {
            return _t.find( pk_value ) != _t.end();
         }

         /**
          * Get the value stored inside the singleton table. Will throw an exception if it doesn't exist
          *
          * @brief Get the value stored inside the singleton table
          * @return T - The value stored
          */
         T get() {
            auto itr = _t.find( pk_value );
            eosio::check( itr != _t.end(), "singleton does not exist" );
            return itr->value;
         }

         /**
          * Get the value stored inside the singleton table. If it doesn't exist, it will return the specified default value
          *
          * @param def - The default value to be returned in case the data doesn't exist
          * @return T - The value stored
          */
         T get_or_default( const T& def = T() ) {
            auto itr = _t.find( pk_value );
            return itr != _t.end() ? itr->value : def;
         }

         /**
          * Get the value stored inside the singleton table. If it doesn't exist, it will create a new one with the specified default value
          *
          * @param bill_to_account - The account to bill for the newly created data if the data doesn't exist
          * @param def - The default value to be created in case the data doesn't exist
          * @return T - The value stored
          */
         T get_or_create( name bill_to_account, const T& def = T() ) {
            auto itr = _t.find( pk_value );
            return itr != _t.end() ? itr->value
               : _t.emplace(bill_to_account, [&](row& r) { r.value = def; })->value;
         }

         /**
          * Set new value to the singleton table
          *
          * @param value - New value to be set
          * @param bill_to_account - Account to pay for the new value
          */
         void set( const T& value, name bill_to_account ) {
            auto itr = _t.find( pk_value );
            if( itr != _t.end() ) {
               _t.modify(itr, bill_to_account, [&](row& r) { r.value = value; });
            } else {
               _t.emplace(bill_to_account, [&](row& r) { r.value = value; });
            }
         }

         /**
          * Remove the only data inside singleton table
          */
         void remove( ) {
            auto itr = _t.find( pk_value );
            if( itr != _t.end() ) {
               _t.erase(itr);
            }
         }

      private:
         table _t;
   };

} // namespace eosio
