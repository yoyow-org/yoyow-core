#pragma once

#include <iostream>
using namespace std;

/*
* Sign a transaction with "WIF" and "chain_id"
*/
string signature(string tx, string wif, string chain_id);

/*
* Generate a private key with "brain_key"
*/
string generate_key(string brain_key, int sequence_number = 0);

/*
* Generate a public key with a private key
*/
string private_to_public(string wif);

/*
* Generate a transaction with an irreversible block ID and an irreversible block time
*/
string base_transaction(string last_irreversible_block_id, string last_irreversible_block_time);