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
#include <cctype>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <list>

#include <boost/version.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/range/algorithm/unique.hpp>
#include <boost/range/algorithm/sort.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <fc/git_revision.hpp>
#include <fc/io/fstream.hpp>
#include <fc/io/json.hpp>
#include <fc/io/stdio.hpp>
#include <fc/network/http/websocket.hpp>
#include <fc/rpc/cli.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <fc/crypto/aes.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/thread/scoped_lock.hpp>

#include <graphene/app/api.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/config.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <graphene/utilities/git_revision.hpp>
#include <graphene/utilities/key_conversion.hpp>
#include <graphene/utilities/string_escape.hpp>
#include <graphene/utilities/words.hpp>
#include <graphene/wallet/wallet.hpp>
#include <graphene/wallet/api_documentation.hpp>
#include <graphene/wallet/reflect_util.hpp>
#include <graphene/debug_witness/debug_api.hpp>
#include <fc/smart_ref_impl.hpp>

#ifndef WIN32
# include <sys/types.h>
# include <sys/stat.h>
#endif

#define BRAIN_KEY_WORD_COUNT 16

namespace graphene { namespace wallet {

namespace detail {

struct operation_result_printer
{
public:
   explicit operation_result_printer( const wallet_api_impl& w )
      : _wallet(w) {}
   const wallet_api_impl& _wallet;
   typedef std::string result_type;

   std::string operator()(const void_result& x) const;
   std::string operator()(const object_id_type& oid);
   std::string operator()(const asset& a);
   std::string operator()(const advertising_confirm_result& a);
};

// BLOCK  TRX  OP  VOP
struct operation_printer
{
private:
   ostream& out;
   const wallet_api_impl& wallet;
   operation_result result;

   std::string fee(const asset& a) const;

public:
   operation_printer( ostream& out, const wallet_api_impl& wallet, const operation_result& r = operation_result() )
      : out(out),
        wallet(wallet),
        result(r)
   {}
   typedef std::string result_type;

   template<typename T>
   std::string operator()(const T& op)const;

   std::string operator()(const transfer_operation& op)const;
   std::string operator()(const override_transfer_operation& op)const;
   std::string operator()(const account_create_operation& op)const;
   std::string operator()(const asset_create_operation& op)const;
};

template<class T>
optional<T> maybe_id( const string& name_or_id )
{
   if( std::isdigit( name_or_id.front() ) )
   {
      try
      {
         return fc::variant(name_or_id).as<T>( 1 );
      }
      catch (const fc::exception&)
      { // not an ID
      }
   }
   return optional<T>();
}

fc::ecc::private_key derive_private_key( const std::string& prefix_string,
                                         int sequence_number )
{
   std::string sequence_string = std::to_string(sequence_number);
   fc::sha512 h = fc::sha512::hash(prefix_string + " " + sequence_string);
   fc::ecc::private_key derived_key = fc::ecc::private_key::regenerate(fc::sha256::hash(h));
   return derived_key;
}

string normalize_brain_key( string s )
{
   size_t i = 0, n = s.length();
   std::string result;
   char c;
   result.reserve( n );

   bool preceded_by_whitespace = false;
   bool non_empty = false;
   while( i < n )
   {
      c = s[i++];
      switch( c )
      {
      case ' ':  case '\t': case '\r': case '\n': case '\v': case '\f':
         preceded_by_whitespace = true;
         continue;

      case 'a': c = 'A'; break;
      case 'b': c = 'B'; break;
      case 'c': c = 'C'; break;
      case 'd': c = 'D'; break;
      case 'e': c = 'E'; break;
      case 'f': c = 'F'; break;
      case 'g': c = 'G'; break;
      case 'h': c = 'H'; break;
      case 'i': c = 'I'; break;
      case 'j': c = 'J'; break;
      case 'k': c = 'K'; break;
      case 'l': c = 'L'; break;
      case 'm': c = 'M'; break;
      case 'n': c = 'N'; break;
      case 'o': c = 'O'; break;
      case 'p': c = 'P'; break;
      case 'q': c = 'Q'; break;
      case 'r': c = 'R'; break;
      case 's': c = 'S'; break;
      case 't': c = 'T'; break;
      case 'u': c = 'U'; break;
      case 'v': c = 'V'; break;
      case 'w': c = 'W'; break;
      case 'x': c = 'X'; break;
      case 'y': c = 'Y'; break;
      case 'z': c = 'Z'; break;

      default:
         break;
      }
      if( preceded_by_whitespace && non_empty )
         result.push_back(' ');
      result.push_back(c);
      preceded_by_whitespace = false;
      non_empty = true;
   }
   return result;
}

struct op_prototype_visitor
{
   typedef void result_type;

   int t = 0;
   flat_map< std::string, operation >& name2op;

   op_prototype_visitor(
      int _t,
      flat_map< std::string, operation >& _prototype_ops
      ):t(_t), name2op(_prototype_ops) {}

   template<typename Type>
   result_type operator()( const Type& op )const
   {
      string name = fc::get_typename<Type>::name();
      size_t p = name.rfind(':');
      if( p != string::npos )
         name = name.substr( p+1 );
      name2op[ name ] = Type();
   }
};

class wallet_api_impl
{
public:
   api_documentation method_documentation;
private:
   void claim_registered_account(const string& name)
   {
      auto it = _wallet.pending_account_registrations.find( name );
      FC_ASSERT( it != _wallet.pending_account_registrations.end() );
      for (const std::string& wif_key : it->second)
         if( !import_key( name, wif_key ) )
         {
            // somebody else beat our pending registration, there is
            //    nothing we can do except log it and move on
            elog( "account ${name} registered by someone else first!",
                  ("name", name) );
            // might as well remove it from pending regs,
            //    because there is now no way this registration
            //    can become valid (even in the extremely rare
            //    possibility of migrating to a fork where the
            //    name is available, the user can always
            //    manually re-register)
         }
      _wallet.pending_account_registrations.erase( it );
   }

   // after a witness registration succeeds, this saves the private key in the wallet permanently
   //
   void claim_registered_witness(const std::string& witness_name)
   {
      auto iter = _wallet.pending_witness_registrations.find(witness_name);
      FC_ASSERT(iter != _wallet.pending_witness_registrations.end());
      std::string wif_key = iter->second;

      // get the list key id this key is registered with in the chain
      fc::optional<fc::ecc::private_key> witness_private_key = wif_to_key(wif_key);
      FC_ASSERT(witness_private_key);

      auto pub_key = witness_private_key->get_public_key();
      _keys[pub_key] = wif_key;
      _wallet.pending_witness_registrations.erase(iter);
   }

   fc::mutex _resync_mutex;
   void resync()
   {
      fc::scoped_lock<fc::mutex> lock(_resync_mutex);
      // this method is used to update wallet_data annotations
      //   e.g. wallet has been restarted and was not notified
      //   of events while it was down
      //
      // everything that is done "incremental style" when a push
      //   notification is received, should also be done here
      //   "batch style" by querying the blockchain

      if( !_wallet.pending_account_registrations.empty() )
      {
         // make a vector of the account names pending registration
         std::vector<string> pending_account_names = boost::copy_range<std::vector<string> >(boost::adaptors::keys(_wallet.pending_account_registrations));

         for(string& name : pending_account_names ) {
            std::map<std::string,account_uid_type> n = _remote_db->lookup_accounts_by_name( name, 1 );
            map<std::string, account_uid_type>::iterator it = n.find(name);
            if( it != n.end() ) {
               claim_registered_account(name);
            }
         }
      }

      if (!_wallet.pending_witness_registrations.empty())
      {
         // make a vector of the owner accounts for witnesses pending registration
         std::vector<string> pending_witness_names = boost::copy_range<std::vector<string> >(boost::adaptors::keys(_wallet.pending_witness_registrations));

         for(string& name : pending_witness_names ) {
            std::map<std::string,account_uid_type> w = _remote_db->lookup_accounts_by_name( name, 1 );
            map<std::string, account_uid_type>::iterator it = w.find(name);
            if( it != w.end() ) {
               fc::optional<witness_object> witness_obj = _remote_db->get_witness_by_account(it->second);
               if (witness_obj)
                  claim_registered_witness(name);
            }
         }
      }
   }

   void enable_umask_protection()
   {
#ifdef __unix__
      _old_umask = umask( S_IRWXG | S_IRWXO );
#endif
   }

   void disable_umask_protection()
   {
#ifdef __unix__
      umask( _old_umask );
#endif
   }

   void init_prototype_ops()
   {
      operation op;
      for( int t=0; t<op.count(); t++ )
      {
         op.set_which( t );
         op.visit( op_prototype_visitor(t, _prototype_ops) );
      }
      return;
   }

   map<transaction_handle_type, signed_transaction> _builder_transactions;

   // if the user executes the same command twice in quick succession,
   // we might generate the same transaction id, and cause the second
   // transaction to be rejected.  This can be avoided by altering the
   // second transaction slightly (bumping up the expiration time by
   // a second).  Keep track of recent transaction ids we've generated
   // so we can know if we need to do this
   struct recently_generated_transaction_record
   {
      fc::time_point_sec generation_time;
      graphene::chain::transaction_id_type transaction_id;
   };
   struct timestamp_index{};
   typedef boost::multi_index_container<recently_generated_transaction_record,
                                        boost::multi_index::indexed_by<boost::multi_index::hashed_unique<boost::multi_index::member<recently_generated_transaction_record,
                                                                                                                                    graphene::chain::transaction_id_type,
                                                                                                                                    &recently_generated_transaction_record::transaction_id>,
                                                                                                         std::hash<graphene::chain::transaction_id_type> >,
                                                                       boost::multi_index::ordered_non_unique<boost::multi_index::tag<timestamp_index>,
                                                                                                              boost::multi_index::member<recently_generated_transaction_record, fc::time_point_sec, &recently_generated_transaction_record::generation_time> > > > recently_generated_transaction_set_type;
   recently_generated_transaction_set_type _recently_generated_transactions;

public:
   wallet_api& self;
   wallet_api_impl( wallet_api& s, const wallet_data& initial_data, fc::api<login_api> rapi )
      : self(s),
        _chain_id(initial_data.chain_id),
        _remote_api(rapi),
        _remote_db(rapi->database()),
        _remote_net_broadcast(rapi->network_broadcast()),
        _remote_hist(rapi->history())
   {
      chain_id_type remote_chain_id = _remote_db->get_chain_id();
      if( remote_chain_id != _chain_id )
      {
         FC_THROW( "Remote server gave us an unexpected chain_id",
            ("remote_chain_id", remote_chain_id)
            ("chain_id", _chain_id) );
      }
      init_prototype_ops();

      _remote_db->set_block_applied_callback( [this](const variant& block_id )
      {
         on_block_applied( block_id );
      } );

      _wallet.chain_id = _chain_id;
      _wallet.ws_server = initial_data.ws_server;
      _wallet.ws_user = initial_data.ws_user;
      _wallet.ws_password = initial_data.ws_password;
   }
   virtual ~wallet_api_impl()
   {
      try
      {
         _remote_db->cancel_all_subscriptions();
      }
      catch (const fc::exception& e)
      {
         // Right now the wallet_api has no way of knowing if the connection to the
         // node has already disconnected (via the node exiting first).
         // If it has exited, cancel_all_subscriptsions() will throw and there's
         // nothing we can do about it.
         // dlog("Caught exception ${e} while canceling database subscriptions", ("e", e));
      }
   }

   void encrypt_keys()
   {
      if( !is_locked() )
      {
         plain_keys data;
         data.keys = _keys;
         data.checksum = _checksum;
         auto plain_txt = fc::raw::pack(data);
         _wallet.cipher_keys = fc::aes_encrypt( data.checksum, plain_txt );
      }
   }

   void on_block_applied( const variant& block_id )
   {
      fc::async([this]{resync();}, "Resync after block");
   }

   bool copy_wallet_file( string destination_filename )
   {
      fc::path src_path = get_wallet_filename();
      if( !fc::exists( src_path ) )
         return false;
      fc::path dest_path = destination_filename + _wallet_filename_extension;
      int suffix = 0;
      while( fc::exists(dest_path) )
      {
         ++suffix;
         dest_path = destination_filename + "-" + to_string( suffix ) + _wallet_filename_extension;
      }
      wlog( "backing up wallet ${src} to ${dest}",
            ("src", src_path)
            ("dest", dest_path) );

      fc::path dest_parent = fc::absolute(dest_path).parent_path();
      try
      {
         enable_umask_protection();
         if( !fc::exists( dest_parent ) )
            fc::create_directories( dest_parent );
         fc::copy( src_path, dest_path );
         disable_umask_protection();
      }
      catch(...)
      {
         disable_umask_protection();
         throw;
      }
      return true;
   }

   bool is_locked()const
   {
      return _checksum == fc::sha512();
   }

   template<typename T>
   T get_object(object_id<T::space_id, T::type_id, T> id)const
   {
      auto ob = _remote_db->get_objects({id}).front();
      return ob.template as<T>( GRAPHENE_MAX_NESTED_OBJECTS );
   }

   void set_operation_fees( signed_transaction& tx, const fee_schedule& s, bool csaf_fee )
   {
      if (csaf_fee) {
         for (auto& op : tx.operations)
            s.set_fee_with_csaf(op);
      } else {
         for (auto& op : tx.operations)
            s.set_fee(op);
      }   
   }

   variant info() const
   {
      auto chain_props = get_chain_properties();
      auto global_props = get_global_properties();
      auto dynamic_props = get_dynamic_global_properties();
      fc::mutable_variant_object result;
      result["head_block_num"] = dynamic_props.head_block_number;
      result["head_block_id"] = fc::variant( dynamic_props.head_block_id, 1 );
      result["head_block_time"] = dynamic_props.time;
      result["head_block_age"] = fc::get_approximate_relative_time_string(dynamic_props.time,
                                                                          time_point_sec(time_point::now()),
                                                                          " old");
      //result["next_maintenance_time"] = fc::get_approximate_relative_time_string(dynamic_props.next_maintenance_time);
      result["last_irreversible_block_num"] = dynamic_props.last_irreversible_block_num;
      result["chain_id"] = chain_props.chain_id;
      result["participation"] = (100*dynamic_props.recent_slots_filled.popcount()) / 128.0;
      result["active_witnesses"] = fc::variant( global_props.active_witnesses, GRAPHENE_MAX_NESTED_OBJECTS );
      result["active_committee_members"] = fc::variant( global_props.active_committee_members, GRAPHENE_MAX_NESTED_OBJECTS );
      return result;
   }

   variant_object about() const
   {
      string client_version( graphene::utilities::git_revision_description );
      const size_t pos = client_version.find( '/' );
      if( pos != string::npos && client_version.size() > pos )
         client_version = client_version.substr( pos + 1 );

      fc::mutable_variant_object result;
      //result["blockchain_name"]        = BLOCKCHAIN_NAME;
      //result["blockchain_description"] = BTS_BLOCKCHAIN_DESCRIPTION;
      result["client_version"]           = client_version;
      result["graphene_revision"]        = graphene::utilities::git_revision_sha;
      result["graphene_revision_age"]    = fc::get_approximate_relative_time_string( fc::time_point_sec( graphene::utilities::git_revision_unix_timestamp ) );
      result["fc_revision"]              = fc::git_revision_sha;
      result["fc_revision_age"]          = fc::get_approximate_relative_time_string( fc::time_point_sec( fc::git_revision_unix_timestamp ) );
      result["compile_date"]             = "compiled on " __DATE__ " at " __TIME__;
      result["boost_version"]            = boost::replace_all_copy(std::string(BOOST_LIB_VERSION), "_", ".");
      result["openssl_version"]          = OPENSSL_VERSION_TEXT;

      std::string bitness = boost::lexical_cast<std::string>(8 * sizeof(int*)) + "-bit";
#if defined(__APPLE__)
      std::string os = "osx";
#elif defined(__linux__)
      std::string os = "linux";
#elif defined(_MSC_VER)
      std::string os = "win32";
#else
      std::string os = "other";
#endif
      result["build"] = os + " " + bitness;

      return result;
   }

   chain_property_object get_chain_properties() const
   {
      return _remote_db->get_chain_properties();
   }
   global_property_object get_global_properties() const
   {
      return _remote_db->get_global_properties();
   }
   content_parameter_extension_type get_global_properties_extensions() const
   {
      return _remote_db->get_global_properties().parameters.get_award_params();
   }
   dynamic_global_property_object get_dynamic_global_properties() const
   {
      return _remote_db->get_dynamic_global_properties();
   }
   account_object get_account(account_uid_type uid) const
   {
      /*
      // TODO review. commented out to get around the caching issue
      const auto& idx = _wallet.my_accounts.get<by_uid>();
      auto itr = idx.find(uid);
      if( itr != idx.end() )
         return *itr;
      */
      auto rec = _remote_db->get_accounts_by_uid({uid}).front();
      FC_ASSERT( rec, "Can not find account ${uid}.", ("uid",uid) );
      return *rec;
   }
   account_object get_account(string account_name_or_id) const
   {
      FC_ASSERT( account_name_or_id.size() > 0 );

      if( graphene::utilities::is_number( account_name_or_id ) )
      {
         // It's a UID
         return get_account( fc::variant( account_name_or_id ).as<account_uid_type>( 1 ) );
      } else {
         // It's a name
         /*
         // TODO review. commented out to get around the caching issue
         if( _wallet.my_accounts.get<by_name>().count(account_name_or_id) )
         {
            auto local_account = *_wallet.my_accounts.get<by_name>().find(account_name_or_id);
            auto blockchain_account = _remote_db->lookup_account_names({account_name_or_id}).front();
            FC_ASSERT( blockchain_account );
            if (local_account.uid != blockchain_account->uid)
               elog("my account uid ${uid} different from blockchain uid ${uid2}", ("uid", local_account.uid)("uid2", blockchain_account->uid));
            if (local_account.name != blockchain_account->name)
               elog("my account name ${name} different from blockchain name ${name2}", ("name", local_account.name)("name2", blockchain_account->name));

            return *_wallet.my_accounts.get<by_name>().find(account_name_or_id);
         }
         */
         optional<account_object> rec = _remote_db->get_account_by_name( account_name_or_id );
         FC_ASSERT( rec && rec->name == account_name_or_id, "Can not find account ${a}.", ("a",account_name_or_id) );
         return *rec;
      }
   }
   account_uid_type get_account_uid(string account_name_or_id) const
   {
      return get_account(account_name_or_id).get_uid();
   }
   account_id_type get_account_id(string account_name_or_id) const
   {
      return get_account(account_name_or_id).get_id();
   }

   optional<asset_object_with_data> find_asset(asset_aid_type aid)const
   {
      auto rec = _remote_db->get_assets({aid}).front();
      return rec;
   }
   optional<asset_object_with_data> find_asset(string asset_symbol_or_id)const
   {
      FC_ASSERT( asset_symbol_or_id.size() > 0 );
      if(graphene::utilities::is_number(asset_symbol_or_id))
      {
         asset_aid_type id = fc::variant( asset_symbol_or_id ).as_uint64();
         return find_asset(id);
      }
      else if(auto id = maybe_id<asset_id_type>(asset_symbol_or_id))
      {
         return get_object(*id);
      }
      else
      {
         auto rec = _remote_db->lookup_asset_symbols({asset_symbol_or_id}).front();
         if( rec )
         {
            if( rec->symbol != asset_symbol_or_id )
               return optional<asset_object>();
         }
         return rec;
      }
   }

   asset_object_with_data get_asset(asset_aid_type aid)const
   {
      auto opt = find_asset(aid);
      FC_ASSERT( opt, "Can not find asset ${a}", ("a", aid) );
      return *opt;
   }

   asset_object_with_data get_asset(string asset_symbol_or_id)const
   {
      auto opt = find_asset(asset_symbol_or_id);
      FC_ASSERT( opt, "Can not find asset ${a}", ("a", asset_symbol_or_id) );
      return *opt;
   }

   asset_aid_type get_asset_aid(string asset_symbol_or_id) const
   {
      FC_ASSERT( asset_symbol_or_id.size() > 0 );
      auto opt_asset = find_asset( asset_symbol_or_id );
      FC_ASSERT( opt_asset.valid(), "Can not find asset ${a}", ("a", asset_symbol_or_id) );
      return opt_asset->asset_id;
   }

   string                            get_wallet_filename() const
   {
      return _wallet_filename;
   }

   fc::ecc::private_key              get_private_key(const public_key_type& id)const
   {
      FC_ASSERT( !self.is_locked(), "The wallet must be unlocked to get the private key" );
      auto it = _keys.find(id);
      FC_ASSERT( it != _keys.end(), "Can not find private key of ${pub} in the wallet", ("pub",id) );

      fc::optional< fc::ecc::private_key > privkey = wif_to_key( it->second );
      FC_ASSERT( privkey, "Can not find private key of ${pub} in the wallet", ("pub",id) );
      return *privkey;
   }

   fc::ecc::private_key get_private_key_for_account(const account_object& account)const
   {
      vector<public_key_type> active_keys = account.active.get_keys();
      if (active_keys.size() != 1)
         FC_THROW("Expecting a simple authority with one active key");
      return get_private_key(active_keys.front());
   }

