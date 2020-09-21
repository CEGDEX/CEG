English

# main

## Introduction

Main program and CEG process entry. Load configuration file after startup, start main thread, initialize all modules and start timer.

## Module Structure
Name | Implementation file | Function
|:--- | --- | ---
| `main` | [main.cpp](./main.cpp) | Main program and process entry. Refer to [Start process](## Start process) section.
| `Configure` | [configure.h](./configure.h) | It parses the main program's configuration file, inherited from the `ConfigureBase` class, and implements loading of all module configuration files such as database, log, WebServer, WebSocketServer, P2P, ledger, creation block, monitoring and so on.


## Startup Workflow

After the main program is started, the main process is as follows:

- Parse the parameters. If the parameter contains the instruction, the corresponding operation is performed. Refer to [argument.h](../common/argument.h).
- Initialize all modules, such as `net`, `Timer`, `Configure`, `Storage`, `Global`, `SlowTimer`, `Logger`, `Console`, `PeerManager`, `LedgerManager`, `ConsensusManager`, `GlueManager`, ` WebSocketServer`, `WebServer`, `MonitorManager`, `ContractManager`, etc.
- If the program is a Linux version, the `Daemon` module is started to write a timestamp to the shared memory for use by the daemon.
- Set itself as the main thread and start timer scheduling.
