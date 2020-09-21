English

# daemon

## Introduction
Daemon is used to monitor whether the target program is running in Linux. When the target program exits abnormally, the daemon starts the target program.

## Module Structure
Process name | Implementation file | Function
|:--- | --- | ---
| `CEGd` | [main.cpp](./CEG/main.cpp) | Daemon, used in Linux. The absolute path of the target program needs to be passed to the boot process. The daemon and the target program share the same memory. The target program periodically writes the timestamp, and the daemon reads the timestamp. If the target program does not write the timestamp for a long time, it indicates the target program is closed. The daemon will start the target program. For the target program writing the timestamp method, refer to the file [daemon.h](../common/daemon.h).
