# yoyow-core

## Getting Started
We recommend building on Ubuntu 16.04 LTS (64-bit)

building on other system :
    **NOTE:** Yoyow requires an [OpenSSL](https://www.openssl.org/) version in the 1.0.x series. OpenSSL 1.1.0 and newer are NOT supported. If your system OpenSSL version is newer, then you will need to manually provide an older version of OpenSSL and specify it to CMake using `-DOPENSSL_INCLUDE_DIR`, `-DOPENSSL_SSL_LIBRARY`, and `-DOPENSSL_CRYPTO_LIBRARY`.
    
    **NOTE:** Yoyow requires a [Boost](http://www.boost.org/) version in the range [1.57, 1.60]. Versions earlier than
    1.57 or newer than 1.60 are NOT supported. If your system Boost version is newer, then you will need to manually build
    an older version of Boost and specify it to CMake using `DBOOST_ROOT`.

### Build Dependencies:
```
sudo apt-get update
sudo apt-get install autoconf cmake make automake libtool git libboost-all-dev libssl-dev g++ libcurl4-openssl-dev
```

### Build Script:
```
git clone https://github.com/yoyow-org/yoyow-core.git
cd yoyow-core
git checkout yy-mainnet # may substitute "yy-mainnet" with current release tag
git submodule update --init --recursive
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ../
make yoyow_node
make yoyow_client
```

### launch:
```
./programs/yoyow_node/yoyow_node
```
The node will automatically create a data directory including a config file. It may take several hours to fully synchronize the blockchain. After syncing, you can exit the node using Ctrl+C and setup the command-line wallet by editing ```witness_node_data_dir/config.ini``` as follows:
```
rpc-endpoint = 127.0.0.1:9000
```
After starting the witness node again, in a separate terminal you can run:
```
./programs/yoyow_client/yoyow_client
```
Set your inital password:
```
>>> set_password <PASSWORD>
>>> unlock <PASSWORD>
```

## Docs
* https://github.com/yoyow-org/yoyow-core/wiki

## More Info
* https://yoyow.org/
* https://wallet.yoyow.org/
