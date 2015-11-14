Distributed File Sharing System
======================
Project description can be found inside Documents/

Compiling
----------
```
make
```

Usage
--------
```
./dfs <mode> <port_number>
```
mode parameter is either s (or) c

s indicates server

c indicates client

Commands
--------

    HELP    - Displays information about the available user command options.
    CREATOR - Displays the author's full name, UBIT name and UB email address.
    DISPLAY - Display the IP address of this process, and the port on which this process is listening for incoming connections.
    CONNECT <destination> <port no> - Connects to <destination> at port <port no>
    REGISTER <server IP> <port no> - Registers with server at given IP Address and port
    TERMINATE <connection id> - Terminates the connection with <connection id> begotten using list command
    EXIT - Shutdown