   // imports the private key into the wallet, and associate it in some way (?) with the
   // given account name.
   // @returns true if the key matches a current active/owner/memo key for the named
   //          account, false otherwise (but it is stored either way)
   bool import_key(string account_name_or_id, string wif_key)
   {
      fc::optional<fc::ecc::private_key> optional_private_key = wif_to_key(wif_key);
      if (!optional_private_key)
         FC_THROW("Invalid private key");
      graphene::chain::public_key_type wif_pub_key = optional_private_key->get_public_key();

      account_object account = get_account( account_name_or_id );

      // make a list of all current public keys for the named account
      flat_set<public_key_type> all_keys_for_account;
      std::vector<public_key_type> secondary_keys = account.secondary.get_keys();
      std::vector<public_key_type> active_keys = account.active.get_keys();
      std::vector<public_key_type> owner_keys = account.owner.get_keys();
      std::copy(secondary_keys.begin(), secondary_keys.end(), std::inserter(all_keys_for_account, all_keys_for_account.end()));
      std::copy(active_keys.begin(), active_keys.end(), std::inserter(all_keys_for_account, all_keys_for_account.end()));
      std::copy(owner_keys.begin(), owner_keys.end(), std::inserter(all_keys_for_account, all_keys_for_account.end()));
      all_keys_for_account.insert(account.memo_key);

      _keys[wif_pub_key] = wif_key;

      _wallet.update_account(account);

      _wallet.extra_keys[account.uid].insert(wif_pub_key);

      return all_keys_for_account.find(wif_pub_key) != all_keys_for_account.end();
   }

   bool load_wallet_file(string wallet_filename = "")
   {
      if( !self.is_locked() )
         self.lock();

      // TODO:  Merge imported wallet with existing wallet,
      //        instead of replacing it
      if( wallet_filename == "" )
         wallet_filename = _wallet_filename;

      if( ! fc::exists( wallet_filename ) )
         return false;

      _wallet = fc::json::from_file( wallet_filename ).as< wallet_data >( 2 * GRAPHENE_MAX_NESTED_OBJECTS );
      if( _wallet.chain_id != _chain_id )
         FC_THROW( "Wallet chain ID does not match",
            ("wallet.chain_id", _wallet.chain_id)
            ("chain_id", _chain_id) );

      size_t account_pagination = 100;
      vector< account_uid_type > account_uids_to_send;
      size_t n = _wallet.my_accounts.size();
      account_uids_to_send.reserve( std::min( account_pagination, n ) );
      auto it = _wallet.my_accounts.begin();

      for( size_t start=0; start<n; start+=account_pagination )
      {
         size_t end = std::min( start+account_pagination, n );
         assert( end > start );
         account_uids_to_send.clear();
         std::vector< account_object > old_accounts;
         for( size_t i=start; i<end; i++ )
         {
            assert( it != _wallet.my_accounts.end() );
            old_accounts.push_back( *it );
            account_uids_to_send.push_back( old_accounts.back().uid );
            ++it;
         }
         std::vector< optional< account_object > > accounts = _remote_db->get_accounts_by_uid(account_uids_to_send);
         // server response should be same length as request
         FC_ASSERT( accounts.size() == account_uids_to_send.size(), "remote server error" );
         size_t i = 0;
         for( const optional< account_object >& acct : accounts )
         {
            account_object& old_acct = old_accounts[i];
            if( !acct.valid() )
            {
               elog( "Could not find account ${uid} : \"${name}\" does not exist on the chain!", ("uid", old_acct.uid)("name", old_acct.name) );
               i++;
               continue;
            }
            // this check makes sure the server didn't send results
            // in a different order, or accounts we didn't request
            FC_ASSERT( acct->uid == old_acct.uid, "remote server error" );
            if( fc::json::to_string(*acct) != fc::json::to_string(old_acct) )
            {
               wlog( "Account ${uid} : \"${name}\" updated on chain", ("uid", acct->uid)("name", acct->name) );
            }
            _wallet.update_account( *acct );
            i++;
         }
      }

      return true;
   }
   void save_wallet_file(string wallet_filename = "")
   {
      //
      // Serialize in memory, then save to disk
      //
      // This approach lessens the risk of a partially written wallet
      // if exceptions are thrown in serialization
      //

      encrypt_keys();

      if( wallet_filename == "" )
         wallet_filename = _wallet_filename;

      wlog( "saving wallet to file ${fn}", ("fn", wallet_filename) );

      string data = fc::json::to_pretty_string( _wallet );
      try
      {
         enable_umask_protection();
         //
         // Parentheses on the following declaration fails to compile,
         // due to the Most Vexing Parse.  Thanks, C++
         //
         // http://en.wikipedia.org/wiki/Most_vexing_parse
         //
         fc::ofstream outfile{ fc::path( wallet_filename ) };
         outfile.write( data.c_str(), data.length() );
         outfile.flush();
         outfile.close();
         disable_umask_protection();
      }
      catch(...)
      {
         disable_umask_protection();
         throw;
      }
   }

   transaction_handle_type begin_builder_transaction()
   {
      int trx_handle = _builder_transactions.empty()? 0
                                                    : (--_builder_transactions.end())->first + 1;
      _builder_transactions[trx_handle];
      return trx_handle;
   }
   void add_operation_to_builder_transaction(transaction_handle_type transaction_handle, const operation& op)
   {
      FC_ASSERT(_builder_transactions.count(transaction_handle));
      _builder_transactions[transaction_handle].operations.emplace_back(op);
   }
   void replace_operation_in_builder_transaction(transaction_handle_type handle,
                                                 uint32_t operation_index,
                                                 const operation& new_op)
   {
      FC_ASSERT(_builder_transactions.count(handle));
      signed_transaction& trx = _builder_transactions[handle];
      FC_ASSERT( operation_index < trx.operations.size());
      trx.operations[operation_index] = new_op;
   }
   asset set_fees_on_builder_transaction(transaction_handle_type handle, string fee_asset = GRAPHENE_SYMBOL)
   {
      FC_ASSERT(_builder_transactions.count(handle));

      auto fee_asset_obj = get_asset(fee_asset);
      asset total_fee = fee_asset_obj.amount(0);

      FC_ASSERT(fee_asset_obj.asset_id == GRAPHENE_CORE_ASSET_AID, "Must use core assets as a fee");


      auto gprops = _remote_db->get_global_properties().parameters;
      for( auto& op : _builder_transactions[handle].operations )
         total_fee += gprops.current_fees->set_fee( op );

      return total_fee;
   }
   transaction preview_builder_transaction(transaction_handle_type handle)
   {
      FC_ASSERT(_builder_transactions.count(handle));
      return _builder_transactions[handle];
   }
   signed_transaction sign_builder_transaction(transaction_handle_type transaction_handle, bool broadcast = true)
   {
      FC_ASSERT(_builder_transactions.count(transaction_handle));

      return _builder_transactions[transaction_handle] = sign_transaction(_builder_transactions[transaction_handle], broadcast);
   }

   signed_transaction propose_builder_transaction(
      transaction_handle_type handle,
      string account_name_or_id,
      time_point_sec expiration = time_point::now() + fc::minutes(1),
      uint32_t review_period_seconds = 0, bool broadcast = true)
   {
      FC_ASSERT(_builder_transactions.count(handle));
      proposal_create_operation op;
      op.fee_paying_account = get_account(account_name_or_id).get_uid();
      op.expiration_time = expiration;
      signed_transaction& trx = _builder_transactions[handle];
      std::transform(trx.operations.begin(), trx.operations.end(), std::back_inserter(op.proposed_ops),
                     [](const operation& op) -> op_wrapper { return op; });
      if( review_period_seconds )
         op.review_period_seconds = review_period_seconds;
      trx.operations = {op};
      _remote_db->get_global_properties().parameters.current_fees->set_fee( trx.operations.front() );

      return trx = sign_transaction(trx, broadcast);
   }

   void remove_builder_transaction(transaction_handle_type handle)
   {
      _builder_transactions.erase(handle);
   }


   signed_transaction register_account(string name,
                                       public_key_type owner,
                                       public_key_type active,
                                       string  registrar_account,
                                       string  referrer_account,
                                       uint32_t referrer_percent,
                                       uint32_t seed,
                                       bool csaf_fee = true,
                                       bool broadcast = false)
   { try {
      FC_ASSERT( !self.is_locked() );
      account_create_operation account_create_op;

      // #449 referrer_percent is on 0-100 scale, if user has larger
      // number it means their script is using GRAPHENE_100_PERCENT scale
      // instead of 0-100 scale.
      FC_ASSERT( referrer_percent <= 100 );
      // TODO:  process when pay_from_account is ID

      account_object registrar_account_object =
            this->get_account( registrar_account );
      //FC_ASSERT( registrar_account_object.is_lifetime_member() );

      account_object referrer_account_object =
         this->get_account(referrer_account);

      // TODO review
      /*
      account_id_type registrar_account_id = registrar_account_object.id;

      account_object referrer_account_object =
            this->get_account( referrer_account );
      account_create_op.referrer = referrer_account_object.id;
      account_create_op.referrer_percent = uint16_t( referrer_percent * GRAPHENE_1_PERCENT );

      account_create_op.registrar = registrar_account_id;
      */
      account_create_op.name = name;
      account_create_op.owner = authority(1, owner, 1);
      account_create_op.active = authority(1, active, 1);
      account_create_op.secondary = authority(1, owner, 1);
      account_create_op.memo_key = active;
      account_create_op.uid = graphene::chain::calc_account_uid(seed);
      account_reg_info reg_info;
      reg_info.registrar = registrar_account_object.uid;
      reg_info.referrer = referrer_account_object.uid;
      account_create_op.reg_info = reg_info;

      signed_transaction tx;

      tx.operations.push_back( account_create_op );

      auto current_fees = _remote_db->get_global_properties().parameters.current_fees;
      set_operation_fees( tx, current_fees, csaf_fee );

      vector<public_key_type> paying_keys = registrar_account_object.active.get_keys();

      auto dyn_props = get_dynamic_global_properties();
      tx.set_reference_block( dyn_props.head_block_id );
      tx.set_expiration( dyn_props.time + fc::seconds(30) );
      tx.validate();

      for( public_key_type& key : paying_keys )
      {
         auto it = _keys.find(key);
         if( it != _keys.end() )
         {
            fc::optional< fc::ecc::private_key > privkey = wif_to_key( it->second );
            if( !privkey.valid() )
            {
               FC_ASSERT( false, "Malformed private key in _keys" );
            }
            tx.sign( *privkey, _chain_id );
         }
      }

      if( broadcast )
         _remote_net_broadcast->broadcast_transaction( tx );
      return tx;
   } FC_CAPTURE_AND_RETHROW( (name)(owner)(active)(registrar_account)(referrer_account)(referrer_percent)(csaf_fee)(broadcast) ) }

   // This function generates derived keys starting with index 0 and keeps incrementing
   // the index until it finds a key that isn't registered in the block chain.  To be
   // safer, it continues checking for a few more keys to make sure there wasn't a short gap
   // caused by a failed registration or the like.
   int find_first_unused_derived_key_index(const fc::ecc::private_key& parent_key)
   {
      int first_unused_index = 0;
      int number_of_consecutive_unused_keys = 0;
      for (int key_index = 0; ; ++key_index)
      {
         fc::ecc::private_key derived_private_key = derive_private_key(key_to_wif(parent_key), key_index);
         graphene::chain::public_key_type derived_public_key = derived_private_key.get_public_key();
         if( _keys.find(derived_public_key) == _keys.end() )
         {
            if (number_of_consecutive_unused_keys)
            {
               ++number_of_consecutive_unused_keys;
               if (number_of_consecutive_unused_keys > 5)
                  return first_unused_index;
            }
            else
            {
               first_unused_index = key_index;
               number_of_consecutive_unused_keys = 1;
            }
         }
         else
         {
            // key_index is used
            first_unused_index = 0;
            number_of_consecutive_unused_keys = 0;
         }
      }
   }

   signed_transaction create_account_with_private_key(fc::ecc::private_key owner_privkey,
                                                      string account_name,
                                                      string registrar_account,
                                                      string referrer_account,
                                                      uint32_t seed,
                                                      bool csaf_fee = true,
                                                      bool broadcast = false,
                                                      bool save_wallet = true)
   { try {
         int active_key_index = find_first_unused_derived_key_index(owner_privkey);
         fc::ecc::private_key active_privkey = derive_private_key( key_to_wif(owner_privkey), active_key_index);

         int memo_key_index = find_first_unused_derived_key_index(active_privkey);
         fc::ecc::private_key memo_privkey = derive_private_key( key_to_wif(active_privkey), memo_key_index);

         graphene::chain::public_key_type owner_pubkey = owner_privkey.get_public_key();
         graphene::chain::public_key_type active_pubkey = active_privkey.get_public_key();
         graphene::chain::public_key_type memo_pubkey = memo_privkey.get_public_key();

         account_create_operation account_create_op;

         // TODO:  process when pay_from_account is ID

         account_object registrar_account_object = get_account( registrar_account );
         account_object referrer_account_object = get_account(referrer_account);

         // TODO: review
         /*
         account_id_type registrar_account_id = registrar_account_object.id;

         account_object referrer_account_object = get_account( referrer_account );
         account_create_op.referrer = referrer_account_object.id;
         account_create_op.referrer_percent = referrer_account_object.referrer_rewards_percentage;

         account_create_op.registrar = registrar_account_id;
         */
         account_create_op.name = account_name;
         account_create_op.owner = authority(1, owner_pubkey, 1);
         account_create_op.active = authority(1, active_pubkey, 1);
         account_create_op.secondary = authority(1, owner_pubkey, 1);
         account_create_op.uid = graphene::chain::calc_account_uid(seed);
         account_reg_info reg_info;
         reg_info.registrar = registrar_account_object.uid;
         reg_info.referrer = referrer_account_object.uid;
         account_create_op.reg_info = reg_info;
         account_create_op.memo_key = memo_pubkey;

         // current_fee_schedule()
         // find_account(pay_from_account)

         // account_create_op.fee = account_create_op.calculate_fee(db.current_fee_schedule());

         signed_transaction tx;

         tx.operations.push_back( account_create_op );

         set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);

         vector<public_key_type> paying_keys = registrar_account_object.active.get_keys();

         auto dyn_props = get_dynamic_global_properties();
         tx.set_reference_block( dyn_props.head_block_id );
         tx.set_expiration( dyn_props.time + fc::seconds(30) );
         tx.validate();

         for( public_key_type& key : paying_keys )
         {
            auto it = _keys.find(key);
            if( it != _keys.end() )
            {
               fc::optional< fc::ecc::private_key > privkey = wif_to_key( it->second );
               FC_ASSERT( privkey.valid(), "Malformed private key in _keys" );
               tx.sign( *privkey, _chain_id );
            }
         }

         // we do not insert owner_privkey here because
         //    it is intended to only be used for key recovery
         _wallet.pending_account_registrations[account_name].push_back(key_to_wif( active_privkey ));
         _wallet.pending_account_registrations[account_name].push_back(key_to_wif( memo_privkey ));
         if( save_wallet )
            save_wallet_file();
         if( broadcast )
            _remote_net_broadcast->broadcast_transaction( tx );
         return tx;
   } FC_CAPTURE_AND_RETHROW( (account_name)(registrar_account)(referrer_account)(csaf_fee)(broadcast) ) }

   signed_transaction create_account_with_brain_key(string brain_key,
                                                    string account_name,
                                                    string registrar_account,
                                                    string referrer_account,
                                                    uint32_t seed,
                                                    bool csaf_fee = true,
                                                    bool broadcast = false,
                                                    bool save_wallet = true)
   { try {
      FC_ASSERT( !self.is_locked() );
      string normalized_brain_key = normalize_brain_key( brain_key );
      // TODO:  scan blockchain for accounts that exist with same brain key
      fc::ecc::private_key owner_privkey = derive_private_key( normalized_brain_key, 0 );
      return create_account_with_private_key(owner_privkey, account_name, registrar_account, referrer_account, seed, csaf_fee, broadcast, save_wallet);
   } FC_CAPTURE_AND_RETHROW( (account_name)(registrar_account)(referrer_account) ) }


