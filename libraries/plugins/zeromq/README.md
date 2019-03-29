# ZeroMQ Plugin

Exports operation history data into a zeromq socket.

## Installation

You need cppzmq to run this plugin: https://github.com/zeromq/cppzmq

Follow the build instructions here: https://github.com/zeromq/cppzmq#build-instructions

You need to install `libzmq` and `cppzmq` as the instructions states.

## Comamnd line options

- `zeromq-socket`: Socket address("tcp://127.0.0.1:5556)

## Usage

There is a demo python socket client, start the client with:

`python libraries/plugins/zeromq/client_sample.py`

You should see the following message: "Connecting to server..."

Now in a new window start the bitshares core with the zeromq plugin:

`./programs/witness_node/witness_node --plugins "zeromq`

After the node create the config, add the seeds, etc you should start to see operation history in the client python side:

```
...
{"account_history":{"id":"2.9.2593","account":"1.2.90713","operation_id":"1.11.2469","sequence":92,"next":"2.9.0"},"operation_history":{"trx_in_block":0,"op_in_trx":0,"operation_results":[1,"1.2.91781"],"virtual_op":33969,"op":[5,{"fee":{"amount":15332,"asset_id":"1.3.0"},"registrar":"1.2.90713","referrer":"1.2.90713","referrer_percent":0,"name":"cryptomaniac69","owner":{"weight_threshold":1,"account_auths":[],"key_auths":[["BTS7V4wbDecXP18psrWxGX9SsF7sPsQuzXaUURmYcDzi3tuhqD1db",1]],"address_auths":[]},"active":{"weight_threshold":1,"account_auths":[],"key_auths":[["BTS8fqHpY7menAEfHJrxsyrakKZycsmRrH9E4LKwVkcPH3PzHLQoi",1]],"address_auths":[]},"options":{"memo_key":"BTS8fqHpY7menAEfHJrxsyrakKZycsmRrH9E4LKwVkcPH3PzHLQoi","voting_account":"1.2.5","num_witness":0,"num_committee":0,"votes":[],"extensions":[]},"extensions":{}}]},"operation_type":5,"operation_id_num":2469,"block_data":{"block_num":7367,"block_time":"2015-10-13T20:55:54","trx_id":"b81f37b678971906b049f3ab112c1bb9e723f1c7"}}
{"account_history":{"id":"2.9.2594","account":"1.2.91781","operation_id":"1.11.2469","sequence":1,"next":"2.9.0"},"operation_history":{"trx_in_block":0,"op_in_trx":0,"operation_results":[1,"1.2.91781"],"virtual_op":33969,"op":[5,{"fee":{"amount":15332,"asset_id":"1.3.0"},"registrar":"1.2.90713","referrer":"1.2.90713","referrer_percent":0,"name":"cryptomaniac69","owner":{"weight_threshold":1,"account_auths":[],"key_auths":[["BTS7V4wbDecXP18psrWxGX9SsF7sPsQuzXaUURmYcDzi3tuhqD1db",1]],"address_auths":[]},"active":{"weight_threshold":1,"account_auths":[],"key_auths":[["BTS8fqHpY7menAEfHJrxsyrakKZycsmRrH9E4LKwVkcPH3PzHLQoi",1]],"address_auths":[]},"options":{"memo_key":"BTS8fqHpY7menAEfHJrxsyrakKZycsmRrH9E4LKwVkcPH3PzHLQoi","voting_account":"1.2.5","num_witness":0,"num_committee":0,"votes":[],"extensions":[]},"extensions":{}}]},"operation_type":5,"operation_id_num":2469,"block_data":{"block_num":7367,"block_time":"2015-10-13T20:55:54","trx_id":"b81f37b678971906b049f3ab112c1bb9e723f1c7"}}
...
```