English 

# Monitor

## Introduction
The module implements monitoring blockchain node status, node information and abnormal alarm functions, as follows:
- Block chain node status, including the online and offline status of the node.
- Node information, including system information, block information, transaction information, user information.
- Abnormal alarm. It provides real-time alerts when node status or node system status is found to be abnormal, including offline alarms, CPU alarms, memory alarms, disk alarms, and block asynchronous alarms.
## Module Structure

Class name | Statement file | Function
|:--- | --- | ---
| `MonitorManager` | [monitor_manager.h](./monitor_manager.h) | The manager of the monitoring module, inherited from the `Network` class. This class has three functions: one is to manage the connection of the node; the other is to process the received message, including the handshake message, the registration message, the node status message, the block status message, the node system message and the heartbeat information; the third is to provide monitoring reconnection and the timer for abnormality detection.
|`Monitor`|  [monitor.h](./monitor.h) | The monitoring status of the node is saved, including monitoring the session, whether it is online, monitoring the ID, and monitoring the version information of the node.
|`SystemManager`|  [system_manager.h](../common/system_manager.h)  |The administrator of the node system information. It provides CPU timer and gets system monitoring information.

## Protocol Definition
For monitoring message, use the Google protocol buffer definition. Refer to file [monitor.proto](../proto/monitor.proto). The types are listed below:
```
MONITOR_MSGTYPE_HELLO  #Handshake message and send the monitoring ID
MONITOR_MSGTYPE_REGISTER  #Register message and verify the monitoring ID
MONITOR_MSGTYPE_CEG   #Get the node information
MONITOR_MSGTYPE_LEDGER    #Get the block information
MONITOR_MSGTYPE_SYSTEM   #Get the node system information
MONITOR_MSGTYPE_ALERT #Alert message
```
