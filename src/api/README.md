English

# API Interface Module

## Introduction
The module implements a variety of interface service functions, as follows:
- HTTP service. There are two ways for this service: one is the API interface, through which you can interact with the node blockchain account, such as querying account information, querying transactions, submitting transactions, calling contracts, etc.; second, it provides access to HTTP pages.
- WebSocket interface service. It provides the ability to initiate transactions and subscribe to transactions.
- Console interface. It provides users with the console mode to operate Wallet, get CEG value, pay CEG and other operations.

## Module Structure

Class name | Statement file | Function
|:--- | --- | ---
| `WebServer` | [web_server.h](./web_server.h) | Provider of HTTP service. Use `http::server::server` to provide HTTP service (refer to [server.hpp](../3rd/http/server.hpp)). It implements routing in the `WebServer` class to invoke the HTTP interface and access HTTP pages.
| `WebSocketServer` | [websocket_server.h](./websocket_server.h) | Provider of Web Socket service. This class extends from `Network` class (refer to [network.h](../common/network.h)).`Network` uses `asio::io_service` asynchronous IO to listen for network events and manage all network connections. The function of `WebSocketServer` is as follows: the originating transaction and the transaction subscription service are provided to the external node; broadcast transaction interfaces are provided for other internal modules for notifying the subscriber of the related transactions.
| `Console` | [console.h](./console.h) | Provider of the console command service. It has a separate thread execution environment, listens to input operations after startup, and performs operations according to instructions. During the execution it will call other modules such as `KeyStore` (refer to [key_store.h](../common/key_store.h))，or `GlueManager` (refer to [glue_manager.h](../glue/glue_manager.h)) etc.

## Interface List

The interfaces implemented in the code include but are not limited to the following:
### HTTP
```
createKeyPair   #Create public-private key pairs for debugging use only, not recommended for use in production environments
getAccount  #Get the account information
getAccountBase  #Get basic account information
getGenesisAccount   #Get the genesis account
getAccountMetaData  #Get the metadata value of the account
getAccountAssets    #Get the assets of the account
getTransactionBlob  #Convert a transaction string to the binary value format
getTransactionHistory   #Get historical transactions
getTransactionCache     #Get the transaction pool cache record
getStatus   #Get the number of transactions and the number of accounts
getLedger   #Get the block information
getModulesStatus    #Get the module information
getConsensusInfo    #Get consensus messages
updateLogLevel      #Set the log level
getAddress          #Get address information
getTransactionFromBlob  #Get transaction information from binary objects
getPeerAddresses    #Get the node information
getLedgerValidators #Get the verification node information
submitTransaction   #Initiate a transaction
callContract        #Call contracts
testTransaction     #Detect transactions
```
### WebSocket
```
protocol::CHAIN_SUBMITTRANSACTION  #Initiate a transaction
protocol::CHAIN_SUBSCRIBE_TX    #Subscribe transactions
```
Protocol definition file [overlay.proto](../proto/overlay.proto)

### Console
```
createWallet     #Create a wallet
openWallet      #Open the wallet
closeWallet     #Close the wallet
restoreWallet   #Reset the wallet
getBalance      #Get the CEG balance
getBlockNumber  #Get the last block information
help            #Help
payCoin     #Pay CEG coins
showKey     #Show the wallet private key
getState    #Get the node status
exit   #Exit the console
```

