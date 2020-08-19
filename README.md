# Implementation within TUN/TAP of the Priority Switching Scheduler (PSS)

Following A. Finzi, E. Lochin, A. Mifdaoui, F. Frances, Improving RFC5865 Core Network Scheduling with a Burst Limiting Shaper. IEEE Global Communications Conference (GLOBECOM), 4 December 2017 - 8 December 2017 (Singapore). Available <http://oatao.univ-toulouse.fr/18448/>, if you use this code please cite this paper.

## 1. Copyright Information

Copyright (C) 2018  
Victor Perrier <victor.perrier@student.isae-supaero.fr> and
Emmanuel Lochin <emmanuel.lochin@isae-supaero.fr>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or any 
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>. 

## 2. Installation 

Create a bin directory with `mkdir bin` and then `make`. You can edit the Makefile and compile with the following options:

- `-DDEBUG` to add debug messages concerning the sending and the receiving 
   of TUN/TAP packets;
- `-DDUMP_STATS` to dump the credit evolution vs time in a file;
- `-DDSCP` to classify packets following a DSCP markng with EF: 1, AF: 2 else DF.
   The classification is done inside `src/bls.c` and `src/pscheduler.c` (seek for DSCP 
   inside these files). Another option is to operate a classification on the packet 
   port number (usefull if you do not wish to enable a DSCP marker). In that case
   compile without `-DDSCP`. By default, classification is done using destination 
   port EF: 20000, AF: 20001 else DF. These values can be changed inside both files.

## 3. Usage 

As soon as compiled, launch `bin/pss -h` for usage (see below):

```
./bin/pss -i <ifacename> [-s|-c <serverIP>] [-p <port>]
./bin/pss -h

-i <ifacename>: Name of interface to use (mandatory)
-s|-c <serverIP>: run in server mode (-s), or specify server address (-c <serverIP>) (mandatory)
-p <port>: port to listen on (if run in server mode) or to connect to (in client mode), default 30001
-h: prints this help text
-C: Capacity of the link in bit/s
-b: BW parameter for the AF class
-m: Lm parameter for the AF class
-r: Lr parameter for the AF class
```

## 4. Practical usecase with TUN/TAP

You first need to create a `tun0` interface on the client:
```
sudo openvpn --mktun --dev tun0
sudo ip link set tun0 up
sudo ip addr add 192.168.10.1/24 dev tun0
```
Then same operation on the serveur side with a different IP adress for the tunnel interface:
```
sudo openvpn --mktun --dev tun0
sudo ip link set tun0 up
sudo ip addr add 192.168.10.2/24 dev tun0
```
First launch `bin/pss -i tun0 -s` on the server side and `bin/pss -i tun0 -c W.X.Y.Z -C your_capacity_value -b your_bw_value -m your_Lm_value -r your_Lr_value` on the client side to enable the tunnel with PSS where W.X.Y.Z is the server IP address (e.g. the IP address associated to `eth0` for instance, not the `tun0` IP address) .

## 5. Acknowledgements

The authors wish to thank CNES and TeSA for support.
