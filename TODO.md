# A list of every _todo_ comment in this project
**Please note:** _This file is automatically generated from all TODO comments found within the code_
## [include/netstack/eth/arp.h](include/netstack/eth/arp.h)
  - Line 60: reduce redundant arguments passed to arp_send_req/reply
  - Line 61: infer interface and hwtype based on routing rules

## [include/netstack/intf/intf.h](include/netstack/intf/intf.h)
  - Line 28: Implement 'virtual' network interfaces
  - Line 42: Move arptbl into an 'ethernet' hardware struct into `void *ll`

## [include/netstack/ip/ipv4.h](include/netstack/ip/ipv4.h)
  - Line 30: Take endianness into account in ipv4_hdr

## [include/netstack/ip/route.h](include/netstack/ip/route.h)
  - Line 25: Change addr type from uint32_t to generic inet_addr_t for ipv4/6

## [include/netstack/tcp/tcp.h](include/netstack/tcp/tcp.h)
  - Line 38: Take endianness into account in tcp_hdr

## [lib/eth/arp.c](lib/eth/arp.c)
  - Line 73: Check for queued outgoing packets that can
  - Line 114: Implement ARP cache locking
  - Line 137: Use hashtable for ARP lookups on IPv4
  - Line 143: This doesn't account for protocol addresses that change hw
  - Line 185: Use hwtype to determine length and type of address
  - Line 234: Add 'incomplete' entry to arp cache
  - Line 240: Use hwtype to determine length and type of address

## [lib/intf/intf.c](lib/intf/intf.c)
  - Line 110: Implement rx 'software' timestamping
  - Line 118: Conditionally print debugging information
  - Line 140: Use same frame stack instead of cloning across threads
  - Line 208: Check intf hwtype to calculate max frame size
  - Line 234: Selectively choose an appropriate address from intf

## [lib/intf/rawsock.c](lib/intf/rawsock.c)
  - Line 41: This is hacky, assuiming lo is loopback
  - Line 123: Move some of this cleanup logic into a generic intf_free() function
  - Line 132: Ensure frames are destroyed, even if they still have references
  - Line 157: Allocate a buffer from the interface for frame storage
  - Line 162: Find an appropriate size for the control buffer
  - Line 194: Print debug messages for uncommon paths like these

## [lib/ip/icmp.c](lib/ip/icmp.c)
  - Line 81: Don't assume IPv4 parent
  - Line 87: Fix frame->data pointer head/tail difference

## [lib/ip/ipv4.c](lib/ip/ipv4.c)
  - Line 71: Keep track of invalid packets
  - Line 83: Take options into account here
  - Line 90: Other integrity checks
  - Line 92: Change to `if (!ipv4_should_accept(frame))` to accept other packets
  - Line 142: Perform correct route/hardware address lookups when appropriate
  - Line 185: Implement ARP cache locking
  - Line 194: Rate limit ARP requests to prevent flooding
  - Line 210: Make this user-configurable

## [lib/ip/route.c](lib/ip/route.c)
  - Line 5: Lock route table for writing
  - Line 18: Define how routes with the same metric should behave?

## [lib/tcp/tcp.c](lib/tcp/tcp.c)
  - Line 26: Investigate TCP checksums invalid with long packets
  - Line 33: Check for TSO and GRO and account for it, somehow..
  - Line 39: Other integrity checks
  - Line 48: Use hashtbl instead of list to lookup sockets
  - Line 49: Lock llist tcp_sockets for concurrent access

## [lib/tcp/tcpin.c](lib/tcp/tcpin.c)
  - Line 21: Send TCP RST for invalid connections
  - Line 22: Optionally don't send TCP RST packets

## [tools/netd/src/main.c](tools/netd/src/main.c)
  - Line 16: Add many configurable interfaces
  - Line 17: Add loopback interface
  - Line 51: Take different socket types into account here
  - Line 53: For now, assume everything is ethernet