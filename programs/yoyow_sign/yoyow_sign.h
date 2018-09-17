#pragma once

#include <iostream>
using namespace std;

string signature(string tx, string wif, string chain_id);

string generate_key(string brain_key, int sequence_number = 0);

string private_to_public(string wif);