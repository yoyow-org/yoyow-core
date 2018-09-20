#pragma once

#include <iostream>
using namespace std;

/*
* Sign a transaction with "WIF" and "chain_id"
*/
string signature( const string& tx, const string& wif, const string& chain_id);

/*
* Generate a private key with "brain_key"
*/
string generate_key(const string& brain_key, int sequence_number = 0);

/*
* Generate a public key with a private key
*/
string private_to_public(const string& wif);

/**
 * Generate an unsigned transfer transaction json string，For offline signature
 * @param last_irreversible_block_id
 * @param last_irreversible_block_time
 * @param from the name or id of the account sending the funds
 * @param to the name or id of the account receiving the funds
 * @param amount the amount to send (in nominal units -- to send half of an asset, specify 0.5)
 * @param memo a memo to attach to the transaction.  The memo will be encrypted in the
 *             transaction and readable for the receiver.  There is no length limit
 *             other than the limit imposed by maximum transaction size, but transaction fee
 *             increase with transaction size
 * @param current_fees_json a fee_schedule json obj.
 * @param from_memo_private_wif
 * @param to_memo_public_key
 * @param expiration default 30s
 * @param asset_id the asset id, default 0
 * @returns the unsigned transaction transferring funds
 */ 
string generate_transaction(const string& last_irreversible_block_id, 
                            const string& last_irreversible_block_time,
                            const string& from, 
                            const string& to, 
                            const string& amount, 
                            const string& memo,
                            const string& from_memo_private_wif,
                            const string& to_memo_public_key,
                            const string& current_fees_json,
                            const int64_t expiration = 30,
                            const u_int64_t asset_id = 0
                            );

/**
 * decrypt memo msg
 * @param memo_json the memo obj json
 * @param memo_private_wif the my memo_key wif
 */
string decrypt_memo(const string& memo_json, const string& memo_private_wif );