   signed_transaction create_asset(string issuer,
                                   string symbol,
                                   uint8_t precision,
                                   asset_options common,
                                   share_type initial_supply,
                                   bool csaf_fee,
                                   bool broadcast = false)
   { try {
      account_object issuer_account = get_account( issuer );
      FC_ASSERT(!find_asset(symbol).valid(), "Asset with that symbol already exists!");

      asset_create_operation create_op;
      create_op.issuer = issuer_account.uid;
      create_op.symbol = symbol;
      create_op.precision = precision;
      create_op.common_options = common;

      if( initial_supply != 0 )
      {
         create_op.extensions = extension<asset_create_operation::ext>();
         create_op.extensions->value.initial_supply = initial_supply;
      }

      signed_transaction tx;
      tx.operations.push_back( create_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (issuer)(symbol)(precision)(common)(csaf_fee)(broadcast) ) }

   signed_transaction update_asset(string symbol,
                                   optional<uint8_t> new_precision,
                                   asset_options new_options,
                                   bool csaf_fee,
                                   bool broadcast /* = false */)
   { try {
      optional<asset_object_with_data> asset_to_update = find_asset(symbol);
      FC_ASSERT( asset_to_update.valid(), "Can not find asset ${a}", ("a", symbol) );

      asset_update_operation update_op;
      update_op.issuer = asset_to_update->issuer;
      update_op.asset_to_update = asset_to_update->asset_id;
      update_op.new_precision = new_precision;
      update_op.new_options = new_options;

      signed_transaction tx;
      tx.operations.push_back( update_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (symbol)(new_precision)(new_options)(csaf_fee)(broadcast) ) }

   signed_transaction reserve_asset(string from,
                                 string amount,
                                 string symbol,
                                 bool csaf_fee,
                                 bool broadcast /* = false */)
   { try {
      account_object from_account = get_account(from);
      optional<asset_object_with_data> asset_to_reserve = find_asset(symbol);
      FC_ASSERT( asset_to_reserve.valid(), "Can not find asset ${a}", ("a", symbol) );

      asset_reserve_operation reserve_op;
      reserve_op.payer = from_account.uid;
      reserve_op.amount_to_reserve = asset_to_reserve->amount_from_string(amount);

      signed_transaction tx;
      tx.operations.push_back( reserve_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (from)(amount)(symbol)(csaf_fee)(broadcast) ) }

   signed_transaction whitelist_account(string authorizing_account,
                                        string account_to_list,
                                        account_whitelist_operation::account_listing new_listing_status,
                                        bool csaf_fee,
                                        bool broadcast /* = false */)
   { try {
      account_whitelist_operation whitelist_op;
      whitelist_op.authorizing_account = get_account_uid(authorizing_account);
      whitelist_op.account_to_list = get_account_uid(account_to_list);
      whitelist_op.new_listing = new_listing_status;

      signed_transaction tx;
      tx.operations.push_back( whitelist_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (authorizing_account)(account_to_list)(new_listing_status)(csaf_fee)(broadcast) ) }

   signed_transaction create_committee_member(string owner_account,
                                              string pledge_amount,
                                              string pledge_asset_symbol,
                                              string url,
                                              bool csaf_fee,
                                              bool broadcast /* = false */)
   { try {
      account_object committee_member_account = get_account(owner_account);

      if( _remote_db->get_committee_member_by_account( committee_member_account.uid ) )
         FC_THROW( "Account ${owner_account} is already a committee_member", ("owner_account", owner_account) );

      fc::optional<asset_object_with_data> asset_obj = get_asset( pledge_asset_symbol );
      FC_ASSERT( asset_obj, "Could not find asset matching ${asset}", ("asset", pledge_asset_symbol) );

      committee_member_create_operation committee_member_create_op;
      committee_member_create_op.account = committee_member_account.uid;
      committee_member_create_op.pledge = asset_obj->amount_from_string( pledge_amount );
      committee_member_create_op.url = url;

      signed_transaction tx;
      tx.operations.push_back( committee_member_create_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (owner_account)(pledge_amount)(pledge_asset_symbol)(csaf_fee)(broadcast) ) }

   witness_object get_witness(string owner_account)
   {
      try
      {
         fc::optional<witness_id_type> witness_id = maybe_id<witness_id_type>(owner_account);
         if (witness_id)
         {
            std::vector<object_id_type> ids_to_get;
            ids_to_get.push_back(*witness_id);
            fc::variants objects = _remote_db->get_objects( ids_to_get );
            for( const variant& obj : objects )
            {
               optional<witness_object> wo;
               from_variant( obj, wo, GRAPHENE_MAX_NESTED_OBJECTS );
               if( wo )
                  return *wo;
            }
            FC_THROW("No witness is registered for id ${id}", ("id", owner_account));
         }
         else
         {
            // then maybe it's the owner account
            try
            {
               account_uid_type owner_account_uid = get_account_uid(owner_account);
               fc::optional<witness_object> witness = _remote_db->get_witness_by_account(owner_account_uid);
               if (witness)
                  return *witness;
               else
                  FC_THROW("No witness is registered for account ${account}", ("account", owner_account));
            }
            catch (const fc::exception&)
            {
               FC_THROW("No account or witness named ${account}", ("account", owner_account));
            }
         }
      }
      FC_CAPTURE_AND_RETHROW( (owner_account) )
   }

   platform_object get_platform(string owner_account)
   {
      try
      {
         fc::optional<platform_id_type> platform_id = maybe_id<platform_id_type>(owner_account);
         if (platform_id)
         {
            std::vector<object_id_type> ids_to_get;
            ids_to_get.push_back(*platform_id);
            fc::variants objects = _remote_db->get_objects( ids_to_get );
            for( const variant& obj : objects )
            {
               optional<platform_object> wo;
               from_variant( obj, wo, GRAPHENE_MAX_NESTED_OBJECTS );
               if( wo )
                  return *wo;
            }
            FC_THROW("No platform is registered for id ${id}", ("id", owner_account));
         }
         else
         {
            // then maybe it's the owner account
            try
            {
               account_uid_type owner_account_uid = get_account_uid(owner_account);
               fc::optional<platform_object> platform = _remote_db->get_platform_by_account(owner_account_uid);
               if (platform)
                  return *platform;
               else
                  FC_THROW("No platform is registered for account ${account}", ("account", owner_account));
            }
            catch (const fc::exception&)
            {
               FC_THROW("No account or platform named ${account}", ("account", owner_account));
            }
         }
      }
      FC_CAPTURE_AND_RETHROW( (owner_account) )
   }

   committee_member_object get_committee_member(string owner_account)
   {
      try
      {
         fc::optional<committee_member_id_type> committee_member_id = maybe_id<committee_member_id_type>(owner_account);
         if (committee_member_id)
         {
            std::vector<object_id_type> ids_to_get;
            ids_to_get.push_back(*committee_member_id);
            fc::variants objects = _remote_db->get_objects( ids_to_get );
            for( const variant& obj : objects )
            {
               optional<committee_member_object> wo;
               from_variant( obj, wo, GRAPHENE_MAX_NESTED_OBJECTS );
               if( wo )
                  return *wo;
            }
            FC_THROW("No committee_member is registered for id ${id}", ("id", owner_account));
         }
         else
         {
            // then maybe it's the owner account
            try
            {
               account_uid_type owner_account_uid = get_account_uid(owner_account);
               fc::optional<committee_member_object> committee_member = _remote_db->get_committee_member_by_account(owner_account_uid);
               if (committee_member)
                  return *committee_member;
               else
                  FC_THROW("No committee_member is registered for account ${account}", ("account", owner_account));
            }
            catch (const fc::exception&)
            {
               FC_THROW("No account or committee_member named ${account}", ("account", owner_account));
            }
         }
      }
      FC_CAPTURE_AND_RETHROW( (owner_account) )
   }

   signed_transaction create_witness_with_details(string owner_account,
                                                  public_key_type block_signing_key,
                                                  string pledge_amount,
                                                  string pledge_asset_symbol,
                                                  string url,
                                                  bool csaf_fee,
                                                  bool broadcast /* = false */)
   { try {
      account_object witness_account = get_account(owner_account);

      if( _remote_db->get_witness_by_account( witness_account.uid ) )
         FC_THROW( "Account ${owner_account} is already a witness", ("owner_account", owner_account) );

      fc::optional<asset_object_with_data> asset_obj = get_asset( pledge_asset_symbol );
      FC_ASSERT( asset_obj, "Could not find asset matching ${asset}", ("asset", pledge_asset_symbol) );

      witness_create_operation witness_create_op;
      witness_create_op.account = witness_account.uid;
      witness_create_op.block_signing_key = block_signing_key;
      witness_create_op.pledge = asset_obj->amount_from_string( pledge_amount );
      witness_create_op.url = url;

      signed_transaction tx;
      tx.operations.push_back( witness_create_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (owner_account)(block_signing_key)(pledge_amount)(pledge_asset_symbol)(csaf_fee)(broadcast) ) }

   signed_transaction create_witness(string owner_account,
                                     string url,
                                     bool csaf_fee,
                                     bool broadcast /* = false */)
   { try {
      account_object witness_account = get_account(owner_account);
      fc::ecc::private_key active_private_key = get_private_key_for_account(witness_account);
      int witness_key_index = find_first_unused_derived_key_index(active_private_key);
      fc::ecc::private_key witness_private_key = derive_private_key(key_to_wif(active_private_key), witness_key_index);
      graphene::chain::public_key_type witness_public_key = witness_private_key.get_public_key();

      witness_create_operation witness_create_op;
      witness_create_op.account = witness_account.uid;
      witness_create_op.block_signing_key = witness_public_key;
      witness_create_op.url = url;

      if (_remote_db->get_witness_by_account(witness_create_op.account))
         FC_THROW("Account ${owner_account} is already a witness", ("owner_account", owner_account));

      signed_transaction tx;
      tx.operations.push_back( witness_create_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
      tx.validate();

      _wallet.pending_witness_registrations[owner_account] = key_to_wif(witness_private_key);

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (owner_account)(csaf_fee)(broadcast) ) }

   signed_transaction create_platform(string owner_account,
                                        string name,
                                        string pledge_amount,
                                        string pledge_asset_symbol,
                                        string url,
                                        string extra_data,
                                        bool csaf_fee,
                                        bool broadcast )
{
   try {
      account_object platform_account = get_account( owner_account );

      if( _remote_db->get_platform_by_account( platform_account.uid ) )
         FC_THROW( "Account ${owner_account} is already a platform", ( "owner_account", owner_account ) );

      fc::optional<asset_object_with_data> asset_obj = get_asset( pledge_asset_symbol );
      FC_ASSERT( asset_obj, "Could not find asset matching ${asset}", ( "asset", pledge_asset_symbol ) );

      platform_create_operation platform_create_op;
      platform_create_op.account = platform_account.uid;
      platform_create_op.name = name;
      platform_create_op.pledge = asset_obj->amount_from_string( pledge_amount );
      platform_create_op.extra_data = extra_data;
      platform_create_op.url = url;

      signed_transaction tx;
      tx.operations.push_back( platform_create_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee );
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (owner_account)(name)(pledge_amount)(pledge_asset_symbol)(url)(extra_data)(csaf_fee)(broadcast) )
}

signed_transaction update_platform(string platform_account,
                                        optional<string> name,
                                        optional<string> pledge_amount,
                                        optional<string> pledge_asset_symbol,
                                        optional<string> url,
                                        optional<string> extra_data,
                                        bool csaf_fee,
                                        bool broadcast )

{
   try {
      FC_ASSERT( pledge_amount.valid() == pledge_asset_symbol.valid(),
                 "Pledge amount and asset symbol should be both set or both not set" );
      fc::optional<asset> pledge;
      if( pledge_amount.valid() )
      {
         fc::optional<asset_object_with_data> asset_obj = get_asset( *pledge_asset_symbol );
         FC_ASSERT( asset_obj, "Could not find asset matching ${asset}", ( "asset", *pledge_asset_symbol ) );
         pledge = asset_obj->amount_from_string( *pledge_amount );
      }

      platform_object platform = get_platform( platform_account );
      account_object platform_owner = get_account( platform.owner );

      platform_update_operation platform_update_op;
      platform_update_op.account = platform_owner.uid;
      platform_update_op.new_name = name;
      platform_update_op.new_pledge = pledge;
      platform_update_op.new_url = url;
      platform_update_op.new_extra_data = extra_data;

      signed_transaction tx;
      tx.operations.push_back( platform_update_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee );
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (platform_account)(name)(pledge_amount)(pledge_asset_symbol)(url)(extra_data)(csaf_fee)(broadcast) )
}

signed_transaction account_auth_platform(string account,
                                         string platform_owner,
                                         string memo,
                                         string limit_for_platform = 0,
                                         uint32_t permission_flags = account_auth_platform_object::Platform_Permission_Forward |
                                                                     account_auth_platform_object::Platform_Permission_Liked |
                                                                     account_auth_platform_object::Platform_Permission_Buyout |
                                                                     account_auth_platform_object::Platform_Permission_Comment |
                                                                     account_auth_platform_object::Platform_Permission_Reward |
                                                                     account_auth_platform_object::Platform_Permission_Post,
                                         bool csaf_fee = true,
                                         bool broadcast = false)
{
   try {
       fc::optional<asset_object_with_data> asset_obj = get_asset(GRAPHENE_CORE_ASSET_AID);
       FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", GRAPHENE_CORE_ASSET_AID));

      account_object user = get_account( account );
      account_object platform_account = get_account( platform_owner );
      auto pa = _remote_db->get_platform_by_account( platform_account.uid );
      FC_ASSERT( pa.valid(), "Account ${platform_owner} is not a platform", ("platform_owner",platform_owner) );
      account_auth_platform_operation op;
      op.uid = user.uid;
      op.platform = pa->owner;

	  account_auth_platform_operation::extension_parameter ext;
      ext.limit_for_platform = asset_obj->amount_from_string(limit_for_platform).amount;
      ext.permission_flags = permission_flags;
      if (memo.size())
      {
          ext.memo = memo_data();
          ext.memo->from = user.memo_key;
          ext.memo->to = platform_account.memo_key;
          ext.memo->set_message(get_private_key(user.memo_key),platform_account.memo_key, memo);
      }
      op.extensions = extension<account_auth_platform_operation::extension_parameter>();
	  op.extensions->value = ext;

      signed_transaction tx;
      tx.operations.push_back( op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee );
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW((account)(platform_owner)(limit_for_platform)(permission_flags)(csaf_fee)(broadcast))
}

signed_transaction account_cancel_auth_platform(string account,
                                            string platform_owner,
                                            bool csaf_fee = true,
                                            bool broadcast = false)
{
   try {
      account_object user = get_account( account );
      account_object platform_account = get_account( platform_owner );

      account_cancel_auth_platform_operation op;
      op.uid = user.uid;
      op.platform = platform_account.uid;
      signed_transaction tx;
      tx.operations.push_back( op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee );
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (account)(platform_owner)(csaf_fee)(broadcast) )
}

   signed_transaction update_committee_member(
                                        string committee_member_account,
                                        optional<string> pledge_amount,
                                        optional<string> pledge_asset_symbol,
                                        optional<string> url,
                                        bool csaf_fee,
                                        bool broadcast /* = false */)
   { try {
      FC_ASSERT( pledge_amount.valid() == pledge_asset_symbol.valid(),
                 "Pledge amount and asset symbol should be both set or both not set" );
      fc::optional<asset> pledge;
      if( pledge_amount.valid() )
      {
         fc::optional<asset_object_with_data> asset_obj = get_asset( *pledge_asset_symbol );
         FC_ASSERT( asset_obj, "Could not find asset matching ${asset}", ("asset", *pledge_asset_symbol) );
         pledge = asset_obj->amount_from_string( *pledge_amount );
      }

      committee_member_object committee_member = get_committee_member( committee_member_account );
      account_object committee_member_account = get_account( committee_member.account );

      committee_member_update_operation committee_member_update_op;
      committee_member_update_op.account = committee_member_account.uid;
      committee_member_update_op.new_pledge = pledge;
      committee_member_update_op.new_url = url;

      signed_transaction tx;
      tx.operations.push_back( committee_member_update_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee );
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (committee_member_account)(pledge_amount)(pledge_asset_symbol)(csaf_fee)(broadcast) ) }

   signed_transaction update_witness_with_details(
                                        string witness_account,
                                        optional<public_key_type> block_signing_key,
                                        optional<string> pledge_amount,
                                        optional<string> pledge_asset_symbol,
                                        optional<string> url,
                                        bool csaf_fee,
                                        bool broadcast /* = false */)
   { try {
      FC_ASSERT( pledge_amount.valid() == pledge_asset_symbol.valid(),
                 "Pledge amount and asset symbol should be both set or both not set" );
      fc::optional<asset> pledge;
      if( pledge_amount.valid() )
      {
         fc::optional<asset_object_with_data> asset_obj = get_asset( *pledge_asset_symbol );
         FC_ASSERT( asset_obj, "Could not find asset matching ${asset}", ("asset", *pledge_asset_symbol) );
         pledge = asset_obj->amount_from_string( *pledge_amount );
      }

      witness_object witness = get_witness( witness_account );
      account_object witness_account = get_account( witness.account );

      witness_update_operation witness_update_op;
      witness_update_op.account = witness_account.uid;
      witness_update_op.new_signing_key = block_signing_key;
      witness_update_op.new_pledge = pledge;
      witness_update_op.new_url = url;

      signed_transaction tx;
      tx.operations.push_back( witness_update_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee );
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (witness_account)(block_signing_key)(pledge_amount)(pledge_asset_symbol)(csaf_fee)(broadcast) ) }

   signed_transaction update_witness(string witness_name,
                                     string url,
                                     string block_signing_key,
                                     bool csaf_fee,
                                     bool broadcast /* = false */)
   { try {
      witness_object witness = get_witness(witness_name);
      account_object witness_account = get_account( witness.account );

      witness_update_operation witness_update_op;
      //witness_update_op.witness = witness.id;
      witness_update_op.account = witness_account.uid;
      if( url != "" )
         witness_update_op.new_url = url;
      if( block_signing_key != "" )
         witness_update_op.new_signing_key = public_key_type( block_signing_key );

      signed_transaction tx;
      tx.operations.push_back( witness_update_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee );
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (witness_name)(url)(block_signing_key)(csaf_fee)(broadcast) ) }

   signed_transaction collect_witness_pay(string witness_account,
                                        string pay_amount,
                                        string pay_asset_symbol,
                                        bool csaf_fee,
                                        bool broadcast /* = false */)
   { try {
      witness_object witness = get_witness(witness_account);

      fc::optional<asset_object_with_data> asset_obj = get_asset( pay_asset_symbol );
      FC_ASSERT( asset_obj, "Could not find asset matching ${asset}", ("asset", pay_asset_symbol) );

      witness_collect_pay_operation witness_collect_pay_op;
      witness_collect_pay_op.account = witness.account;
      witness_collect_pay_op.pay = asset_obj->amount_from_string( pay_amount );

      signed_transaction tx;
      tx.operations.push_back( witness_collect_pay_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee );
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (witness_account)(pay_amount)(pay_asset_symbol)(csaf_fee)(broadcast) ) }

   signed_transaction collect_csaf(string from,
                                   string to,
                                   string amount,
                                   string asset_symbol,
                                   time_point_sec time,
                                   bool csaf_fee,
                                   bool broadcast /* = false */)
   { try {
      FC_ASSERT( !self.is_locked(), "Should unlock first" );
      fc::optional<asset_object_with_data> asset_obj = get_asset(asset_symbol);
      FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));

      account_object from_account = get_account(from);
      account_object to_account = get_account(to);

      csaf_collect_operation cc_op;

      cc_op.from = from_account.uid;
      cc_op.to = to_account.uid;
      cc_op.amount = asset_obj->amount_from_string(amount);
      cc_op.time = time;

      signed_transaction tx;
      tx.operations.push_back(cc_op);
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
      tx.validate();

      return sign_transaction(tx, broadcast);
   } FC_CAPTURE_AND_RETHROW( (from)(to)(amount)(asset_symbol)(time)(csaf_fee)(broadcast) ) }

   signed_transaction update_witness_votes(string voting_account,
                                          flat_set<string> witnesses_to_add,
                                          flat_set<string> witnesses_to_remove,
                                          bool csaf_fee,
                                          bool broadcast /* = false */)
   { try {
      account_object voting_account_object = get_account(voting_account);
      flat_set<account_uid_type> uids_to_add;
      flat_set<account_uid_type> uids_to_remove;
      uids_to_add.reserve( witnesses_to_add.size() );
      uids_to_remove.reserve( witnesses_to_remove.size() );
      for( string wit : witnesses_to_add )
         uids_to_add.insert( get_witness( wit ).account );
      for( string wit : witnesses_to_remove )
         uids_to_remove.insert( get_witness( wit ).account );

      witness_vote_update_operation witness_vote_update_op;
      witness_vote_update_op.voter = voting_account_object.uid;
      witness_vote_update_op.witnesses_to_add = uids_to_add;
      witness_vote_update_op.witnesses_to_remove = uids_to_remove;

      signed_transaction tx;
      tx.operations.push_back( witness_vote_update_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (voting_account)(witnesses_to_add)(witnesses_to_remove)(csaf_fee)(broadcast) ) }

   signed_transaction update_platform_votes(string voting_account,
                                          flat_set<string> platforms_to_add,
                                          flat_set<string> platforms_to_remove,
                                          bool csaf_fee,
                                          bool broadcast )
   { try {
      account_object voting_account_object = get_account( voting_account );
      flat_set<account_uid_type> uids_to_add;
      flat_set<account_uid_type> uids_to_remove;
      uids_to_add.reserve( platforms_to_add.size() );
      uids_to_remove.reserve( platforms_to_remove.size() );
      for( string pla : platforms_to_add )
         uids_to_add.insert( get_platform( pla ).owner );
      for( string pla : platforms_to_remove )
         uids_to_remove.insert( get_platform( pla ).owner );

      platform_vote_update_operation platform_vote_update_op;
      platform_vote_update_op.voter = voting_account_object.uid;
      platform_vote_update_op.platform_to_add = uids_to_add;
      platform_vote_update_op.platform_to_remove = uids_to_remove;

      signed_transaction tx;
      tx.operations.push_back( platform_vote_update_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee );
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (voting_account)(platforms_to_add)(platforms_to_remove)(csaf_fee)(broadcast) ) }

   signed_transaction update_committee_member_votes(string voting_account,
                                          flat_set<string> committee_members_to_add,
                                          flat_set<string> committee_members_to_remove,
                                          bool csaf_fee,
                                          bool broadcast /* = false */)
   { try {
      account_object voting_account_object = get_account(voting_account);
      flat_set<account_uid_type> uids_to_add;
      flat_set<account_uid_type> uids_to_remove;
      uids_to_add.reserve( committee_members_to_add.size() );
      uids_to_remove.reserve( committee_members_to_remove.size() );
      for( string com : committee_members_to_add )
         uids_to_add.insert( get_committee_member( com ).account );
      for( string com : committee_members_to_remove )
         uids_to_remove.insert( get_committee_member( com ).account );

      committee_member_vote_update_operation committee_member_vote_update_op;
      committee_member_vote_update_op.voter = voting_account_object.uid;
      committee_member_vote_update_op.committee_members_to_add = uids_to_add;
      committee_member_vote_update_op.committee_members_to_remove = uids_to_remove;

      signed_transaction tx;
      tx.operations.push_back( committee_member_vote_update_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (voting_account)(committee_members_to_add)(committee_members_to_remove)(csaf_fee)(broadcast) ) }

   signed_transaction set_voting_proxy(string account_to_modify,
                                       optional<string> voting_account,
                                       bool csaf_fee,
                                       bool broadcast /* = false */)
   { try {

      account_update_proxy_operation account_update_op;
      account_update_op.voter = get_account_uid(account_to_modify);
      if (voting_account)
         account_update_op.proxy = get_account_uid(*voting_account);
      else
         account_update_op.proxy = GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID;

      signed_transaction tx;
      tx.operations.push_back( account_update_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (account_to_modify)(voting_account)(csaf_fee)(broadcast) ) }

   signed_transaction enable_allowed_assets(string account,
                                          bool enable,
                                          bool csaf_fee,
                                          bool broadcast /* = false */)
   { try {
      account_enable_allowed_assets_operation op;
      op.account = get_account_uid( account );
      op.enable = enable;

      signed_transaction tx;
      tx.operations.push_back( op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (account)(enable)(csaf_fee)(broadcast) ) }

   signed_transaction update_allowed_assets(string account,
                                          flat_set<string> assets_to_add,
                                          flat_set<string> assets_to_remove,
                                          bool csaf_fee,
                                          bool broadcast /* = false */)
   { try {
      account_object account_obj = get_account( account );
      flat_set<asset_aid_type> aids_to_add;
      flat_set<asset_aid_type> aids_to_remove;
      aids_to_add.reserve( assets_to_add.size() );
      aids_to_remove.reserve( assets_to_remove.size() );
      for( string a : assets_to_add )
         aids_to_add.insert( get_asset( a ).asset_id );
      for( string a : assets_to_remove )
         aids_to_remove.insert( get_asset( a ).asset_id );

      account_update_allowed_assets_operation op;
      op.account = get_account_uid( account );
      op.assets_to_add = aids_to_add;
      op.assets_to_remove = aids_to_remove;

      signed_transaction tx;
      tx.operations.push_back( op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (account)(assets_to_add)(assets_to_remove)(csaf_fee)(broadcast) ) }

   signed_transaction sign_transaction( signed_transaction tx, bool broadcast = false )
   {
      // get required keys to sign the trx
      const auto& result = _remote_db->get_required_signatures( tx, flat_set<public_key_type>() );
      const auto& required_keys = result.first.second;

      // check whether it's possible to fullfil the authority requirement
      if( required_keys.find( public_key_type() ) == required_keys.end() )
      {
         // get a subset of available keys
         flat_set<public_key_type> available_keys;
         flat_map<public_key_type,fc::ecc::private_key> available_keys_map;
         for( const auto& pub_key : required_keys )
         {
            auto it = _keys.find( pub_key );
            if( it != _keys.end() )
            {
               fc::optional<fc::ecc::private_key> privkey = wif_to_key( it->second );
               FC_ASSERT( privkey.valid(), "Malformed private key in _keys" );
               available_keys.insert( pub_key );
               available_keys_map[ pub_key ] = *privkey;
            }
         }

         // if we have the required key(s), preceed to sign
         if( !available_keys.empty() )
         {
            // get a subset of required keys
            const auto& new_result = _remote_db->get_required_signatures( tx, available_keys );
            const auto& required_keys_subset = new_result.first.first;
            //const auto& missed_keys = new_result.first.second;
            const auto& unused_signatures = new_result.second;

            // unused signatures can be removed safely
            for( const auto& sig : unused_signatures )
               tx.signatures.erase( std::remove( tx.signatures.begin(), tx.signatures.end(), sig), tx.signatures.end() );

            bool no_sig = tx.signatures.empty();
            auto dyn_props = get_dynamic_global_properties();

            // if no signature is included in the trx, reset the tapos data; otherwise keep the tapos data
            if( no_sig )
               tx.set_reference_block( dyn_props.head_block_id );

            // if no signature is included in the trx, reset expiration time; otherwise keep it
            if( no_sig )
            {
               // first, some bookkeeping, expire old items from _recently_generated_transactions
               // since transactions include the head block id, we just need the index for keeping transactions unique
               // when there are multiple transactions in the same block.  choose a time period that should be at
               // least one block long, even in the worst case.  5 minutes ought to be plenty.
               fc::time_point_sec oldest_transaction_ids_to_track(dyn_props.time - fc::minutes(5));
               auto oldest_transaction_record_iter = _recently_generated_transactions.get<timestamp_index>().lower_bound(oldest_transaction_ids_to_track);
               auto begin_iter = _recently_generated_transactions.get<timestamp_index>().begin();
               _recently_generated_transactions.get<timestamp_index>().erase(begin_iter, oldest_transaction_record_iter);

            }

            uint32_t expiration_time_offset = 0;
            for (;;)
            {
               if( no_sig )
               {
                  tx.set_expiration( dyn_props.time + fc::seconds(120 + expiration_time_offset) );
                  tx.signatures.clear();
               }

               //idump((required_keys_subset)(available_keys));
               // TODO: for better performance, sign after dupe check
               for( const auto& key : required_keys_subset )
               {
                  tx.sign( available_keys_map[key], _chain_id );
                  /// TODO: if transaction has enough signatures to be "valid" don't add any more,
                  /// there are cases where the wallet may have more keys than strictly necessary and
                  /// the transaction will be rejected if the transaction validates without requiring
                  /// all signatures provided
               }

               graphene::chain::transaction_id_type this_transaction_id = tx.id();
               auto iter = _recently_generated_transactions.find(this_transaction_id);
               if (iter == _recently_generated_transactions.end())
               {
                  // we haven't generated this transaction before, the usual case
                  recently_generated_transaction_record this_transaction_record;
                  this_transaction_record.generation_time = dyn_props.time;
                  this_transaction_record.transaction_id = this_transaction_id;
                  _recently_generated_transactions.insert(this_transaction_record);
                  break;
               }

               // if there was a signature included in the trx, we can not update expiration field
               if( !no_sig ) break;

               // if we've generated a dupe, increment expiration time and re-sign it
               ++expiration_time_offset;
            }

         }
      }

      //wdump((tx));

      if( broadcast )
      {
         try
         {
            _remote_net_broadcast->broadcast_transaction( tx );
         }
         catch (const fc::exception& e)
         {
            elog("Caught exception while broadcasting tx ${id}:  ${e}", ("id", tx.id().str())("e", e.to_detail_string()) );
            throw;
         }
      }

      return tx;
   }

   signed_transaction transfer(string from, string to, string amount,
                               string asset_symbol, string memo, bool csaf_fee = true, bool broadcast = false)
   { try {
      FC_ASSERT( !self.is_locked(), "Should unlock first" );
      fc::optional<asset_object_with_data> asset_obj = get_asset(asset_symbol);
      FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));

      account_object from_account = get_account(from);
      account_object to_account = get_account(to);

      transfer_operation xfer_op;

      xfer_op.from = from_account.uid;
      xfer_op.to = to_account.uid;
      xfer_op.amount = asset_obj->amount_from_string(amount);

      if( memo.size() )
         {
            xfer_op.memo = memo_data();
            xfer_op.memo->from = from_account.memo_key;
            xfer_op.memo->to = to_account.memo_key;
            xfer_op.memo->set_message(get_private_key(from_account.memo_key),
                                      to_account.memo_key, memo);
         }

      //xfer_op.fee = fee_type(asset(_remote_db->get_required_fee_data({ xfer_op }).at(0).min_fee));
      signed_transaction tx;
      tx.operations.push_back(xfer_op);
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
      tx.validate();

      return sign_transaction(tx, broadcast);
   } FC_CAPTURE_AND_RETHROW( (from)(to)(amount)(asset_symbol)(memo)(csaf_fee)(broadcast) ) }

   signed_transaction transfer_extension(string from, string to, string amount,
                               string asset_symbol, string memo, bool isfrom_balance = true, bool isto_balance = true, bool csaf_fee = true, bool broadcast = false)
   {
       try {
           FC_ASSERT(!self.is_locked(), "Should unlock first");
           fc::optional<asset_object_with_data> asset_obj = get_asset(asset_symbol);
           FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));

           account_object from_account = get_account(from);
           account_object to_account = get_account(to);

           transfer_operation xfer_op;
           xfer_op.extensions = extension< transfer_operation::ext >();
           if (isfrom_balance)
               xfer_op.extensions->value.from_balance = asset_obj->amount_from_string(amount);
           else
               xfer_op.extensions->value.from_prepaid = asset_obj->amount_from_string(amount);
           if (isto_balance)
               xfer_op.extensions->value.to_balance = asset_obj->amount_from_string(amount);
           else
               xfer_op.extensions->value.to_prepaid = asset_obj->amount_from_string(amount);

           xfer_op.from = from_account.uid;
           xfer_op.to = to_account.uid;
           xfer_op.amount = asset_obj->amount_from_string(amount);

           if (memo.size())
           {
               xfer_op.memo = memo_data();
               xfer_op.memo->from = from_account.memo_key;
               xfer_op.memo->to = to_account.memo_key;
               xfer_op.memo->set_message(get_private_key(from_account.memo_key),
                   to_account.memo_key, memo);
           }

           signed_transaction tx;
           tx.operations.push_back(xfer_op);
           set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
           tx.validate();

           return sign_transaction(tx, broadcast);
       } FC_CAPTURE_AND_RETHROW((from)(to)(amount)(asset_symbol)(memo)(isfrom_balance)(isto_balance)(csaf_fee)(broadcast))
   }

   signed_transaction override_transfer(string from, string to, string amount,
                               string asset_symbol, string memo, bool csaf_fee = true, bool broadcast = false)
   { try {
      FC_ASSERT( !self.is_locked(), "Should unlock first" );
      fc::optional<asset_object_with_data> asset_obj = get_asset(asset_symbol);
      FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));

      account_object issuer_account = get_account(asset_obj->issuer);
      account_object from_account = get_account(from);
      account_object to_account = get_account(to);

      override_transfer_operation xfer_op;

      xfer_op.issuer = issuer_account.uid;
      xfer_op.from = from_account.uid;
      xfer_op.to = to_account.uid;
      xfer_op.amount = asset_obj->amount_from_string(amount);

      if( memo.size() )
      {
         xfer_op.memo = memo_data();
         xfer_op.memo->from = issuer_account.memo_key;
         xfer_op.memo->to = to_account.memo_key;
         xfer_op.memo->set_message(get_private_key(issuer_account.memo_key),
                                   to_account.memo_key, memo);
      }

      signed_transaction tx;
      tx.operations.push_back(xfer_op);
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
      tx.validate();

      return sign_transaction(tx, broadcast);
   } FC_CAPTURE_AND_RETHROW( (from)(to)(amount)(asset_symbol)(memo)(csaf_fee)(broadcast) ) }

   signed_transaction issue_asset(string to_account, string amount, string symbol,
                                  string memo, bool csaf_fee = true, bool broadcast = false)
   {
      auto asset_obj = get_asset(symbol);

      account_object to = get_account(to_account);
      account_object issuer = get_account(asset_obj.issuer);

      asset_issue_operation issue_op;
      issue_op.issuer           = asset_obj.issuer;
      issue_op.asset_to_issue   = asset_obj.amount_from_string(amount);
      issue_op.issue_to_account = to.uid;

      if( memo.size() )
      {
         issue_op.memo = memo_data();
         issue_op.memo->from = issuer.memo_key;
         issue_op.memo->to = to.memo_key;
         issue_op.memo->set_message(get_private_key(issuer.memo_key),
                                    to.memo_key, memo);
      }

      signed_transaction tx;
      tx.operations.push_back(issue_op);
      set_operation_fees(tx,_remote_db->get_global_properties().parameters.current_fees, csaf_fee);
      tx.validate();

      return sign_transaction(tx, broadcast);
   }

   std::map<string,std::function<string(fc::variant,const fc::variants&)>> get_result_formatters() const
   {
      std::map<string,std::function<string(fc::variant,const fc::variants&)> > m;
      m["help"] = [](variant result, const fc::variants& a)
      {
         return result.get_string();
      };

      m["gethelp"] = [](variant result, const fc::variants& a)
      {
         return result.get_string();
      };

      m["get_relative_account_history"] = [this](variant result, const fc::variants& a)
      {
         auto r = result.as<vector<operation_detail>>( GRAPHENE_MAX_NESTED_OBJECTS );
         std::stringstream ss;

         ss << "#" << " ";
         ss << "block_num" << " ";
         ss << "time             " << " ";
         ss << "description/fee_payer/fee/operation_result" << " ";
         ss << " \n";
         for( operation_detail& d : r )
         {
            operation_history_object& i = d.op;
            ss << d.sequence << " ";
            ss << i.block_num << " ";
            ss << i.block_timestamp.to_iso_string() << " ";
            i.op.visit(operation_printer(ss, *this, i.result));
            ss << " \n";
         }

         return ss.str();
      };

      m["list_account_balances"] = [this](variant result, const fc::variants& a)
      {
         auto r = result.as<vector<asset>>( GRAPHENE_MAX_NESTED_OBJECTS );
         vector<asset_object_with_data> asset_recs;
         std::transform(r.begin(), r.end(), std::back_inserter(asset_recs), [this](const asset& a) {
            return get_asset(a.asset_id);
         });

         std::stringstream ss;
         for( unsigned i = 0; i < asset_recs.size(); ++i )
            ss << asset_recs[i].amount_to_pretty_string(r[i]) << "\n";

         return ss.str();
      };

      return m;
   }

   signed_transaction committee_proposal_create(
         const string committee_member_account,
         const vector<committee_proposal_item_type> items,
         const uint32_t voting_closing_block_num,
         optional<voting_opinion_type> proposer_opinion,
         const uint32_t execution_block_num = 0,
         const uint32_t expiration_block_num = 0,
         bool csaf_fee = true,
         bool broadcast = false
      )
   { try {
      committee_proposal_create_operation op;
      op.proposer = get_account_uid( committee_member_account );
      op.items = items;
      op.voting_closing_block_num = voting_closing_block_num;
      op.proposer_opinion = proposer_opinion;
      op.execution_block_num = execution_block_num;
      op.expiration_block_num = expiration_block_num;

      signed_transaction tx;
      tx.operations.push_back( op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
      tx.validate();

      return sign_transaction( tx, broadcast );

   } FC_CAPTURE_AND_RETHROW( (committee_member_account)(items)(voting_closing_block_num)(proposer_opinion)(execution_block_num)(expiration_block_num)(csaf_fee)(broadcast) ) }

   signed_transaction committee_proposal_vote(
      const string committee_member_account,
      const uint64_t proposal_number,
      const voting_opinion_type opinion,
      bool csaf_fee = true,
      bool broadcast = false
      )
   { try {
      committee_proposal_update_operation update_op;
      update_op.account = get_account_uid( committee_member_account );
      update_op.proposal_number = proposal_number;
      update_op.opinion = opinion;

      signed_transaction tx;
      tx.operations.push_back( update_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (committee_member_account)(proposal_number)(opinion)(csaf_fee)(broadcast) ) }

   signed_transaction proposal_create(const string              fee_paying_account,
                                      const vector<op_wrapper>  proposed_ops,
                                      time_point_sec            expiration_time,
                                      uint32_t                  review_period_seconds,
                                      bool                      csaf_fee = true,
                                      bool                      broadcast = false
                                      )
   {
       try {
           proposal_create_operation op;
           op.fee_paying_account = get_account_uid(fee_paying_account);
           op.proposed_ops = proposed_ops;
           op.expiration_time = expiration_time;
           op.review_period_seconds = review_period_seconds;

           signed_transaction tx;
           tx.operations.push_back(op);
           set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
           tx.validate();

           return sign_transaction(tx, broadcast);

       } FC_CAPTURE_AND_RETHROW((fee_paying_account)(proposed_ops)(expiration_time)(review_period_seconds)(csaf_fee)(broadcast))
   }

   signed_transaction proposal_update(const string                      fee_paying_account,
                                      proposal_id_type                  proposal,
                                      const flat_set<account_uid_type>  secondary_approvals_to_add,
                                      const flat_set<account_uid_type>  secondary_approvals_to_remove,
                                      const flat_set<account_uid_type>  active_approvals_to_add,
                                      const flat_set<account_uid_type>  active_approvals_to_remove,
                                      const flat_set<account_uid_type>  owner_approvals_to_add,
                                      const flat_set<account_uid_type>  owner_approvals_to_remove,
                                      const flat_set<public_key_type>   key_approvals_to_add,
                                      const flat_set<public_key_type>   key_approvals_to_remove,
                                      bool                              csaf_fee = true,
                                      bool                              broadcast = false
                                      )
   {
       try {
           proposal_update_operation op;
           op.fee_paying_account = get_account_uid(fee_paying_account);
           op.proposal = proposal;
           
           op.secondary_approvals_to_add    = secondary_approvals_to_add;
           op.secondary_approvals_to_remove = secondary_approvals_to_remove;
           op.active_approvals_to_add       = active_approvals_to_add;
           op.active_approvals_to_remove    = active_approvals_to_remove;
           op.owner_approvals_to_add        = owner_approvals_to_add; 
           op.owner_approvals_to_remove     = owner_approvals_to_remove;
           op.key_approvals_to_add          = key_approvals_to_add;
           op.key_approvals_to_remove       = key_approvals_to_remove;

           signed_transaction tx;
           tx.operations.push_back(op);
           set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
           tx.validate();

           return sign_transaction(tx, broadcast);

       } FC_CAPTURE_AND_RETHROW((fee_paying_account)(proposal)(secondary_approvals_to_add)(secondary_approvals_to_remove)(active_approvals_to_add)(active_approvals_to_remove)
           (owner_approvals_to_add)(owner_approvals_to_remove)(key_approvals_to_add)(key_approvals_to_remove)(csaf_fee)(broadcast))
   }

   signed_transaction proposal_delete(const string              fee_paying_account,
                                      proposal_id_type          proposal,
                                      bool                      csaf_fee = true,
                                      bool                      broadcast = false
                                      )
   {
       try {
           proposal_delete_operation op;
           op.fee_paying_account = get_account_uid(fee_paying_account);
           op.proposal = proposal;

           signed_transaction tx;
           tx.operations.push_back(op);
           set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
           tx.validate();

           return sign_transaction(tx, broadcast);

       } FC_CAPTURE_AND_RETHROW((fee_paying_account)(proposal)(csaf_fee)(broadcast))
   }

   signed_transaction score_a_post(string from_account,
                                   string platform,
                                   string poster,
                                   post_pid_type    post_pid,
                                   int8_t           score,
                                   string           csaf,
                                   bool csaf_fee = true,
                                   bool broadcast = false)
   {
       try {
           fc::optional<asset_object_with_data> asset_obj = get_asset(GRAPHENE_CORE_ASSET_AID);
           FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", GRAPHENE_CORE_ASSET_AID));

           score_create_operation create_op;
           create_op.from_account_uid = get_account_uid(from_account);
           create_op.platform = get_account_uid(platform);
           create_op.poster = get_account_uid(poster);
           create_op.post_pid = post_pid;
           create_op.score = score;
           create_op.csaf = asset_obj->amount_from_string(csaf).amount;

           signed_transaction tx;
           tx.operations.push_back(create_op);
           set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
           tx.validate();

           return sign_transaction(tx, broadcast);
       } FC_CAPTURE_AND_RETHROW((from_account)(platform)(poster)(post_pid)(score)(csaf)(csaf_fee)(broadcast))
   }

   signed_transaction reward_post(string           from_account,
                                  string           platform,
                                  string           poster,
                                  post_pid_type    post_pid,
                                  string           amount,
                                  string           asset_symbol,
                                  bool csaf_fee = true,
                                  bool broadcast = false)
   {
       try {
           FC_ASSERT(!self.is_locked(), "Should unlock first");
           fc::optional<asset_object_with_data> asset_obj = get_asset(asset_symbol);
           FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));

           reward_operation reward_op;
           reward_op.from_account_uid = get_account_uid(from_account);
           reward_op.platform = get_account_uid(platform);
           reward_op.poster = get_account_uid(poster);
           reward_op.post_pid = post_pid;
           reward_op.amount = asset_obj->amount_from_string(amount);

           signed_transaction tx;
           tx.operations.push_back(reward_op);
           set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
           tx.validate();

           return sign_transaction(tx, broadcast);
       } FC_CAPTURE_AND_RETHROW((from_account)(platform)(poster)(post_pid)(amount)(asset_symbol)(csaf_fee)(broadcast))
   }

   signed_transaction reward_post_proxy_by_platform(string           from_account,
                                                    string           platform,
                                                    string           poster,
                                                    post_pid_type    post_pid,
                                                    string           amount,
                                                    bool csaf_fee = true,
                                                    bool broadcast = false)
   {
       try {
           FC_ASSERT(!self.is_locked(), "Should unlock first");
           fc::optional<asset_object_with_data> asset_obj = get_asset(GRAPHENE_CORE_ASSET_AID);
           FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", GRAPHENE_CORE_ASSET_AID));

           reward_proxy_operation reward_op;
           reward_op.from_account_uid = get_account_uid(from_account);
           reward_op.platform = get_account_uid(platform);
           reward_op.poster = get_account_uid(poster);
           reward_op.post_pid = post_pid;
           reward_op.amount = asset_obj->amount_from_string(amount).amount;

           signed_transaction tx;
           tx.operations.push_back(reward_op);
           set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
           tx.validate();

           return sign_transaction(tx, broadcast);
       } FC_CAPTURE_AND_RETHROW((from_account)(platform)(poster)(post_pid)(amount)(csaf_fee)(broadcast))
   }

   signed_transaction buyout_post(string           from_account,
                                  string           platform,
                                  string           poster,
                                  post_pid_type    post_pid,
                                  string           receiptor_account,
                                  bool csaf_fee = true,
                                  bool broadcast = false)
   {
       try {
           FC_ASSERT(!self.is_locked(), "Should unlock first");

           buyout_operation buyout_op;
           buyout_op.from_account_uid = get_account_uid(from_account);
           buyout_op.platform = get_account_uid(platform);
           buyout_op.poster = get_account_uid(poster);
           buyout_op.post_pid = post_pid;
           buyout_op.receiptor_account_uid = get_account_uid(receiptor_account);

           signed_transaction tx;
           tx.operations.push_back(buyout_op);
           set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
           tx.validate();

           return sign_transaction(tx, broadcast);
       } FC_CAPTURE_AND_RETHROW((from_account)(platform)(poster)(post_pid)(receiptor_account)(csaf_fee)(broadcast))
   }

   signed_transaction create_license(string           platform,
                                     uint8_t          license_type,
                                     string           hash_value,
                                     string           title,
                                     string           body,
                                     string           extra_data,
                                     bool csaf_fee = true,
                                     bool broadcast = false)
   {
       try {
           FC_ASSERT(!self.is_locked(), "Should unlock first");

           account_uid_type platform_uid = get_account_uid(platform);
           fc::optional<platform_object> platform_obj = _remote_db->get_platform_by_account(platform_uid);
           FC_ASSERT(platform_obj.valid(), "platform doesn`t exsit. ");

           license_create_operation create_op;
           create_op.license_lid = platform_obj->last_license_sequence + 1;
           create_op.platform = platform_uid;
           create_op.type = license_type;
           create_op.hash_value = hash_value;
           create_op.extra_data = extra_data;
           create_op.title = title;
           create_op.body = body;

           signed_transaction tx;
           tx.operations.push_back(create_op);
           set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
           tx.validate();

           return sign_transaction(tx, broadcast);
       } FC_CAPTURE_AND_RETHROW((platform)(license_type)(hash_value)(title)(body)(extra_data)(csaf_fee)(broadcast))
   }

   signed_transaction create_post(string           platform,
                                  string           poster,
                                  string           hash_value,
                                  string           title,
                                  string           body,
                                  string           extra_data,
                                  string           origin_platform = "",
                                  string           origin_poster = "",
                                  string           origin_post_pid = "",
                                  post_create_ext  exts = post_create_ext(),
                                  bool csaf_fee = true,
                                  bool broadcast = false)
   {
       try {
           FC_ASSERT(!self.is_locked(), "Should unlock first");
           fc::optional<asset_object_with_data> asset_obj = get_asset(GRAPHENE_CORE_ASSET_AID);
           FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", GRAPHENE_CORE_ASSET_AID));

           account_uid_type poster_uid = get_account_uid(poster);
           const account_statistics_object& poster_account_statistics = _remote_db->get_account_statistics_by_uid(poster_uid);
           post_operation create_op;
           create_op.post_pid = poster_account_statistics.last_post_sequence + 1;
           create_op.platform = get_account_uid(platform);
           create_op.poster = poster_uid;
           if (!origin_platform.empty())
               create_op.origin_platform = get_account_uid(origin_platform);
           if (!origin_poster.empty())
               create_op.origin_poster = get_account_uid(origin_poster);
           if (!origin_post_pid.empty())
               create_op.origin_post_pid = fc::to_uint64(fc::string(origin_post_pid));
           
           create_op.hash_value = hash_value;
           create_op.extra_data = extra_data;
           create_op.title = title;
           create_op.body = body;

           post_operation::ext extension_;
           if (exts.post_type)
               extension_.post_type = exts.post_type;
           if (exts.forward_price.valid())
               extension_.forward_price = asset_obj->amount_from_string(*(exts.forward_price)).amount;
           if (exts.receiptors.valid())
           {
               map<account_uid_type, Recerptor_Parameter> maps_receiptors;
               for (auto itor = (*exts.receiptors).begin(); itor != (*exts.receiptors).end(); itor++)
               {
                   Recerptor_Parameter para;
                   para.cur_ratio = uint16_t(itor->second.cur_ratio * GRAPHENE_1_PERCENT);
                   para.to_buyout = itor->second.to_buyout;
                   para.buyout_ratio = uint16_t(itor->second.buyout_ratio * GRAPHENE_1_PERCENT);
                   para.buyout_price = asset_obj->amount_from_string(itor->second.buyout_price).amount;
                   maps_receiptors.insert(std::make_pair(itor->first, para));
               }
               extension_.receiptors = maps_receiptors;
           }  
           if (exts.license_lid.valid())
               extension_.license_lid = exts.license_lid;
           if (exts.permission_flags)
               extension_.permission_flags = exts.permission_flags;
           create_op.extensions = graphene::chain::extension<post_operation::ext>();
           create_op.extensions->value = extension_;

           signed_transaction tx;
           tx.operations.push_back(create_op);
           set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
           tx.validate();

           return sign_transaction(tx, broadcast);
       } FC_CAPTURE_AND_RETHROW((platform)(poster)(hash_value)(title)(body)(extra_data)(origin_platform)(origin_poster)(origin_post_pid)(exts)(csaf_fee)(broadcast))
   }

   signed_transaction update_post(string           platform,
                                  string           poster,
                                  string           post_pid,
                                  string           hash_value = "",
                                  string           title = "",
                                  string           body = "",
                                  string           extra_data = "",
                                  post_update_ext ext = post_update_ext(),
                                  bool csaf_fee = true,
                                  bool broadcast = false)
   {
       try {
           FC_ASSERT(!self.is_locked(), "Should unlock first");
           fc::optional<asset_object_with_data> asset_obj = get_asset(GRAPHENE_CORE_ASSET_AID);
           FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", GRAPHENE_CORE_ASSET_AID));

           post_update_operation update_op;
           update_op.post_pid = fc::to_uint64(fc::string(post_pid));
           update_op.platform = get_account_uid(platform);
           update_op.poster = get_account_uid(poster);

           if (!hash_value.empty())
               update_op.hash_value = hash_value;
           if (!extra_data.empty())
               update_op.extra_data = extra_data;
           if (!title.empty())
               update_op.title = title;
           if (!body.empty())
               update_op.body = body;

           update_op.extensions = graphene::chain::extension<post_update_operation::ext>();
           if (ext.forward_price.valid())
               update_op.extensions->value.forward_price = asset_obj->amount_from_string(*(ext.forward_price)).amount;
           if (ext.receiptor.valid())
               update_op.extensions->value.receiptor = get_account_uid(*(ext.receiptor));
           if (ext.to_buyout.valid())
               update_op.extensions->value.to_buyout = ext.to_buyout;
           if (ext.buyout_ratio.valid())
               update_op.extensions->value.buyout_ratio = uint16_t((*(ext.buyout_ratio))* GRAPHENE_1_PERCENT);
           if (ext.buyout_price.valid())
               update_op.extensions->value.buyout_price = asset_obj->amount_from_string(*(ext.buyout_price)).amount;
           if (ext.buyout_expiration.valid())
               update_op.extensions->value.buyout_expiration = time_point_sec(*(ext.buyout_expiration));
           if (ext.license_lid.valid())
               update_op.extensions->value.license_lid = ext.license_lid;
           if (ext.permission_flags.valid())
               update_op.extensions->value.permission_flags = ext.permission_flags;

           signed_transaction tx;
           tx.operations.push_back(update_op);
           set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
           tx.validate();

           return sign_transaction(tx, broadcast);
       } FC_CAPTURE_AND_RETHROW((platform)(poster)(post_pid)(hash_value)(title)(body)(extra_data)(ext)(csaf_fee)(broadcast))
   }

   signed_transaction account_manage(string executor,
                                     string account,
                                     account_manage_operation::opt options,
                                     bool csaf_fee = true,
                                     bool broadcast = false
                                     )
   {
       try {
           FC_ASSERT(!self.is_locked(), "Should unlock first");

           account_manage_operation manage_op;
           manage_op.account = get_account_uid(account);
           manage_op.executor = get_account_uid(executor);
           manage_op.options.value = options;

           signed_transaction tx;
           tx.operations.push_back(manage_op);
           set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
           tx.validate();

           return sign_transaction(tx, broadcast);
       } FC_CAPTURE_AND_RETHROW((executor)(account)(options)(csaf_fee)(broadcast))
   }

   signed_transaction buy_advertising(string               account,
                                      string               platform,
                                      advertising_aid_type advertising_aid,
                                      uint32_t             start_time,
                                      uint32_t             buy_number,
                                      string               extra_data,
                                      string               memo,
                                      bool                 csaf_fee = true,
                                      bool                 broadcast = false
                                      )
   {
      try {
         FC_ASSERT(!self.is_locked(), "Should unlock first");
         account_uid_type platform_uid = get_account_uid(platform);
         account_uid_type account_uid = get_account_uid(account);

         optional<advertising_object> ad_obj = _remote_db->get_advertising(platform_uid, advertising_aid);
         FC_ASSERT(ad_obj.valid(), "advertising_object doesn`t exsit. ");
         advertising_buy_operation buy_op;
         buy_op.from_account = account_uid;
         buy_op.platform = platform_uid;
         buy_op.advertising_aid = advertising_aid;
         buy_op.advertising_order_oid = ad_obj->last_order_sequence + 1;
         buy_op.start_time = time_point_sec(start_time);
         buy_op.buy_number = buy_number;
         buy_op.extra_data = extra_data;

         account_object user = get_account(account);
         account_object platform_account = get_account(platform);
         if (memo.size())
         {
             buy_op.memo = memo_data();
             buy_op.memo->from = user.memo_key;
             buy_op.memo->to = platform_account.memo_key;
             buy_op.memo->set_message(get_private_key(user.memo_key), platform_account.memo_key, memo);
         }

         signed_transaction tx;
         tx.operations.push_back(buy_op);
         set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
         tx.validate();

         return sign_transaction(tx, broadcast);

      } FC_CAPTURE_AND_RETHROW((account)(platform)(advertising_aid)(start_time)(buy_number)(extra_data)(memo)(csaf_fee)(broadcast))
   }

   signed_transaction confirm_advertising(string              platform,
                                          advertising_aid_type        advertising_aid,
                                          advertising_order_oid_type  advertising_order_oid,
                                          bool                comfirm,
                                          bool                csaf_fee = true,
                                          bool                broadcast = false
                                         )
   {
      try {
         FC_ASSERT(!self.is_locked(), "Should unlock first");

         advertising_confirm_operation confirm_op;
         confirm_op.platform = get_account_uid(platform);
         confirm_op.advertising_aid = advertising_aid;
         confirm_op.advertising_order_oid = advertising_order_oid;
         confirm_op.iscomfirm = comfirm;

         signed_transaction tx;
         tx.operations.push_back(confirm_op);
         set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
         tx.validate();

         return sign_transaction(tx, broadcast);

      } FC_CAPTURE_AND_RETHROW((platform)(advertising_aid)(advertising_order_oid)(comfirm)(csaf_fee)(broadcast))
   }

   post_object get_post(string platform_owner,
                        string poster_uid,
                        string post_pid)
   {
       try {
           post_pid_type postid = fc::to_uint64(fc::string(post_pid));
           account_uid_type platform = get_account_uid(platform_owner);
           account_uid_type poster = get_account_uid(poster_uid);
           fc::optional<post_object> post = _remote_db->get_post(platform, poster, postid);
           if (post)
              return *post;
           else
              FC_THROW("poster: ${poster} don't publish post: ${post} in platform: ${platform}", 
              ("poster", poster_uid)("post", post_pid)("platform", platform_owner));
       } FC_CAPTURE_AND_RETHROW((platform_owner)(poster_uid)(post_pid))
   }

   vector<post_object> get_posts_by_platform_poster(string           platform_owner,
                                                    optional<string> poster,
                                                    time_point_sec   begin_time_range,
                                                    time_point_sec   end_time_range,
                                                    object_id_type   lower_bound_post,
                                                    uint32_t         limit)
   {
       try {
           account_uid_type platform = get_account_uid(platform_owner);
           if (poster.valid()){
               account_uid_type poster_uid = get_account_uid(*poster);
               return _remote_db->get_posts_by_platform_poster(platform, poster_uid, std::make_pair(begin_time_range, end_time_range), lower_bound_post, limit);
           }
           else{
               return _remote_db->get_posts_by_platform_poster(platform, optional<account_uid_type>(), std::make_pair(begin_time_range, end_time_range), lower_bound_post, limit);
           }
           
       } FC_CAPTURE_AND_RETHROW((platform_owner)(poster)(begin_time_range)(end_time_range)(lower_bound_post)(limit))
   }

   score_object get_score(string platform,
                          string poster_uid,
                          string post_pid,
                          string from_account)
   {
       try {
           post_pid_type postid = fc::to_uint64(fc::string(post_pid));
           account_uid_type platform_uid = get_account_uid(platform);
           account_uid_type poster = get_account_uid(poster_uid);
           account_uid_type from_uid = get_account_uid(from_account);
           fc::optional<score_object> score = _remote_db->get_score(platform_uid, poster, postid, from_uid);
           if (score)
              return *score;
           else
              FC_THROW("score that form account : ${from_account} for post£º${post} created by poster: ${poster} in platform: ${platform} not found", 
              ("from_account", from_account)("post", post_pid)("poster", poster_uid)("platform", platform));
       } FC_CAPTURE_AND_RETHROW((platform)(poster_uid)(post_pid)(from_account))
   }

   vector<score_object> get_scores_by_uid(string   scorer,
                                          uint32_t period,
                                          object_id_type lower_bound_score,
                                          uint32_t limit)
   {
      try {
         account_uid_type scorer_uid = get_account_uid(scorer);
         return _remote_db->get_scores_by_uid(scorer_uid, period, lower_bound_score, limit);
      } FC_CAPTURE_AND_RETHROW((scorer)(period)(lower_bound_score)(limit))
   }

   vector<score_object> list_scores(string   platform,
                                    string   poster_uid,
                                    string   post_pid,
                                    object_id_type lower_bound_score,
                                    uint32_t       limit,
                                    bool           list_cur_period)
   {
       try {
           post_pid_type postid = fc::to_uint64(fc::string(post_pid));
           account_uid_type platform_uid = get_account_uid(platform);
           account_uid_type poster = get_account_uid(poster_uid);
           return _remote_db->list_scores(platform_uid, poster, postid, lower_bound_score, limit, list_cur_period);
       } FC_CAPTURE_AND_RETHROW((platform)(poster_uid)(post_pid)(lower_bound_score)(limit)(list_cur_period))
   }

   license_object get_license(string platform,
                              string license_lid)
   {
       try {
           account_uid_type platform_uid = get_account_uid(platform);
           license_lid_type lid = fc::to_uint64(fc::string(license_lid));
           fc::optional<license_object> license = _remote_db->get_license(platform_uid, lid);
           if (license)
              return *license;
           else
              FC_THROW("license: ${license} not found in platform: ${platform}",("license", license_lid)("platform", platform));
       } FC_CAPTURE_AND_RETHROW((platform)(license_lid))
   }

   vector<license_object> list_licenses(string platform, object_id_type lower_bound_license, uint32_t limit)
   {
       try {
           account_uid_type platform_uid = get_account_uid(platform);
           return _remote_db->list_licenses(platform_uid, lower_bound_license, limit);
       } FC_CAPTURE_AND_RETHROW((platform)(lower_bound_license)(limit))
   }

   vector<advertising_object> list_advertisings(string platform, string lower_bound_advertising, uint32_t limit)
   {
       try {
           account_uid_type platform_uid = get_account_uid(platform);
           advertising_aid_type lower_advertising_aid = fc::to_uint64(fc::string(lower_bound_advertising));
           return _remote_db->list_advertisings(platform_uid, lower_advertising_aid, limit);
       } FC_CAPTURE_AND_RETHROW((platform)(lower_bound_advertising)(limit))
   }

   vector<active_post_object> get_post_profits_detail(uint32_t         begin_period,
                                                      uint32_t         end_period,
                                                      string           platform,
                                                      string           poster,
                                                      string           post_pid)
   {
       try {
           FC_ASSERT(begin_period <= end_period, "begin_period should be less then end_period.");
           account_uid_type platform_uid = get_account_uid(platform);
           account_uid_type poster_uid = get_account_uid(poster);
           post_pid_type postid = fc::to_uint64(fc::string(post_pid));
           return _remote_db->get_post_profits_detail(begin_period, end_period, platform_uid, poster_uid, postid);
       } FC_CAPTURE_AND_RETHROW((begin_period)(end_period)(platform)(poster)(post_pid))
   }

   vector<Platform_Period_Profit_Detail> get_platform_profits_detail(uint32_t         begin_period,
                                                                     uint32_t         end_period,
                                                                     string           platform)
   {
       try {
           FC_ASSERT(begin_period <= end_period, "begin_period should be less then end_period.");
           account_uid_type platform_uid = get_account_uid(platform);
           return _remote_db->get_platform_profits_detail(begin_period, end_period, platform_uid);
       } FC_CAPTURE_AND_RETHROW((begin_period)(end_period)(platform))
   }

   vector<Poster_Period_Profit_Detail> get_poster_profits_detail(uint32_t         begin_period,
                                                                 uint32_t         end_period,
                                                                 string           poster)
   {
       try {
           FC_ASSERT(begin_period <= end_period, "begin_period should be less then end_period.");
           account_uid_type poster_uid = get_account_uid(poster);
           return _remote_db->get_poster_profits_detail(begin_period, end_period, poster_uid);
       } FC_CAPTURE_AND_RETHROW((begin_period)(end_period)(poster))
   }

   share_type get_score_profit(string account, uint32_t period)
   {
      try {
         account_uid_type account_uid = get_account_uid(account);
         auto dynamic_props = _remote_db->get_dynamic_global_properties();
         FC_ASSERT(period <= dynamic_props.current_active_post_sequence, "period does not exist");
         return _remote_db->get_score_profit(account_uid, period);
      } FC_CAPTURE_AND_RETHROW((account)(period))
   }

   account_statistics_object get_account_statistics(string account)
   {
       try {
           account_uid_type account_uid = get_account_uid(account);
           account_statistics_object plat_account_statistics = _remote_db->get_account_statistics_by_uid(account_uid);
           return plat_account_statistics;
       } FC_CAPTURE_AND_RETHROW((account))
   }

   signed_transaction create_advertising(string           platform,
                                         string           description,
                                         string           unit_price,
                                         uint32_t         unit_time,
                                         bool             csaf_fee,
                                         bool             broadcast)
   {
       try {
           FC_ASSERT(!self.is_locked(), "Should unlock first");
           fc::optional<asset_object_with_data> asset_obj = get_asset(GRAPHENE_CORE_ASSET_AID);
           FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", GRAPHENE_CORE_ASSET_AID));

           account_uid_type platform_uid = get_account_uid(platform);
           fc::optional<platform_object> platform_obj = _remote_db->get_platform_by_account(platform_uid);
           FC_ASSERT(platform_obj.valid(), "platform doesn`t exsit. ");
           const account_statistics_object& plat_account_statistics = _remote_db->get_account_statistics_by_uid(platform_uid);
           advertising_create_operation create_op;
           create_op.platform = platform_uid;
           create_op.advertising_aid = platform_obj->last_advertising_sequence + 1;
           create_op.description = description;
           create_op.unit_price = asset_obj->amount_from_string(unit_price).amount;
           create_op.unit_time = unit_time;

           signed_transaction tx;
           tx.operations.push_back(create_op);
           set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
           tx.validate();

           return sign_transaction(tx, broadcast);
       } FC_CAPTURE_AND_RETHROW((platform)(description)(unit_price)(unit_time)(csaf_fee)(broadcast))
   }

   signed_transaction update_advertising(string                     platform,
                                         advertising_aid_type       advertising_aid,
                                         optional<string>           description,
                                         optional<string>           unit_price,
                                         optional<uint32_t>         unit_time,
                                         optional<bool>             on_sell,
                                         bool                       csaf_fee,
                                         bool                       broadcast)
   {
       try {
           FC_ASSERT(!self.is_locked(), "Should unlock first");
           fc::optional<asset_object_with_data> asset_obj = get_asset(GRAPHENE_CORE_ASSET_AID);
           FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", GRAPHENE_CORE_ASSET_AID));

           account_uid_type platform_uid = get_account_uid(platform);
           advertising_update_operation update_op;
           update_op.platform = platform_uid;
           update_op.advertising_aid = advertising_aid;
           if (description.valid())
               update_op.description = *description;
           if (unit_price.valid())
               update_op.unit_price = asset_obj->amount_from_string(*unit_price).amount;
           if (unit_time.valid())
               update_op.unit_time = *unit_time;
           if (on_sell.valid())
               update_op.on_sell = *on_sell;

           signed_transaction tx;
           tx.operations.push_back(update_op);
           set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
           tx.validate();

           return sign_transaction(tx, broadcast);
       } FC_CAPTURE_AND_RETHROW((platform)(advertising_aid)(description)(unit_price)(unit_time)(on_sell)(csaf_fee)(broadcast))
   }

   signed_transaction ransom_advertising(string           platform,
                                         string           from_account,
                                         advertising_aid_type        advertising_aid,
                                         advertising_order_oid_type  advertising_order_oid,
                                         bool             csaf_fee,
                                         bool             broadcast)
   {
       try {
           FC_ASSERT(!self.is_locked(), "Should unlock first");

           account_uid_type platform_uid = get_account_uid(platform);
           account_uid_type from_account_uid = get_account_uid(from_account);
           advertising_ransom_operation ransom_op;
           ransom_op.platform = platform_uid;
           ransom_op.from_account = from_account_uid;
           ransom_op.advertising_aid = advertising_aid;
           ransom_op.advertising_order_oid = advertising_order_oid;

           signed_transaction tx;
           tx.operations.push_back(ransom_op);
           set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
           tx.validate();

           return sign_transaction(tx, broadcast);
       } FC_CAPTURE_AND_RETHROW((platform)(from_account)(advertising_aid)(advertising_order_oid)(csaf_fee)(broadcast))
   }

   signed_transaction create_custom_vote(string           create_account,
                                         string           title,
                                         string           description,
                                         time_point_sec   expired_time,
                                         asset_aid_type   asset_id,
                                         share_type       required_amount,
                                         uint8_t          minimum_selected_items,
                                         uint8_t          maximum_selected_items,
                                         vector<string>   options,
                                         bool csaf_fee,
                                         bool broadcast)
   {
      try {
         FC_ASSERT(!self.is_locked(), "Should unlock first");

        
         account_uid_type creater = get_account_uid(create_account);
         const account_statistics_object& creater_statistics = _remote_db->get_account_statistics_by_uid(creater);

         custom_vote_create_operation create_op;
         create_op.custom_vote_creater = creater;
         create_op.vote_vid = creater_statistics.last_custom_vote_sequence + 1;
         create_op.title = title;
         create_op.description = description;
         create_op.vote_expired_time = expired_time;
         create_op.vote_asset_id = asset_id;
         create_op.required_asset_amount = required_amount;
         create_op.minimum_selected_items = minimum_selected_items;
         create_op.maximum_selected_items = maximum_selected_items;
         create_op.options = options;

         signed_transaction tx;
         tx.operations.push_back(create_op);
         set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
         tx.validate();

         return sign_transaction(tx, broadcast);
      } FC_CAPTURE_AND_RETHROW((create_account)(title)(description)(expired_time)(asset_id)
         (required_amount)(minimum_selected_items)(maximum_selected_items)(options)(csaf_fee)(broadcast))
   }

   signed_transaction cast_custom_vote(string                voter,
                                       string                custom_vote_creater,
                                       custom_vote_vid_type  custom_vote_vid,
                                       set<uint8_t>          vote_result,
                                       bool csaf_fee,
                                       bool broadcast)
   {
      try {
         FC_ASSERT(!self.is_locked(), "Should unlock first");

         account_uid_type cast_voter = get_account_uid(voter);
         account_uid_type creater = get_account_uid(custom_vote_creater);
         custom_vote_cast_operation vote_op;
         vote_op.voter = cast_voter;
         vote_op.custom_vote_creater = creater;
         vote_op.custom_vote_vid = custom_vote_vid;
         vote_op.vote_result = vote_result;

         signed_transaction tx;
         tx.operations.push_back(vote_op);
         set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees, csaf_fee);
         tx.validate();

         return sign_transaction(tx, broadcast);
      } FC_CAPTURE_AND_RETHROW((voter)(custom_vote_creater)(custom_vote_vid)(vote_result)(csaf_fee)(broadcast))
   }

   uint64_t get_account_auth_platform_count(string platform)
   {
      try {
         account_uid_type platform_uid = get_account_uid(platform);
         return _remote_db->get_account_auth_platform_count(platform_uid);
      } FC_CAPTURE_AND_RETHROW((platform))
   }

   vector<account_auth_platform_object> list_account_auth_platform_by_platform(string   platform,
                                                                               account_uid_type   lower_bound_account,
                                                                               uint32_t limit)
   {
       try {
           account_uid_type platform_uid = get_account_uid(platform);
           return _remote_db->list_account_auth_platform_by_platform(platform_uid, lower_bound_account, limit);
       } FC_CAPTURE_AND_RETHROW((platform)(lower_bound_account)(limit))
   }

   vector<account_auth_platform_object> list_account_auth_platform_by_account(string   account,
                                                                              account_uid_type   lower_bound_platform,
                                                                              uint32_t limit)
   {
       try {
           account_uid_type account_uid = get_account_uid(account);
           return _remote_db->list_account_auth_platform_by_account(account_uid, lower_bound_platform, limit);
       } FC_CAPTURE_AND_RETHROW((account)(lower_bound_platform)(limit))
   }

   signed_transaction approve_proposal(
      const string& fee_paying_account,
      const string& proposal_id,
      const approval_delta& delta,
      bool csaf_fee = true,
      bool broadcast = false)
   {
      proposal_update_operation update_op;

      update_op.fee_paying_account = get_account(fee_paying_account).uid;
      update_op.proposal = fc::variant(proposal_id).as<proposal_id_type>( 1 );
      // make sure the proposal exists
      get_object( update_op.proposal );

      for( const std::string& name : delta.secondary_approvals_to_add )
         update_op.secondary_approvals_to_add.insert( get_account( name ).uid );
      for( const std::string& name : delta.secondary_approvals_to_remove )
         update_op.secondary_approvals_to_remove.insert( get_account( name ).uid );
      for( const std::string& name : delta.active_approvals_to_add )
         update_op.active_approvals_to_add.insert( get_account( name ).uid );
      for( const std::string& name : delta.active_approvals_to_remove )
         update_op.active_approvals_to_remove.insert( get_account( name ).uid );
      for( const std::string& name : delta.owner_approvals_to_add )
         update_op.owner_approvals_to_add.insert( get_account( name ).uid );
      for( const std::string& name : delta.owner_approvals_to_remove )
         update_op.owner_approvals_to_remove.insert( get_account( name ).uid );
      for( const std::string& k : delta.key_approvals_to_add )
         update_op.key_approvals_to_add.insert( public_key_type( k ) );
      for( const std::string& k : delta.key_approvals_to_remove )
         update_op.key_approvals_to_remove.insert( public_key_type( k ) );

      signed_transaction tx;
      tx.operations.push_back(update_op);
      set_operation_fees(tx, get_global_properties().parameters.current_fees, csaf_fee);
      tx.validate();
      return sign_transaction(tx, broadcast);
   }

   void dbg_make_uia(string creator, string symbol)
   {
      asset_options opts;
      opts.flags &= ~(white_list);
      opts.issuer_permissions = opts.flags;
      create_asset(get_account(creator).name, symbol, 2, opts, {}, true);
   }

   void dbg_push_blocks( const std::string& src_filename, uint32_t count )
   {
      use_debug_api();
      (*_remote_debug)->debug_push_blocks( src_filename, count );
      (*_remote_debug)->debug_stream_json_objects_flush();
   }

   void dbg_generate_blocks( const std::string& debug_wif_key, uint32_t count )
   {
      use_debug_api();
      (*_remote_debug)->debug_generate_blocks( debug_wif_key, count );
      (*_remote_debug)->debug_stream_json_objects_flush();
   }

   void dbg_stream_json_objects( const std::string& filename )
   {
      use_debug_api();
      (*_remote_debug)->debug_stream_json_objects( filename );
      (*_remote_debug)->debug_stream_json_objects_flush();
   }

   void dbg_update_object( const fc::variant_object& update )
   {
      use_debug_api();
      (*_remote_debug)->debug_update_object( update );
      (*_remote_debug)->debug_stream_json_objects_flush();
   }

   void use_network_node_api()
   {
      if( _remote_net_node )
         return;
      try
      {
         _remote_net_node = _remote_api->network_node();
      }
      catch( const fc::exception& e )
      {
         std::cerr << "\nCouldn't get network node API.  You probably are not configured\n"
         "to access the network API on the yoyow_node you are\n"
         "connecting to.  Please follow the instructions in README.md to set up an apiaccess file.\n"
         "\n";
         throw(e);
      }
   }

   void use_debug_api()
   {
      if( _remote_debug )
         return;
      try
      {
        _remote_debug = _remote_api->debug();
      }
      catch( const fc::exception& e )
      {
         std::cerr << "\nCouldn't get debug node API.  You probably are not configured\n"
         "to access the debug API on the node you are connecting to.\n"
         "\n"
         "To fix this problem:\n"
         "- Please ensure you are running debug_node, not witness_node.\n"
         "- Please follow the instructions in README.md to set up an apiaccess file.\n"
         "\n";
      }
   }

   void network_add_nodes( const vector<string>& nodes )
   {
      use_network_node_api();
      for( const string& node_address : nodes )
      {
         (*_remote_net_node)->add_node( fc::ip::endpoint::from_string( node_address ) );
      }
   }

   vector< variant > network_get_connected_peers()
   {
      use_network_node_api();
      const auto peers = (*_remote_net_node)->get_connected_peers();
      vector< variant > result;
      result.reserve( peers.size() );
      for( const auto& peer : peers )
      {
         variant v;
         fc::to_variant( peer, v, GRAPHENE_MAX_NESTED_OBJECTS );
         result.push_back( v );
      }
      return result;
   }

   void flood_network(string prefix, uint32_t number_of_transactions)
   {
      try
      {
         const account_object& master = *_wallet.my_accounts.get<by_name>().lower_bound("import");
         int number_of_accounts = number_of_transactions / 3;
         number_of_transactions -= number_of_accounts;
         try {
            dbg_make_uia(master.name, "SHILL");
         } catch(...) {/* Ignore; the asset probably already exists.*/}

         fc::time_point start = fc::time_point::now();
         for( int i = 0; i < number_of_accounts; ++i )
         {
            std::ostringstream brain_key;
            brain_key << "brain key for account " << prefix << i;
            signed_transaction trx = create_account_with_brain_key(brain_key.str(), prefix + fc::to_string(i), master.name, master.name, /* broadcast = */ true, /* save wallet = */ false);
         }
         fc::time_point end = fc::time_point::now();
         ilog("Created ${n} accounts in ${time} milliseconds",
              ("n", number_of_accounts)("time", (end - start).count() / 1000));

         start = fc::time_point::now();
         for( int i = 0; i < number_of_accounts; ++i )
         {
            signed_transaction trx = transfer(master.name, prefix + fc::to_string(i), "10", "CORE", "", true);
            trx = transfer(master.name, prefix + fc::to_string(i), "1", "CORE", "", true);
         }
         end = fc::time_point::now();
         ilog("Transferred to ${n} accounts in ${time} milliseconds",
              ("n", number_of_accounts*2)("time", (end - start).count() / 1000));

         start = fc::time_point::now();
         for( int i = 0; i < number_of_accounts; ++i )
         {
            signed_transaction trx = issue_asset(prefix + fc::to_string(i), "1000", "SHILL", "", true);
         }
         end = fc::time_point::now();
         ilog("Issued to ${n} accounts in ${time} milliseconds",
              ("n", number_of_accounts)("time", (end - start).count() / 1000));
      }
      catch (...)
      {
         throw;
      }

   }

   operation get_prototype_operation( string operation_name )
   {
      auto it = _prototype_ops.find( operation_name );
      if( it == _prototype_ops.end() )
         FC_THROW("Unsupported operation: \"${operation_name}\"", ("operation_name", operation_name));
      return it->second;
   }

   string                  _wallet_filename;
   wallet_data             _wallet;

   map<public_key_type,string> _keys;
   fc::sha512                  _checksum;

   chain_id_type           _chain_id;
   fc::api<login_api>      _remote_api;
   fc::api<database_api>   _remote_db;
   fc::api<network_broadcast_api>   _remote_net_broadcast;
   fc::api<history_api>    _remote_hist;
   optional< fc::api<network_node_api> > _remote_net_node;
   optional< fc::api<graphene::debug_witness::debug_api> > _remote_debug;

   flat_map<string, operation> _prototype_ops;

   static_variant_map _operation_which_map = create_static_variant_map< operation >();

#ifdef __unix__
   mode_t                  _old_umask;
#endif
   const string _wallet_filename_extension = ".wallet";

};

std::string operation_printer::fee(const asset& a)const {
   out << "   (Fee: " << wallet.get_asset(a.asset_id).amount_to_pretty_string(a) << ")";
   return "";
}

BOOST_TTI_HAS_MEMBER_DATA(fee)

template<typename T>
const asset get_operation_total_fee(const T& op, std::true_type)
{
   return op.fee;
}
template<typename T>
const asset get_operation_total_fee(const T& op, std::false_type)
{
   return op.fee.total;
}
template<typename T>
const asset get_operation_total_fee(const T& op)
{
   const bool b = has_member_data_fee<T,asset>::value;
   return get_operation_total_fee( op, std::integral_constant<bool, b>() );
}

template<typename T>
std::string operation_printer::operator()(const T& op)const
{
   //balance_accumulator acc;
   //op.get_balance_delta( acc, result );
   asset op_fee = get_operation_total_fee(op);
   auto a = wallet.get_asset( op_fee.asset_id );
   // TODO: review
   //auto payer = wallet.get_account( op.fee_payer() );
   auto payer_uid = op.fee_payer_uid();

   string op_name = fc::get_typename<T>::name();
   if( op_name.find_last_of(':') != string::npos )
      op_name.erase(0, op_name.find_last_of(':')+1);
   out << op_name <<" ";
  // out << "balance delta: " << fc::json::to_string(acc.balance) <<"   ";
   //out << payer.name << " fee: " << a.amount_to_pretty_string( op.fee );
   out << payer_uid << " fee: " << a.amount_to_pretty_string( op_fee );
   operation_result_printer rprinter(wallet);
   std::string str_result = result.visit(rprinter);
   if( str_result != "" )
   {
      out << "   result: " << str_result;
   }
   return "";
}

string operation_printer::operator()(const transfer_operation& op) const
{
   out << "Transfer " << wallet.get_asset(op.amount.asset_id).amount_to_pretty_string(op.amount)
       << " from " << op.from << " to " << op.to;
   std::string memo;
   if( op.memo )
   {
      if( wallet.is_locked() )
      {
         out << " -- Unlock wallet to see memo.";
      } else {
         try {
            FC_ASSERT(wallet._keys.count(op.memo->to) || wallet._keys.count(op.memo->from), "Memo is encrypted to a key ${to} or ${from} not in this wallet.", ("to", op.memo->to)("from",op.memo->from));
            if( wallet._keys.count(op.memo->to) ) {
               auto my_key = wif_to_key(wallet._keys.at(op.memo->to));
               FC_ASSERT(my_key, "Unable to recover private key to decrypt memo. Wallet may be corrupted.");
               memo = op.memo->get_message(*my_key, op.memo->from);
               out << " -- Memo: " << memo;
            } else {
               auto my_key = wif_to_key(wallet._keys.at(op.memo->from));
               FC_ASSERT(my_key, "Unable to recover private key to decrypt memo. Wallet may be corrupted.");
               memo = op.memo->get_message(*my_key, op.memo->to);
               out << " -- Memo: " << memo;
            }
         } catch (const fc::exception& e) {
            out << " -- could not decrypt memo";
            //elog("Error when decrypting memo: ${e}", ("e", e.to_detail_string()));
         }
      }
   }
   fee(op.fee.total);
   return memo;
}

string operation_printer::operator()(const override_transfer_operation& op) const
{
   out << "Override-transfer " << wallet.get_asset(op.amount.asset_id).amount_to_pretty_string(op.amount)
       << " from " << op.from << " to " << op.to;
   std::string memo;
   if( op.memo )
   {
      if( wallet.is_locked() )
      {
         out << " -- Unlock wallet to see memo.";
      } else {
         try {
            FC_ASSERT(wallet._keys.count(op.memo->to) || wallet._keys.count(op.memo->from), "Memo is encrypted to a key ${to} or ${from} not in this wallet.", ("to", op.memo->to)("from",op.memo->from));
            if( wallet._keys.count(op.memo->to) ) {
               auto my_key = wif_to_key(wallet._keys.at(op.memo->to));
               FC_ASSERT(my_key, "Unable to recover private key to decrypt memo. Wallet may be corrupted.");
               memo = op.memo->get_message(*my_key, op.memo->from);
               out << " -- Memo: " << memo;
            } else {
               auto my_key = wif_to_key(wallet._keys.at(op.memo->from));
               FC_ASSERT(my_key, "Unable to recover private key to decrypt memo. Wallet may be corrupted.");
               memo = op.memo->get_message(*my_key, op.memo->to);
               out << " -- Memo: " << memo;
            }
         } catch (const fc::exception& e) {
            out << " -- could not decrypt memo";
            //elog("Error when decrypting memo: ${e}", ("e", e.to_detail_string()));
         }
      }
   }
   fee(op.fee.total);
   return memo;
}

std::string operation_printer::operator()(const account_create_operation& op) const
{
   out << "Create Account '" << op.name << "'";
   return fee(op.fee.total);
}

std::string operation_printer::operator()(const asset_create_operation& op) const
{
   out << "Create ";
   out << "Asset ";
   out << "'" << op.symbol << "' with issuer " << wallet.get_account(op.issuer).name;
   return fee(op.fee.total);
}

std::string operation_result_printer::operator()(const void_result& x) const
{
   return "";
}

std::string operation_result_printer::operator()(const object_id_type& oid)
{
   return std::string(oid);
}

std::string operation_result_printer::operator()(const asset& a)
{
   return _wallet.get_asset(a.asset_id).amount_to_pretty_string(a);
}

std::string operation_result_printer::operator()(const advertising_confirm_result& a)
{
    std::string str = "Return the deposit money: \n";
    for (auto iter : a)
    {
        str += "  account: " + to_string(iter.first) + " : " + to_string(iter.second.value) + "\n";
    }
    return str;
}

}}}

namespace graphene { namespace wallet {
   vector<brain_key_info> utility::derive_owner_keys_from_brain_key(string brain_key, int number_of_desired_keys)
   {
      // Safety-check
      FC_ASSERT( number_of_desired_keys >= 1 );

      // Create as many derived owner keys as requested
      vector<brain_key_info> results;
      brain_key = graphene::wallet::detail::normalize_brain_key(brain_key);
      for (int i = 0; i < number_of_desired_keys; ++i) {
        fc::ecc::private_key priv_key = graphene::wallet::detail::derive_private_key( brain_key, i );

        brain_key_info result;
        result.brain_priv_key = brain_key;
        result.wif_priv_key = key_to_wif( priv_key );
        result.pub_key = priv_key.get_public_key();

        results.push_back(result);
      }

      return results;
   }
}}

namespace graphene { namespace wallet {

wallet_api::wallet_api(const wallet_data& initial_data, fc::api<login_api> rapi)
   : my(new detail::wallet_api_impl(*this, initial_data, rapi))
{
}

wallet_api::~wallet_api()
{
}

bool wallet_api::copy_wallet_file(string destination_filename)
{
   return my->copy_wallet_file(destination_filename);
}

optional<signed_block_with_info> wallet_api::get_block(uint32_t num)
{
   return my->_remote_db->get_block(num);
}

uint64_t wallet_api::get_account_count() const
{
   return my->_remote_db->get_account_count();
}

vector<account_object> wallet_api::list_my_accounts_cached()
{
   // TODO this implementation has caching issue. To get latest data, check the steps in `load_wallet_file()`
   return vector<account_object>(my->_wallet.my_accounts.begin(), my->_wallet.my_accounts.end());
}

map<string,account_uid_type> wallet_api::list_accounts_by_name(const string& lowerbound, uint32_t limit)
{
   return my->_remote_db->lookup_accounts_by_name(lowerbound, limit);
}

vector<asset> wallet_api::list_account_balances(const string& account)
{
   return my->_remote_db->get_account_balances( get_account( account ).uid, flat_set<asset_aid_type>() );
}

vector<asset_object_with_data> wallet_api::list_assets(const string& lowerbound, uint32_t limit)const
{
   return my->_remote_db->list_assets( lowerbound, limit );
}

vector<operation_detail> wallet_api::get_relative_account_history(string account, optional<uint16_t> op_type, uint32_t stop, int limit, uint32_t start)const
{
   vector<operation_detail> result;

   account_uid_type uid = get_account( account ).uid;
   while( limit > 0 )
   {
      vector <pair<uint32_t,operation_history_object>> current = my->_remote_hist->get_relative_account_history(uid, op_type, stop, std::min<uint32_t>(100, limit), start);
      for (auto &p : current) {
         auto &o = p.second;
         std::stringstream ss;
         auto memo = o.op.visit(detail::operation_printer(ss, *my, o.result));
         result.push_back(operation_detail{memo, ss.str(), p.first, o});
      }
      if (current.size() < std::min<uint32_t>(100, limit))
         break;
      limit -= current.size();
      start = result.back().sequence - 1;
      if( start == 0 || start < stop ) break;
   }
   return result;
}

uint64_t wallet_api::calculate_account_uid(uint64_t n)const
{
   return calc_account_uid( n );
}

brain_key_info wallet_api::suggest_brain_key()const
{
   brain_key_info result;
   // create a private key for secure entropy
   fc::sha256 sha_entropy1 = fc::ecc::private_key::generate().get_secret();
   fc::sha256 sha_entropy2 = fc::ecc::private_key::generate().get_secret();
   fc::bigint entropy1( sha_entropy1.data(), sha_entropy1.data_size() );
   fc::bigint entropy2( sha_entropy2.data(), sha_entropy2.data_size() );
   fc::bigint entropy(entropy1);
   entropy <<= 8*sha_entropy1.data_size();
   entropy += entropy2;
   string brain_key = "";

   for( int i=0; i<BRAIN_KEY_WORD_COUNT; i++ )
   {
      fc::bigint choice = entropy % graphene::words::word_list_size;
      entropy /= graphene::words::word_list_size;
      if( i > 0 )
         brain_key += " ";
      brain_key += graphene::words::word_list[ choice.to_int64() ];
   }

   brain_key = normalize_brain_key(brain_key);
   fc::ecc::private_key priv_key = derive_private_key( brain_key, 0 );
   result.brain_priv_key = brain_key;
   result.wif_priv_key = key_to_wif( priv_key );
   result.pub_key = priv_key.get_public_key();
   return result;
}

vector<brain_key_info> wallet_api::derive_owner_keys_from_brain_key(string brain_key, int number_of_desired_keys) const
{
   return graphene::wallet::utility::derive_owner_keys_from_brain_key(brain_key, number_of_desired_keys);
}

bool wallet_api::is_public_key_registered(string public_key) const
{
   bool is_known = my->_remote_db->is_public_key_registered(public_key);
   return is_known;
}


string wallet_api::serialize_transaction( signed_transaction tx )const
{
   return fc::to_hex(fc::raw::pack(tx));
}

variant wallet_api::get_object( object_id_type id ) const
{
   return my->_remote_db->get_objects({id});
}

string wallet_api::get_wallet_filename() const
{
   return my->get_wallet_filename();
}

transaction_handle_type wallet_api::begin_builder_transaction()
{
   return my->begin_builder_transaction();
}

void wallet_api::add_operation_to_builder_transaction(transaction_handle_type transaction_handle, const operation& op)
{
   my->add_operation_to_builder_transaction(transaction_handle, op);
}

void wallet_api::replace_operation_in_builder_transaction(transaction_handle_type handle, unsigned operation_index, const operation& new_op)
{
   my->replace_operation_in_builder_transaction(handle, operation_index, new_op);
}

asset wallet_api::set_fees_on_builder_transaction(transaction_handle_type handle, string fee_asset)
{
   return my->set_fees_on_builder_transaction(handle, fee_asset);
}

transaction wallet_api::preview_builder_transaction(transaction_handle_type handle)
{
   return my->preview_builder_transaction(handle);
}

signed_transaction wallet_api::sign_builder_transaction(transaction_handle_type transaction_handle, bool broadcast)
{
   return my->sign_builder_transaction(transaction_handle, broadcast);
}

signed_transaction wallet_api::propose_builder_transaction(
   transaction_handle_type handle,
   string account_name_or_id,
   time_point_sec expiration,
   uint32_t review_period_seconds,
   bool broadcast)
{
   return my->propose_builder_transaction(handle, account_name_or_id, expiration, review_period_seconds, broadcast);
}

void wallet_api::remove_builder_transaction(transaction_handle_type handle)
{
   return my->remove_builder_transaction(handle);
}

account_object wallet_api::get_account(string account_name_or_id) const
{
   return my->get_account(account_name_or_id);
}

full_account wallet_api::get_full_account(string account_name_or_uid) const
{
   account_uid_type uid = my->get_account_uid( account_name_or_uid );
   vector<account_uid_type> uids( 1, uid );
   full_account_query_options opt = { true, true, true, true, true, true, true, true, true, true, true, true, true };
   const auto& results = my->_remote_db->get_full_accounts_by_uid( uids, opt );
   return results.at( uid );
}

asset_object_with_data wallet_api::get_asset(string asset_name_or_id) const
{
   auto a = my->find_asset(asset_name_or_id);
   FC_ASSERT( a, "Can not find asset ${a}", ("a", asset_name_or_id) );
   return *a;
}

asset_aid_type wallet_api::get_asset_aid(string asset_symbol_or_id) const
{
   return my->get_asset_aid(asset_symbol_or_id);
}

bool wallet_api::import_key(string account_name_or_id, string wif_key)
{
   FC_ASSERT(!is_locked(), "Should unlock first");
   // backup wallet
   fc::optional<fc::ecc::private_key> optional_private_key = wif_to_key(wif_key);
   if (!optional_private_key)
      FC_THROW("Invalid private key");
   //string shorthash = detail::address_to_shorthash(optional_private_key->get_public_key());
   //copy_wallet_file( "before-import-key-" + shorthash );

   bool result = my->import_key(account_name_or_id, wif_key);
   save_wallet_file();
   //copy_wallet_file( "after-import-key-" + shorthash );
   return result;
}

map<string, bool> wallet_api::import_accounts( string filename, string password )
{
   FC_ASSERT( !is_locked() );
   FC_ASSERT( fc::exists( filename ) );

   const auto imported_keys = fc::json::from_file<exported_keys>( filename );

   const auto password_hash = fc::sha512::hash( password );
   FC_ASSERT( fc::sha512::hash( password_hash ) == imported_keys.password_checksum );

   map<string, bool> result;
   for( const auto& item : imported_keys.account_keys )
   {
       const auto import_this_account = [ & ]() -> bool
       {
           try
           {
               const account_object account = get_account( item.account_name );
               const auto& owner_keys = account.owner.get_keys();
               const auto& active_keys = account.active.get_keys();

               for( const auto& public_key : item.public_keys )
               {
                   if( std::find( owner_keys.begin(), owner_keys.end(), public_key ) != owner_keys.end() )
                       return true;

                   if( std::find( active_keys.begin(), active_keys.end(), public_key ) != active_keys.end() )
                       return true;
               }
           }
           catch( ... )
           {
           }

           return false;
       };

       const auto should_proceed = import_this_account();
       result[ item.account_name ] = should_proceed;

       if( should_proceed )
       {
           uint32_t import_successes = 0;
           uint32_t import_failures = 0;
           // TODO: First check that all private keys match public keys
           for( const auto& encrypted_key : item.encrypted_private_keys )
           {
               try
               {
                  const auto plain_text = fc::aes_decrypt( password_hash, encrypted_key );
                  const auto private_key = fc::raw::unpack<private_key_type>( plain_text );

                  import_key( item.account_name, string( graphene::utilities::key_to_wif( private_key ) ) );
                  ++import_successes;
               }
               catch( const fc::exception& e )
               {
                  elog( "Couldn't import key due to exception ${e}", ("e", e.to_detail_string()) );
                  ++import_failures;
               }
           }
           ilog( "successfully imported ${n} keys for account ${name}", ("n", import_successes)("name", item.account_name) );
           if( import_failures > 0 )
              elog( "failed to import ${n} keys for account ${name}", ("n", import_failures)("name", item.account_name) );
       }
   }

   return result;
}

bool wallet_api::import_account_keys( string filename, string password, string src_account_name, string dest_account_name )
{
   FC_ASSERT( !is_locked() );
   FC_ASSERT( fc::exists( filename ) );

   bool is_my_account = false;
   const auto accounts = list_my_accounts_cached();
   for( const auto& account : accounts )
   {
       if( account.name == dest_account_name )
       {
           is_my_account = true;
           break;
       }
   }
   FC_ASSERT( is_my_account );

   const auto imported_keys = fc::json::from_file<exported_keys>( filename );

   const auto password_hash = fc::sha512::hash( password );
   FC_ASSERT( fc::sha512::hash( password_hash ) == imported_keys.password_checksum );

   bool found_account = false;
   for( const auto& item : imported_keys.account_keys )
   {
       if( item.account_name != src_account_name )
           continue;

       found_account = true;

       for( const auto& encrypted_key : item.encrypted_private_keys )
       {
           const auto plain_text = fc::aes_decrypt( password_hash, encrypted_key );
           const auto private_key = fc::raw::unpack<private_key_type>( plain_text );

           my->import_key( dest_account_name, string( graphene::utilities::key_to_wif( private_key ) ) );
       }

       return true;
   }
   save_wallet_file();

   FC_ASSERT( found_account );

   return false;
}

string wallet_api::normalize_brain_key(string s) const
{
   return detail::normalize_brain_key( s );
}

variant wallet_api::info()
{
   return my->info();
}

variant_object wallet_api::about() const
{
    return my->about();
}

fc::ecc::private_key wallet_api::derive_private_key(const std::string& prefix_string, int sequence_number) const
{
   return detail::derive_private_key( prefix_string, sequence_number );
}

signed_transaction wallet_api::register_account(string name,
                                                public_key_type owner_pubkey,
                                                public_key_type active_pubkey,
                                                string  registrar_account,
                                                string  referrer_account,
                                                uint32_t referrer_percent,
                                                uint32_t seed,
                                                bool csaf_fee,
                                                bool broadcast)
{
   return my->register_account(name, owner_pubkey, active_pubkey, registrar_account, referrer_account, referrer_percent, seed, csaf_fee, broadcast);
}
signed_transaction wallet_api::create_account_with_brain_key(string brain_key, string account_name,
                                                             string registrar_account, string referrer_account,
                                                             uint32_t id,
                                                             bool csaf_fee,
                                                             bool broadcast /* = false */)
{
   return my->create_account_with_brain_key(
            brain_key, account_name, registrar_account,
            referrer_account, id, csaf_fee, broadcast
            );
}
signed_transaction wallet_api::issue_asset(string to_account, string amount, string symbol,
                                           string memo, bool csaf_fee, bool broadcast)
{
   return my->issue_asset(to_account, amount, symbol, memo, csaf_fee, broadcast);
}

signed_transaction wallet_api::transfer(string from, string to, string amount,
                                        string asset_symbol, string memo, bool csaf_fee, bool broadcast /* = false */)
{
   return my->transfer(from, to, amount, asset_symbol, memo, csaf_fee, broadcast);
}

signed_transaction wallet_api::transfer_extension(string from, string to, string amount,
    string asset_symbol, string memo, bool isfrom_balance, bool isto_balance, bool csaf_fee, bool broadcast)
{
    return my->transfer_extension(from, to, amount, asset_symbol, memo, isfrom_balance, isto_balance, csaf_fee, broadcast);
}

signed_transaction wallet_api::override_transfer(string from, string to, string amount,
                                        string asset_symbol, string memo, bool csaf_fee, bool broadcast /* = false */)
{
   return my->override_transfer(from, to, amount, asset_symbol, memo, csaf_fee, broadcast);
}

signed_transaction wallet_api::create_asset(string issuer,
                                            string symbol,
                                            uint8_t precision,
                                            asset_options common,
                                            share_type initial_supply,
                                            bool csaf_fee,
                                            bool broadcast)

{
   return my->create_asset(issuer, symbol, precision, common, initial_supply, csaf_fee, broadcast);
}

signed_transaction wallet_api::update_asset(string symbol,
                                            optional<uint8_t> new_precision,
                                            asset_options new_options,
                                            bool csaf_fee,
                                            bool broadcast /* = false */)
{
   return my->update_asset(symbol, new_precision, new_options, csaf_fee, broadcast);
}

signed_transaction wallet_api::reserve_asset(string from,
                                          string amount,
                                          string symbol,
                                          bool csaf_fee,
                                          bool broadcast /* = false */)
{
   return my->reserve_asset(from, amount, symbol, csaf_fee, broadcast);
}

signed_transaction wallet_api::whitelist_account(string authorizing_account,
                                                 string account_to_list,
                                                 account_whitelist_operation::account_listing new_listing_status,
                                                 bool csaf_fee,
                                                 bool broadcast /* = false */)
{
   return my->whitelist_account(authorizing_account, account_to_list, new_listing_status, csaf_fee, broadcast);
}

signed_transaction wallet_api::create_committee_member(string owner_account,
                                              string pledge_amount,
                                              string pledge_asset_symbol,
                                              string url,
                                              bool csaf_fee,
                                              bool broadcast /* = false */)
{
   return my->create_committee_member(owner_account, pledge_amount, pledge_asset_symbol, url, csaf_fee, broadcast);
}

vector<witness_object> wallet_api::list_witnesses(const account_uid_type lowerbound, uint32_t limit,
                                                  data_sorting_type order_by)
{
   return my->_remote_db->lookup_witnesses(lowerbound, limit, order_by);
}

vector<committee_member_object> wallet_api::list_committee_members(const account_uid_type lowerbound, uint32_t limit,
                                                                   data_sorting_type order_by)
{
   return my->_remote_db->lookup_committee_members(lowerbound, limit, order_by);
}

vector<committee_proposal_object> wallet_api::list_committee_proposals()
{
   return my->_remote_db->list_committee_proposals();
}

witness_object wallet_api::get_witness(string owner_account)
{
   return my->get_witness(owner_account);
}

platform_object wallet_api::get_platform(string owner_account)
{
   return my->get_platform(owner_account);
}

vector<platform_object> wallet_api::list_platforms(const account_uid_type lowerbound, uint32_t limit,
                                                  data_sorting_type order_by)
{
   return my->_remote_db->lookup_platforms(lowerbound, limit, order_by);
}

uint64_t wallet_api::get_platform_count()
{
   return my->_remote_db->get_platform_count();
}

committee_member_object wallet_api::get_committee_member(string owner_account)
{
   return my->get_committee_member(owner_account);
}

signed_transaction wallet_api::create_witness(string owner_account,
                                              public_key_type block_signing_key,
                                              string pledge_amount,
                                              string pledge_asset_symbol,
                                              string url,
                                              bool csaf_fee,
                                              bool broadcast /* = false */)
{
   return my->create_witness_with_details(owner_account, block_signing_key, pledge_amount, pledge_asset_symbol, url, csaf_fee, broadcast);
   //return my->create_witness(owner_account, url, broadcast);
}

signed_transaction wallet_api::create_platform(string owner_account,
                                        string name,
                                        string pledge_amount,
                                        string pledge_asset_symbol,
                                        string url,
                                        string extra_data,
                                        bool csaf_fee,
                                        bool broadcast )
{
   return my->create_platform( owner_account, name, pledge_amount, pledge_asset_symbol, url, extra_data, csaf_fee, broadcast );
}

signed_transaction wallet_api::update_platform(string platform_account,
                                        optional<string> name,
                                        optional<string> pledge_amount,
                                        optional<string> pledge_asset_symbol,
                                        optional<string> url,
                                        optional<string> extra_data,
                                        bool csaf_fee,
                                        bool broadcast )

{
   return my->update_platform( platform_account, name, pledge_amount, pledge_asset_symbol, url, extra_data, csaf_fee, broadcast );
}

signed_transaction wallet_api::update_platform_votes(string voting_account,
                                          flat_set<string> platforms_to_add,
                                          flat_set<string> platforms_to_remove,
                                          bool csaf_fee,
                                          bool broadcast )
{
   return my->update_platform_votes( voting_account, platforms_to_add, platforms_to_remove, csaf_fee, broadcast );
}

signed_transaction wallet_api::account_auth_platform(string account, string platform_owner, string memo, string limit_for_platform, uint32_t permission_flags, bool csaf_fee, bool broadcast)
{
    return my->account_auth_platform(account, platform_owner, memo, limit_for_platform, permission_flags, csaf_fee, broadcast);
}

signed_transaction wallet_api::account_cancel_auth_platform(string account, string platform_owner, bool csaf_fee, bool broadcast)
{
   return my->account_cancel_auth_platform( account, platform_owner, csaf_fee, broadcast );
}

signed_transaction wallet_api::update_committee_member(
                                        string committee_member_account,
                                        optional<string> pledge_amount,
                                        optional<string> pledge_asset_symbol,
                                        optional<string> url,
                                        bool csaf_fee,
                                        bool broadcast /* = false */)
{
   return my->update_committee_member(committee_member_account, pledge_amount, pledge_asset_symbol, url, csaf_fee, broadcast);
}

signed_transaction wallet_api::update_witness(
                                        string witness_account,
                                        optional<public_key_type> block_signing_key,
                                        optional<string> pledge_amount,
                                        optional<string> pledge_asset_symbol,
                                        optional<string> url,
                                        bool csaf_fee,
                                        bool broadcast /* = false */)
{
   return my->update_witness_with_details(witness_account, block_signing_key, pledge_amount, pledge_asset_symbol, url, csaf_fee, broadcast);
   //return my->update_witness(witness_name, url, block_signing_key, broadcast);
}

signed_transaction wallet_api::collect_witness_pay(string witness_account,
                                        string pay_amount,
                                        string pay_asset_symbol,
                                        bool csaf_fee,
                                        bool broadcast /* = false */)
{
   return my->collect_witness_pay(witness_account, pay_amount, pay_asset_symbol, csaf_fee, broadcast);
}

signed_transaction wallet_api::collect_csaf(string from,
                                        string to,
                                        string amount,
                                        string asset_symbol,
                                        bool csaf_fee,
                                        bool broadcast /* = false */)
{
   time_point_sec time( time_point::now().sec_since_epoch() / 60 * 60 );
   return my->collect_csaf(from, to, amount, asset_symbol, time, csaf_fee, broadcast);
}

signed_transaction wallet_api::collect_csaf_with_time(string from,
                                        string to,
                                        string amount,
                                        string asset_symbol,
                                        time_point_sec time,
                                        bool csaf_fee,
                                        bool broadcast /* = false */)
{
   return my->collect_csaf(from, to, amount, asset_symbol, time, csaf_fee, broadcast);
}

signed_transaction wallet_api::update_witness_votes(string voting_account,
                                          flat_set<string> witnesses_to_add,
                                          flat_set<string> witnesses_to_remove,
                                          bool csaf_fee,
                                          bool broadcast /* = false */)
{
   return my->update_witness_votes( voting_account, witnesses_to_add, witnesses_to_remove, csaf_fee, broadcast );
}

signed_transaction wallet_api::update_committee_member_votes(string voting_account,
                                          flat_set<string> committee_members_to_add,
                                          flat_set<string> committee_members_to_remove,
                                          bool csaf_fee,
                                          bool broadcast /* = false */)
{
   return my->update_committee_member_votes( voting_account, committee_members_to_add, committee_members_to_remove, csaf_fee, broadcast );
}

signed_transaction wallet_api::set_voting_proxy(string account_to_modify,
                                                optional<string> voting_account,
                                                bool csaf_fee,
                                                bool broadcast /* = false */)
{
   return my->set_voting_proxy(account_to_modify, voting_account, csaf_fee, broadcast);
}

signed_transaction wallet_api::enable_allowed_assets(string account,
                                          bool enable,
                                          bool csaf_fee,
                                          bool broadcast /* = false */)
{
   return my->enable_allowed_assets( account, enable, csaf_fee, broadcast );
}

signed_transaction wallet_api::update_allowed_assets(string account,
                                          flat_set<string> assets_to_add,
                                          flat_set<string> assets_to_remove,
                                          bool csaf_fee,
                                          bool broadcast /* = false */)
{
   return my->update_allowed_assets( account, assets_to_add, assets_to_remove, csaf_fee, broadcast );
}

void wallet_api::set_wallet_filename(string wallet_filename)
{
   my->_wallet_filename = wallet_filename;
}

signed_transaction wallet_api::sign_transaction(signed_transaction tx, bool broadcast /* = false */)
{ try {
   return my->sign_transaction( tx, broadcast);
} FC_CAPTURE_AND_RETHROW( (tx) ) }

operation wallet_api::get_prototype_operation(string operation_name)
{
   return my->get_prototype_operation( operation_name );
}

void wallet_api::dbg_make_uia(string creator, string symbol)
{
   FC_ASSERT(!is_locked());
   my->dbg_make_uia(creator, symbol);
}

void wallet_api::dbg_push_blocks( std::string src_filename, uint32_t count )
{
   my->dbg_push_blocks( src_filename, count );
}

void wallet_api::dbg_generate_blocks( std::string debug_wif_key, uint32_t count )
{
   my->dbg_generate_blocks( debug_wif_key, count );
}

void wallet_api::dbg_stream_json_objects( const std::string& filename )
{
   my->dbg_stream_json_objects( filename );
}

void wallet_api::dbg_update_object( fc::variant_object update )
{
   my->dbg_update_object( update );
}

void wallet_api::network_add_nodes( const vector<string>& nodes )
{
   my->network_add_nodes( nodes );
}

vector< variant > wallet_api::network_get_connected_peers()
{
   return my->network_get_connected_peers();
}

void wallet_api::flood_network(string prefix, uint32_t number_of_transactions)
{
   FC_ASSERT(!is_locked());
   my->flood_network(prefix, number_of_transactions);
}

signed_transaction wallet_api::committee_proposal_create(
         const string committee_member_account,
         const vector<committee_proposal_item_type> items,
         const uint32_t voting_closing_block_num,
         optional<voting_opinion_type> proposer_opinion,
         const uint32_t execution_block_num,
         const uint32_t expiration_block_num,
         bool csaf_fee,
         bool broadcast
      )
{
   return my->committee_proposal_create( committee_member_account,
                                         items,
                                         voting_closing_block_num,
                                         proposer_opinion,
                                         execution_block_num,
                                         expiration_block_num,
                                         csaf_fee,
                                         broadcast );
}

signed_transaction wallet_api::committee_proposal_vote(
   const string committee_member_account,
   const uint64_t proposal_number,
   const voting_opinion_type opinion,
   bool csaf_fee,
   bool broadcast /* = false */
   )
{
   return my->committee_proposal_vote( committee_member_account, proposal_number, opinion, csaf_fee, broadcast );
}

signed_transaction wallet_api::proposal_create(const string              fee_paying_account,
                                               const vector<op_wrapper>  proposed_ops,
                                               time_point_sec            expiration_time,
                                               uint32_t                  review_period_seconds,
                                               bool                      csaf_fee,
                                               bool                      broadcast
                                               )
{
    return my->proposal_create(fee_paying_account, proposed_ops, expiration_time, review_period_seconds, csaf_fee, broadcast);
}

signed_transaction wallet_api::proposal_update(const string                      fee_paying_account,
                                               proposal_id_type                  proposal,
                                               const flat_set<account_uid_type>  secondary_approvals_to_add,
                                               const flat_set<account_uid_type>  secondary_approvals_to_remove,
                                               const flat_set<account_uid_type>  active_approvals_to_add,
                                               const flat_set<account_uid_type>  active_approvals_to_remove,
                                               const flat_set<account_uid_type>  owner_approvals_to_add,
                                               const flat_set<account_uid_type>  owner_approvals_to_remove,
                                               const flat_set<public_key_type>   key_approvals_to_add,
                                               const flat_set<public_key_type>   key_approvals_to_remove,
                                               bool                              csaf_fee,
                                               bool                              broadcast
                                               )
{
    return my->proposal_update(fee_paying_account, proposal, secondary_approvals_to_add, secondary_approvals_to_remove, active_approvals_to_add,
        active_approvals_to_remove, owner_approvals_to_add, owner_approvals_to_remove, key_approvals_to_add, key_approvals_to_remove, csaf_fee, broadcast);
}

signed_transaction wallet_api::proposal_delete(const string              fee_paying_account,
                                               proposal_id_type          proposal,
                                               bool                      csaf_fee,
                                               bool                      broadcast
                                               )
{
    return my->proposal_delete(fee_paying_account, proposal, csaf_fee, broadcast);
}

signed_transaction wallet_api::score_a_post(string         from_account,
                                            string         platform,
                                            string         poster,
                                            post_pid_type  post_pid,
                                            int8_t         score,
                                            string         csaf,
                                            bool           csaf_fee,
                                            bool           broadcast)
{
    return my->score_a_post(from_account, platform, poster, post_pid, score, csaf, csaf_fee, broadcast);
}

signed_transaction wallet_api::reward_post(string           from_account,
                                           string           platform,
                                           string           poster,
                                           post_pid_type    post_pid,
                                           string           amount,
                                           string           asset_symbol,
                                           bool             csaf_fee,
                                           bool             broadcast)
{
    return my->reward_post(from_account, platform, poster, post_pid, amount, asset_symbol, csaf_fee, broadcast);
}

signed_transaction wallet_api::reward_post_proxy_by_platform(string           from_account,
                                                             string           platform,
                                                             string           poster,
                                                             post_pid_type    post_pid,
                                                             string           amount,
                                                             bool             csaf_fee,
                                                             bool             broadcast)
{
    return my->reward_post_proxy_by_platform(from_account, platform, poster, post_pid, amount, csaf_fee, broadcast);
}

signed_transaction wallet_api::buyout_post(string           from_account,
                                           string           platform,
                                           string           poster,
                                           post_pid_type    post_pid,
                                           string           receiptor_account,
                                           bool             csaf_fee,
                                           bool             broadcast)
{
    return my->buyout_post(from_account, platform, poster, post_pid, receiptor_account, csaf_fee, broadcast);
}

signed_transaction wallet_api::create_license(string           platform,
                                              uint8_t          license_type,
                                              string           hash_value,
                                              string           title,
                                              string           body,
                                              string           extra_data,
                                              bool             csaf_fee,
                                              bool             broadcast)
{
    return my->create_license(platform, license_type, hash_value, title, body, extra_data, csaf_fee, broadcast);
}

signed_transaction wallet_api::create_post(string              platform,
                                           string              poster,
                                           string              hash_value,
                                           string              title,
                                           string              body,
                                           string              extra_data,
                                           string              origin_platform,
                                           string              origin_poster,
                                           string              origin_post_pid,
                                           post_create_ext     ext,
                                           bool                csaf_fee,
                                           bool                broadcast)
{
    return my->create_post(platform, poster, hash_value, title, body, extra_data, origin_platform, origin_poster, origin_post_pid, ext, csaf_fee, broadcast);
}

signed_transaction wallet_api::update_post(string                     platform,
                                           string                     poster,
                                           string                     post_pid,
                                           string                     hash_value,
                                           string                     title,
                                           string                     body,
                                           string                     extra_data,
                                           post_update_ext            ext,
                                           bool                       csaf_fee,
                                           bool                       broadcast)
{
    return my->update_post(platform, poster, post_pid, hash_value, title, body, extra_data, ext, csaf_fee, broadcast);
}

signed_transaction wallet_api::account_manage(string executor,
                                              string account,
                                              account_manage_operation::opt options,
                                              bool csaf_fee,
                                              bool broadcast
                                              )
{
    return my->account_manage(executor,account, options, csaf_fee, broadcast);
}

signed_transaction wallet_api::buy_advertising(string               account,
                                               string               platform,
                                               advertising_aid_type advertising_aid,
                                               uint32_t             start_time,
                                               uint32_t             buy_number,
                                               string               extra_data,
                                               string               memo,
                                               bool                 csaf_fee,
                                               bool                 broadcast)
{
    return my->buy_advertising(account, platform, advertising_aid, start_time, buy_number, extra_data, memo, csaf_fee, broadcast);
}

signed_transaction wallet_api::confirm_advertising(string         platform,
                                                   advertising_aid_type         advertising_aid,
                                                   advertising_order_oid_type   advertising_order_oid,
                                                   bool           comfirm,
                                                   bool           csaf_fee,
                                                   bool           broadcast)
{
    return my->confirm_advertising(platform, advertising_aid, advertising_order_oid, comfirm, csaf_fee, broadcast);
}

post_object wallet_api::get_post(string platform_owner,
                                 string poster_uid,
                                 string post_pid)
{
    return my->get_post(platform_owner, poster_uid, post_pid);
}

vector<post_object> wallet_api::get_posts_by_platform_poster(string           platform_owner,
                                                             optional<string> poster,
                                                             uint32_t         begin_time_range,
                                                             uint32_t         end_time_range,
                                                             object_id_type   lower_bound_post,
                                                             uint32_t         limit)
{
    time_point_sec  begin_time(begin_time_range);
    time_point_sec  end_time(end_time_range);
    return my->get_posts_by_platform_poster(platform_owner, poster, begin_time, end_time, lower_bound_post, limit);
}

score_object wallet_api::get_score(string platform,
                                   string poster_uid,
                                   string post_pid,
                                   string from_account)
{
    return my->get_score(platform, poster_uid, post_pid, from_account);
}

vector<score_object> wallet_api::get_scores_by_uid(string   scorer,
                                       uint32_t period,
                                       object_id_type lower_bound_score,
                                       uint32_t limit)
{
   return my->get_scores_by_uid(scorer, period, lower_bound_score, limit);
}

vector<score_object> wallet_api::list_scores(string platform,
                                             string poster_uid,
                                             string post_pid,
                                             object_id_type lower_bound_score,
                                             uint32_t       limit,
                                             bool           list_cur_period)
{
    return my->list_scores(platform, poster_uid, post_pid, lower_bound_score, limit, list_cur_period);
}

license_object wallet_api::get_license(string platform,
                                       string license_lid)
{
    return my->get_license(platform, license_lid);
}

vector<license_object> wallet_api::list_licenses(string platform, object_id_type lower_bound_license, uint32_t limit)
{
    return my->list_licenses(platform, lower_bound_license, limit);
}

vector<advertising_object> wallet_api::list_advertisings(string platform, string lower_bound_advertising, uint32_t limit)
{
    return my->list_advertisings(platform, lower_bound_advertising, limit);
}

vector<active_post_object> wallet_api::get_post_profits_detail(uint32_t         begin_period,
                                                               uint32_t         end_period,
                                                               string           platform,
                                                               string           poster,
                                                               string           post_pid)
{
    return my->get_post_profits_detail(begin_period, end_period, platform, poster, post_pid);
}

vector<Platform_Period_Profit_Detail> wallet_api::get_platform_profits_detail(uint32_t         begin_period,
                                                                              uint32_t         end_period,
                                                                              string           platform)
{
    return my->get_platform_profits_detail(begin_period, end_period, platform);
}

vector<Poster_Period_Profit_Detail> wallet_api::get_poster_profits_detail(uint32_t         begin_period,
                                                                          uint32_t         end_period,
                                                                          string           poster)
{
    return my->get_poster_profits_detail(begin_period, end_period, poster);
}

share_type wallet_api::get_score_profit(string account, uint32_t period)
{
    return my->get_score_profit(account, period);
}

account_statistics_object wallet_api::get_account_statistics(string account)
{
    return my->get_account_statistics(account);
}

signed_transaction wallet_api::create_advertising(string           platform,
                                                  string           description,
                                                  string           unit_price,
                                                  uint32_t         unit_time,
                                                  bool             csaf_fee,
                                                  bool             broadcast)
{
    return my->create_advertising(platform, description, unit_price, unit_time, csaf_fee, broadcast);
}

signed_transaction wallet_api::update_advertising(string                     platform,
                                                  advertising_aid_type       advertising_aid,
                                                  optional<string>           description,
                                                  optional<string>           unit_price,
                                                  optional<uint32_t>         unit_time,
                                                  optional<bool>             on_sell,
                                                  bool                       csaf_fee,
                                                  bool                       broadcast)
{
    return my->update_advertising(platform, advertising_aid, description, unit_price, unit_time, on_sell, csaf_fee, broadcast);
}

signed_transaction wallet_api::ransom_advertising(string           platform,
                                                  string           from_account,
                                                  advertising_aid_type        advertising_aid,
                                                  advertising_order_oid_type  advertising_order_oid,
                                                  bool             csaf_fee,
                                                  bool             broadcast)
{
    return my->ransom_advertising(platform, from_account, advertising_aid, advertising_order_oid, csaf_fee, broadcast);
}

vector<advertising_order_object> wallet_api::list_advertising_orders_by_purchaser(string purchaser, object_id_type lower_bound_advertising_order, uint32_t limit)
{
   account_uid_type account = my->get_account_uid(purchaser);
   return my->_remote_db->list_advertising_orders_by_purchaser(account, lower_bound_advertising_order, limit);
}

vector<advertising_order_object> wallet_api::list_advertising_orders_by_ads_aid(string platform, string advertising_aid, string lower_bound_advertising_order, uint32_t limit)
{
    account_uid_type platform_uid = my->get_account_uid(platform);
    advertising_aid_type ad_aid = fc::to_uint64(fc::string(advertising_aid));
    advertising_order_oid_type lower_order_oid = fc::to_uint64(fc::string(lower_bound_advertising_order));
    return my->_remote_db->list_advertising_orders_by_ads_aid(platform_uid, ad_aid, lower_order_oid, limit);
}

signed_transaction wallet_api::create_custom_vote(string           create_account,
                                                  string           title,
                                                  string           description,
                                                  uint32_t         expired_time,
                                                  asset_aid_type   asset_id,
                                                  share_type       required_amount,
                                                  uint8_t          minimum_selected_items,
                                                  uint8_t          maximum_selected_items,
                                                  vector<string>   options,
                                                  bool csaf_fee,
                                                  bool broadcast)
{
   time_point_sec time = time_point_sec(expired_time);
   return my->create_custom_vote(create_account, title, description, time, asset_id,
      required_amount, minimum_selected_items, maximum_selected_items, options, csaf_fee, broadcast);
}

signed_transaction wallet_api::cast_custom_vote(string                voter,
                                                string                custom_vote_creater,
                                                custom_vote_vid_type  custom_vote_vid,
                                                set<uint8_t>          vote_result,
                                                bool csaf_fee,
                                                bool broadcast)
{
   return my->cast_custom_vote(voter, custom_vote_creater, custom_vote_vid, vote_result, csaf_fee, broadcast);
}

vector<custom_vote_object> wallet_api::list_custom_votes(const account_uid_type lowerbound, uint32_t limit)
{
    return my->_remote_db->list_custom_votes(lowerbound, limit);
}

vector<custom_vote_object> wallet_api::lookup_custom_votes(string creater, custom_vote_vid_type lower_bound_custom_vote, uint32_t limit)
{
   account_uid_type account = my->get_account_uid(creater);
   return my->_remote_db->lookup_custom_votes(account, lower_bound_custom_vote, limit);
}

vector<cast_custom_vote_object> wallet_api::list_cast_custom_votes_by_id(const string creater,
                                                                         const custom_vote_vid_type vote_vid,
                                                                         const object_id_type lower_bound_cast_custom_vote,
                                                                         uint32_t limit)
{
   account_uid_type creater_account = my->get_account_uid(creater);
   return my->_remote_db->list_cast_custom_votes_by_id(creater_account, vote_vid, lower_bound_cast_custom_vote, limit);
}

vector<cast_custom_vote_object> wallet_api::list_cast_custom_votes_by_voter(string voter, object_id_type lower_bound_cast_custom_vote, uint32_t limit)
{
   account_uid_type account = my->get_account_uid(voter);
   return my->_remote_db->list_cast_custom_votes_by_voter(account, lower_bound_cast_custom_vote, limit);
}

uint64_t wallet_api::get_account_auth_platform_count(string platform)
{
   return my->get_account_auth_platform_count(platform);
}

vector<account_auth_platform_object> wallet_api::list_account_auth_platform_by_platform(string   platform,
                                                                                        account_uid_type   lower_bound_account,
                                                                                        uint32_t limit)
{
    return my->list_account_auth_platform_by_platform(platform, lower_bound_account, limit);
}

vector<account_auth_platform_object> wallet_api::list_account_auth_platform_by_account(string   account,
                                                                                       account_uid_type   lower_bound_platform,
                                                                                       uint32_t limit)
{
    return my->list_account_auth_platform_by_account(account, lower_bound_platform, limit);
}

signed_transaction wallet_api::approve_proposal(
   const string& fee_paying_account,
   const string& proposal_id,
   const approval_delta& delta,
   bool  csaf_fee,
   bool broadcast /* = false */
   )
{
   return my->approve_proposal( fee_paying_account, proposal_id, delta, csaf_fee, broadcast );
}

vector<proposal_object> wallet_api::list_proposals( string account_name_or_id )
{
   auto acc = my->get_account( account_name_or_id );
   //return my->_remote_db->get_proposed_transactions( acc.uid );
   return {};
}

global_property_object wallet_api::get_global_properties() const
{
   return my->get_global_properties();
}

content_parameter_extension_type wallet_api::get_global_properties_extensions() const
{
   return my->get_global_properties_extensions();
}

dynamic_global_property_object wallet_api::get_dynamic_global_properties() const
{
   return my->get_dynamic_global_properties();
}

string wallet_api::help()const
{
   std::vector<std::string> method_names = my->method_documentation.get_method_names();
   std::stringstream ss;
   for (const std::string method_name : method_names)
   {
      try
      {
         ss << my->method_documentation.get_brief_description(method_name);
      }
      catch (const fc::key_not_found_exception&)
      {
         ss << method_name << " (no help available)\n";
      }
   }
   return ss.str();
}

string wallet_api::gethelp(const string& method)const
{
   fc::api<wallet_api> tmp;
   std::stringstream ss;
   ss << "\n";

   // doxygen help string first
   try
   {
      string brief_desc = my->method_documentation.get_brief_description(method);
      boost::trim( brief_desc );
      ss << brief_desc << "\n\n";
      std::string doxygenHelpString = my->method_documentation.get_detailed_description(method);
      if (!doxygenHelpString.empty())
         ss << doxygenHelpString << "\n";
      else
         ss << "No doxygen help defined for method " << method << "\n\n";
   }
   catch (const fc::key_not_found_exception&)
   {
      ss << "No doxygen help defined for method " << method << "\n\n";
   }

   if( method == "import_key" )
   {
      ss << "usage: import_key ACCOUNT_NAME_OR_ID  WIF_PRIVATE_KEY\n\n";
      ss << "example: import_key \"1.3.11\" 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3\n";
      ss << "example: import_key \"usera\" 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3\n";
   }
   else if( method == "transfer" )
   {
      ss << "usage: transfer FROM TO AMOUNT SYMBOL \"memo\" BROADCAST\n\n";
      ss << "example: transfer \"1.3.11\" \"1.3.4\" 1000.03 CORE \"memo\" true\n";
      ss << "example: transfer \"usera\" \"userb\" 1000.123 CORE \"memo\" true\n";
   }
   else if( method == "create_account_with_brain_key" )
   {
      ss << "usage: create_account_with_brain_key BRAIN_KEY ACCOUNT_NAME REGISTRAR REFERRER BROADCAST\n\n";
      ss << "example: create_account_with_brain_key \"my really long brain key\" \"newaccount\" \"1.3.11\" \"1.3.11\" true\n";
      ss << "example: create_account_with_brain_key \"my really long brain key\" \"newaccount\" \"someaccount\" \"otheraccount\" true\n";
      ss << "\n";
      ss << "This method should be used if you would like the wallet to generate new keys derived from the brain key.\n";
      ss << "The BRAIN_KEY will be used as the owner key, and the active key will be derived from the BRAIN_KEY.  Use\n";
      ss << "register_account if you already know the keys you know the public keys that you would like to register.\n";
   }
   else if( method == "register_account" )
   {
      ss << "usage: register_account ACCOUNT_NAME OWNER_PUBLIC_KEY ACTIVE_PUBLIC_KEY REGISTRAR REFERRER REFERRER_PERCENT BROADCAST\n\n";
      ss << "example: register_account \"newaccount\" \"CORE6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV\" \"CORE6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV\" \"1.3.11\" \"1.3.11\" 50 true\n";
      ss << "\n";
      ss << "Use this method to register an account for which you do not know the private keys.";
   }
   else if( method == "create_asset" )
   {
      ss << "usage: ISSUER SYMBOL PRECISION_DIGITS OPTIONS INITIAL_SUPPLY BROADCAST\n\n";
      ss << "PRECISION_DIGITS: the number of digits after the decimal point\n\n";
      ss << "Example value of OPTIONS: \n";
      ss << fc::json::to_pretty_string( graphene::chain::asset_options() );
   }
   else if( method == "committee_proposal_create" )
   {
      ss << "usage: COMMITTEE_MEMBER_UID PROPOSED_ITEMS BLOCK_NUM PROPOSER_OPINION BLOCK_NUM BLOCK_NUM BROADCAST\n\n";
      ss << "Example value of PROPOSED_ITEMS: \n";
      ss << "item[0].new_priviledges:\n\n";
      graphene::chain::committee_update_account_priviledge_item_type::account_priviledge_update_options apuo;
      apuo.can_vote = true;
      apuo.is_admin = true;
      apuo.is_registrar = true;
      apuo.takeover_registrar = 25638;
      ss << fc::json::to_pretty_string( apuo );
      ss << "\n\nitem[1].parameters:\n\n";
      ss << fc::json::to_pretty_string( fee_schedule::get_default().parameters );
      ss << "\n\nitem[2]:\n\n";
      ss << "see graphene::chain::committee_updatable_parameters or Calling \"get_global_properties\" to see";
      ss << "\n\n";
      ss << "[[0,{\"account\":28182,\"new_priviledges\": {\"can_vote\":true}}],[1,{\"parameters\": ";
      ss << "[[16,{\"fee\":10000,\"min_real_fee\":0,\"min_rf_percent\":0}]]}],[2,{\"governance_voting_expiration_blocks\":150000}]]";
      ss << "\n\n";
   }
   return ss.str();
}

bool wallet_api::load_wallet_file( string wallet_filename )
{
   return my->load_wallet_file( wallet_filename );
}

void wallet_api::save_wallet_file( string wallet_filename )
{
   my->save_wallet_file( wallet_filename );
}

std::map<string,std::function<string(fc::variant,const fc::variants&)> >
wallet_api::get_result_formatters() const
{
   return my->get_result_formatters();
}

bool wallet_api::is_locked()const
{
   return my->is_locked();
}
bool wallet_api::is_new()const
{
   return my->_wallet.cipher_keys.size() == 0;
}

void wallet_api::encrypt_keys()
{
   my->encrypt_keys();
}

void wallet_api::lock()
{ try {
   if ( is_locked() )
      return;
   encrypt_keys();
   for( auto key : my->_keys )
      key.second = key_to_wif(fc::ecc::private_key());
   my->_keys.clear();
   my->_checksum = fc::sha512();
   my->self.lock_changed(true);
} FC_CAPTURE_AND_RETHROW() }

void wallet_api::unlock(string password)
{ try {
   FC_ASSERT( !is_new(), "Please use the set_password method to initialize a new wallet before continuing" );
   FC_ASSERT( is_locked(), "The wallet is already unlocked" );
   FC_ASSERT(password.size() > 0);
   auto pw = fc::sha512::hash(password.c_str(), password.size());
   vector<char> decrypted = fc::aes_decrypt(pw, my->_wallet.cipher_keys);
   auto pk = fc::raw::unpack<plain_keys>(decrypted);
   FC_ASSERT(pk.checksum == pw);
   my->_keys = std::move(pk.keys);
   my->_checksum = pk.checksum;
   my->self.lock_changed(false);
} FC_CAPTURE_AND_RETHROW() }

void wallet_api::set_password( string password )
{
   if( !is_new() )
      FC_ASSERT( !is_locked(), "The wallet must be unlocked before the password can be set" );
   my->_checksum = fc::sha512::hash( password.c_str(), password.size() );
   lock();
}

map<public_key_type, string> wallet_api::dump_private_keys()
{
   FC_ASSERT( !is_locked(), "Should unlock first" );
   return my->_keys;
}

string wallet_api::get_key_label( public_key_type key )const
{
   auto key_itr   = my->_wallet.labeled_keys.get<by_key>().find(key);
   if( key_itr != my->_wallet.labeled_keys.get<by_key>().end() )
      return key_itr->label;
   return string();
}

string wallet_api::get_private_key( public_key_type pubkey )const
{
   return key_to_wif( my->get_private_key( pubkey ) );
}

public_key_type  wallet_api::get_public_key( string label )const
{
   try { return fc::variant(label).as<public_key_type>( 1 ); } catch ( ... ){}

   auto key_itr   = my->_wallet.labeled_keys.get<by_label>().find(label);
   if( key_itr != my->_wallet.labeled_keys.get<by_label>().end() )
      return key_itr->key;
   return public_key_type();
}

bool               wallet_api::set_key_label( public_key_type key, string label )
{
   auto result = my->_wallet.labeled_keys.insert( key_label{label,key} );
   if( result.second  ) return true;

   auto key_itr   = my->_wallet.labeled_keys.get<by_key>().find(key);
   auto label_itr = my->_wallet.labeled_keys.get<by_label>().find(label);
   if( label_itr == my->_wallet.labeled_keys.get<by_label>().end() )
   {
      if( key_itr != my->_wallet.labeled_keys.get<by_key>().end() )
         return my->_wallet.labeled_keys.get<by_key>().modify( key_itr, [&]( key_label& obj ){ obj.label = label; } );
   }
   return false;
}

} } // graphene::wallet

namespace fc {

      void /*fc::*/to_variant(const account_multi_index_type& accts, fc::variant& vo, uint32_t max_depth)
      {
         to_variant( std::vector<account_object>(accts.begin(), accts.end()), vo, max_depth );
      }

      void /*fc::*/from_variant(const fc::variant& var, account_multi_index_type& vo, uint32_t max_depth)
      {
         const std::vector<account_object>& v = var.as<std::vector<account_object>>( max_depth );
         vo = account_multi_index_type(v.begin(), v.end());
      }
}
