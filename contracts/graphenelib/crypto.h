#pragma once
#include <graphenelib/types.h>
extern "C" {
/**
 *  This method is implemented as:
 * 
 *  public_key_type pk;
 *  datastream<const char *> pubds(pub, publen);
 *  fc::raw::unpack(pubds, pk);
 *  auto check = public_key_type(fc::ecc::public_key(sig, digest, true));
 *  graphene_assert(check == pk, "Error expected key different than recovered key");
 *
 *  This method is optimized to a NO-OP when in fast evaluation mode
 */
void assert_recover_key(const checksum256 *digest,const signature *sig,
                              const char *pub, uint32_t publen);
/*
 *  This method is deprecated, assert_recover_key is more efficient and robust
 */
bool verify_signature(const char *data, uint32_t datalen, const signature* sig,  const char *pub_key, uint32_t pub_keylen);

/**
 *  This method is implemented as:
 *
 *  checksum calc_hash;
 *  sha256( data, length, &calc_hash );
 *  graphene_assert( calc_hash == hash, "invalid hash" );
 *
 *  This method is optimized to a NO-OP when in fast evaluation mode
 */
void assert_sha256(const char *data, uint32_t length, const checksum256 *hash);

/**
 *  This method is implemented as:
 *
 *  checksum calc_hash;
 *  sha1( data, length, &calc_hash );
 *  graphene_assert( calc_hash == hash, "invalid hash" );
 *
 *  This method is optimized to a NO-OP when in fast evaluation mode
 */
void assert_sha1(const char *data, uint32_t length, const checksum160 *hash);

/**
 *  This method is implemented as:
 *
 *  checksum calc_hash;
 *  sha512( data, length, &calc_hash );
 *  graphene_assert( calc_hash == hash, "invalid hash" );
 *
 *  This method is optimized to a NO-OP when in fast evaluation mode
 */
void assert_sha512(const char *data, uint32_t length, const checksum512 *hash);

/**
 *  This method is implemented as:
 *
 *  checksum calc_hash;
 *  ripemd160( data, length, &calc_hash );
 *  graphene_assert( calc_hash == hash, "invalid hash" );
 *
 *  This method is optimized to a NO-OP when in fast evaluation mode
 */
void assert_ripemd160(const char *data, uint32_t length, const checksum160 *hash);

/**
 *  Calculates sha256( data,length) and stores result in memory pointed to by hash
 *  `hash` should be checksum<256>
 */
void sha256(const char *data, uint32_t length, checksum256 *hash);

/**
 *  Calculates sha1( data,length) and stores result in memory pointed to by hash
 *  `hash` should be checksum<160>
 */
void sha1(const char *data, uint32_t length, checksum160 *hash);

/**
 *  Calculates sha512( data,length) and stores result in memory pointed to by hash
 *  `hash` should be checksum<512>
 */
void sha512(const char *data, uint32_t length, checksum512 *hash);

/**
 *  Calculates ripemd160( data,length) and stores result in memory pointed to by hash
 *  `hash` should be checksum<160>
 */
void ripemd160(const char *data, uint32_t length, checksum160 *hash);
}
