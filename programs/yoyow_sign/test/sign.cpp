#include "../yoyow_sign.h"
#include <iostream>
#include <stdio.h>

using namespace std;

int main()
{
   string tx = "{\"ref_block_num\": 0,\"ref_block_prefix\": 0,\"expiration\": \"1970-01-01T00:00:00\",\"operations\": [[0, {\"fee\": {\"total\": {\"amount\": 20000,\"asset_id\": 0},\"options\": {\"from_csaf\": {\"amount\": 20000,\"asset_id\": 0}}},\"from\": 25997,\"to\": 26264,\"amount\": {\"amount\": 100000,\"asset_id\": 0}}]],\"signatures\": []}";
   string wif = "5JuTEwvFqnhjWRtvdUhTxVDhZBG71QgiRTmRV1B5Ph4jFmWBM7F";
   string chain = "ae4f234c75199f67e526c9478cf499dd6e94c2b66830ee5c58d0868a3179baf6";

   string result = signature( tx, wif, chain );
   printf("%s\n", result.c_str() );

   string pvkey = generate_key(chain);
   printf("%s\n", pvkey.c_str());

   printf("%s\n", private_to_public(pvkey).c_str());

   return 0;
}