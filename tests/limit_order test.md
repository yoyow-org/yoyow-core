#                          limit_order test

## limit order 测试链启动

服务器地址：47.99.52.183

```
./yoyow_node --rpc-endpoint=0.0.0.0:9999 --p2p-endpoint=0.0.0.0:10000
```

## 导入init账户private key

```
import_key init 5JbLA2gCHs7T9XhrCpcMJoJk3Y8EA41Y6to87GuQwMHWoJ9ZHYH
```

## 注册test账户

```
register_account test YYW5KWYNAJYFf7Y9qQBoTr2E1gzKzBBKtBZA2uC1WTnBenG8JsNBT YYW5KWYNAJYFf7Y9qQBoTr2E1gzKzBBKtBZA2uC1WTnBenG8JsNBT init init 0 121 true true
{
  "ref_block_num": 46779,
  "ref_block_prefix": 3048687327,
  "expiration": "2019-05-09T03:19:21",
  "operations": [[
      1,{
        "fee": {
          "total": {
            "amount": 0,
            "asset_id": 0
          }
        },
        "uid": 31036,
        "name": "test",
        "owner": {
          "weight_threshold": 1,
          "account_uid_auths": [],
          "key_auths": [[
              "YYW5KWYNAJYFf7Y9qQBoTr2E1gzKzBBKtBZA2uC1WTnBenG8JsNBT",
              1
            ]
          ]
        },
        "active": {
          "weight_threshold": 1,
          "account_uid_auths": [],
          "key_auths": [[
              "YYW5KWYNAJYFf7Y9qQBoTr2E1gzKzBBKtBZA2uC1WTnBenG8JsNBT",
              1
            ]
          ]
        },
        "secondary": {
          "weight_threshold": 1,
          "account_uid_auths": [],
          "key_auths": [[
              "YYW5KWYNAJYFf7Y9qQBoTr2E1gzKzBBKtBZA2uC1WTnBenG8JsNBT",
              1
            ]
          ]
        },
        "memo_key": "YYW5KWYNAJYFf7Y9qQBoTr2E1gzKzBBKtBZA2uC1WTnBenG8JsNBT",
        "reg_info": {
          "registrar": 25638,
          "referrer": 25638,
          "registrar_percent": 0,
          "referrer_percent": 0,
          "allowance_per_article": {
            "amount": 0,
            "asset_id": 0
          },
          "max_share_per_article": {
            "amount": 0,
            "asset_id": 0
          },
          "max_share_total": {
            "amount": 0,
            "asset_id": 0
          },
          "buyout_percent": 10000
        }
      }
    ]
  ],
  "signatures": [
    "203a8a5c04b18c257c23943a6a007f6ce607ebf8883ee34a59553cb516b05be6e9751272e3ee613c5843aebad5e13a0336d870c0263bf70b4f3314e6ec0e0f2bdd"
  ]
}


```

## 给test账户转账100000YOYO

```
transfer init test 100000 YOYO "" true true
{
  "ref_block_num": 46940,
  "ref_block_prefix": 2398107580,
  "expiration": "2019-05-09T03:28:54",
  "operations": [[
      0,{
        "fee": {
          "total": {
            "amount": 20000,
            "asset_id": 0
          },
          "options": {
            "from_csaf": {
              "amount": 20000,
              "asset_id": 0
            }
          }
        },
        "from": 25638,
        "to": 31036,
        "amount": {
          "amount": "10000000000",
          "asset_id": 0
        }
      }
    ]
  ],
  "signatures": [
    "1f6afdd1ce3f40d27fb20b271e116622339955cd8d53d005718341b2a8a74114f17ef159f42f39f99a423ad5a3c1de7cee4c5b0e00e5062ab14dd136b7e70ddbec"
  ]
}
```

## init账户收集积分过渡给test账户用于支付操作手续费

```
transfer init test 100000 YOYO "" true true
{
  "ref_block_num": 47036,
  "ref_block_prefix": 2063073271,
  "expiration": "2019-05-09T03:33:42",
  "operations": [[
      0,{
        "fee": {
          "total": {
            "amount": 20000,
            "asset_id": 0
          },
          "options": {
            "from_csaf": {
              "amount": 20000,
              "asset_id": 0
            }
          }
        },
        "from": 25638,
        "to": 31036,
        "amount": {
          "amount": "10000000000",
          "asset_id": 0
        }
      }
    ]
  ],
  "signatures": [
    "207b72846d1b5d5be120896cf48e86611dd3dec355fd20719e43d1df89525b049f0fb198596459ced89fafa157996987297e4d28459a54031497a975bccda5d3e3"
  ]
}
```

## init账户创建ABCD资产

```
create_asset init  ABCD 5 {max_supply : 100000000000000,market_fee_percent : 0,max_market_fee : 0,issuer_permissions : 15,flags : 0,whitelist_authorities : [],blacklist_authorities : [],whitelist_markets : [],blacklist_markets : [],description : "create test asset",extensions : [] }  10000000000000 true true
{
  "ref_block_num": 47094,
  "ref_block_prefix": 3009089405,
  "expiration": "2019-05-09T03:36:36",
  "operations": [[
      25,{
        "fee": {
          "total": {
            "amount": 2000001757,
            "asset_id": 0
          }
        },
        "issuer": 25638,
        "symbol": "ABCD",
        "precision": 5,
        "common_options": {
          "max_supply": "100000000000000",
          "market_fee_percent": 0,
          "max_market_fee": 0,
          "issuer_permissions": 15,
          "flags": 0,
          "whitelist_authorities": [],
          "blacklist_authorities": [],
          "whitelist_markets": [],
          "blacklist_markets": [],
          "description": "create test asset",
          "extensions": {}
        },
        "extensions": {
          "initial_supply": "10000000000000"
        }
      }
    ]
  ],
  "signatures": [
    "201f7a8076756c27fc2badff9772ad6b6f46918e78163626c187f6c3e51191e8057c2cd50d36876a2c7ae5c85a08c2ce4bb123145167129d7af8265ecc7bcb145f"
  ]
}
```

