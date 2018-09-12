# yoyow-core

## Getting Started
We recommend building on Ubuntu 16.04 LTS (64-bit)

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
