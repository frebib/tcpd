#ifndef NETSTACK_NEIGH_H
#define NETSTACK_NEIGH_H

#include <netstack/addr.h>
#include <netstack/col/llist.h>
#include <netstack/lock/retlock.h>

/*
 * Neighbour provides functionality to dispatch internet protocol packets
 * onto an IP network, routing them appropriately. It also handles resolving
 * the hardware addresses and dispatching queued packets to those devices as
 * hardware addresses are resolved.
 *
 *   Neighbour: An immediately connected device (within the same subnet) that
 *   is zero hops away. This is usually either the destination device or the
 *   local gateway router.
 */

struct queued_pkt {
    retlock_t retwait;
    struct frame *frame;
    addr_t saddr;
    addr_t daddr;
    addr_t nexthop;
    uint8_t proto;
    uint16_t flags;
};

/*!
 * Sends an IP packet to a neighbour, being the nexthop in the route.
 * Given a destination address, a route is found and hardware address
 * looked-up and the packet is forwarded on.
 * Packets that cannot be dispatched straight away will be queued and sent as
 * soon as the required lower-layer information is available.
 * @param frame frame to send or queue
 * @param proto IP header protocol field
 * @param flags IP flags
 * @param daddr IP destination address
 * @param saddr IP source address (should belong to frame->intf
 * @param wait  set true if should wait until the packet is sent or if this
 *              call should return immediately
 * @return 0 on success, otherwise if error
 */
int neigh_send(struct frame *frame, uint8_t proto, uint16_t flags,
               addr_t *daddr, addr_t *saddr, bool wait);

/*!
 * Updates the neighbour processing of a hardware address update that can be
 * used to send queued packets
 * @param intf   interface the hwaddr relates to
 * @param daddr  IP address that the hwaddr relates to
 * @param hwaddr the hwaddr that has updated
 */
void neigh_update_hwaddr(struct intf *intf, addr_t *daddr, addr_t *hwaddr);

#endif //NETSTACK_NEIGH_H