## 查看当前资产详情

```
list_assets 0 10
[{
    "id": "1.3.1",
    "asset_id": 1,
    "symbol": "ABC",
    "precision": 5,
    "issuer": 25638,
    "options": {
      "max_supply": "100000000000000",
      "market_fee_percent": 0,
      "max_market_fee": 0,
      "issuer_permissions": 15,
      "flags": 0,
      "whitelist_authorities": [],
      "blacklist_authorities": [],
      "whitelist_markets": [],
      "blacklist_markets": [],
      "description": "create test asset",
      "extensions": {}
    },
    "dynamic_asset_data_id": "2.2.1",
    "dynamic_asset_data": {
      "id": "2.2.1",
      "asset_id": 1,
      "current_supply": "10000000000000",
      "accumulated_fees": 0
    }
  },{
    "id": "1.3.2",
    "asset_id": 2,
    "symbol": "ABCD",
    "precision": 5,
    "issuer": 25638,
    "options": {
      "max_supply": "100000000000000",
      "market_fee_percent": 0,
      "max_market_fee": 0,
      "issuer_permissions": 15,
      "flags": 0,
      "whitelist_authorities": [],
      "blacklist_authorities": [],
      "whitelist_markets": [],
      "blacklist_markets": [],
      "description": "create test asset",
      "extensions": {}
    },
    "dynamic_asset_data_id": "2.2.2",
    "dynamic_asset_data": {
      "id": "2.2.2",
      "asset_id": 2,
      "current_supply": "10000000000000",
      "accumulated_fees": 0
    }
  },{
    "id": "1.3.0",
    "asset_id": 0,
    "symbol": "YOYO",
    "precision": 5,
    "issuer": 1264,
    "options": {
      "max_supply": "200000000000000",
      "market_fee_percent": 0,
      "max_market_fee": "1000000000000000",
      "issuer_permissions": 0,
      "flags": 0,
      "whitelist_authorities": [],
      "blacklist_authorities": [],
      "whitelist_markets": [],
      "blacklist_markets": [],
      "description": ""
    },
    "dynamic_asset_data_id": "2.2.0",
    "dynamic_asset_data": {
      "id": "2.2.0",
      "asset_id": 0,
      "current_supply": "100351563541332",
      "accumulated_fees": 0
    }
  }
]
```



## 导入test私钥到钱包

```
import_key test 5JDJdcFrx6HEbQ9etbMuhQ5EMwmqTGBNYAAVQLWjtc5wLaZBiAf
```

## 限价订单交易之前账户明细：

```
list_account_balances init
999649999.96486 YOYO
100000000 ABC
100000000 ABCD
```

```
list_account_balances test
199979.90000 YOYO
```



## 创建限价订单

```
create_limit_order test 0 10000 2 5000 1569862861 false true
{
  "ref_block_num": 52348,
  "ref_block_prefix": 2384405294,
  "expiration": "2019-05-09T07:59:18",
  "operations": [[
      49,{
        "fee": {
          "total": {
            "amount": 2000000,
            "asset_id": 0
          }
        },
        "seller": 31036,
        "amount_to_sell": {
          "amount": 10000,
          "asset_id": 0
        },
        "min_to_receive": {
          "amount": 5000,
          "asset_id": 2
        },
        "expiration": "2019-09-30T17:01:01",
        "fill_or_kill": false
      }
    ]
  ],
  "signatures": [
    "205ef5e36d2ba59c487a8a30d728a7b2ebad9090f996f01bb4678a4258e0c3ca3a7d589902dabadf32ec04094dd29d37a3dc97529ae087182e93c5ce2db2dc9ffb"
  ]
}
```



```
create_limit_order init 2 5000 0 10000 1569862861 false true
{
  "ref_block_num": 52385,
  "ref_block_prefix": 56799869,
  "expiration": "2019-05-09T08:01:09",
  "operations": [[
      49,{
        "fee": {
          "total": {
            "amount": 2000000,
            "asset_id": 0
          }
        },
        "seller": 25638,
        "amount_to_sell": {
          "amount": 5000,
          "asset_id": 2
        },
        "min_to_receive": {
          "amount": 10000,
          "asset_id": 0
        },
        "expiration": "2019-09-30T17:01:01",
        "fill_or_kill": false
      }
    ]
  ],
  "signatures": [
    "1f10234f5e6807c554205369be9ac8ec0b861999d3ead8579dd27768132189b3a53264aeda735ac007c9b9e8c8dea747850862801aaa699cb5ed20dd9c4fc3b0ec"
  ]
}
```



## 限价订单交易之后账户明细：

```
list_account_balances init
999649980.06486 YOYO
100000000 ABC
99999999.95000 ABCD
```

```
list_account_balances test
199959.80000 YOYO
0.05000 ABCD
```

## 结果分析：

#### init账户订单交易之前：

999649999.96486 YOYO

100000000 ABCD

#### init账户订单交易之后：

999649980.06486 YOYO

99999999.95000 ABCD

#### test账户订单交易之前：

199979.90000 YOYO

#### test账户订单交易之后：

199959.80000 YOYO
0.05000 ABCD

##### init账户YOYO资产先扣除20YOYO创建订单手续费，后增加订单资金0.1YOYO，ABCD资产减少0.05ABCD，结果正确；test账户YOYO资产先扣除20YOYO创建订单手续费，后减少订单资金0.1YOYO，ABCD资产增加0.05ABCD，结果正确



