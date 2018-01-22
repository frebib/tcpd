#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>

#include <netinet/in.h>

#include <netstack/eth/arp.h>


bool arp_log(struct pkt_log *log, struct frame *frame) {
    struct arp_hdr *msg = (struct arp_hdr *) frame->head;
    frame->data += ARP_HDR_LEN;
    struct log_trans *trans = &log->t;

    struct arp_ipv4 *req;
    switch (ntohs(msg->proto)) {
        case ETH_P_IP:
            req = (struct arp_ipv4 *) frame->data;
            switch (ntohs(msg->op)) {
                case ARP_OP_REQUEST:
                    LOGT(trans, "Who has %s? ", fmtip4(ntohl(req->dipv4)));
                    LOGT(trans, "Tell %s ", fmtip4(ntohl(req->sipv4)));
                    break;
                case ARP_OP_REPLY:
                    LOGT(trans, "Reply %s ", fmtip4(ntohl(req->sipv4)));
                    LOGT(trans, "is at %s ", fmtmac(req->saddr));
                    break;
                default:
                    LOGT(trans, "invalid op %d", ntohs(msg->op));
            }
            break;
        default:
            LOGT(trans, "unrecognised proto %d", ntohs(msg->proto));
            break;
    };
    
    return true;
}

void arp_recv(struct frame *frame) {
    struct arp_hdr *msg = (struct arp_hdr *) frame->head;
    frame->data += ARP_HDR_LEN;

    switch (ntohs(msg->hwtype)) {
        case ARP_HW_ETHER:
            // this is good
            break;
        default:
            LOG(LINFO, "ARP hardware %d not supported\n", ntohs(msg->hwtype));
    }

    // https://tools.ietf.org/html/rfc826

    struct arp_ipv4 *req;
    switch (ntohs(msg->proto)) {
        case ETH_P_IP:
            // also good
            req = (struct arp_ipv4 *) frame->data;

            addr_t ether = {.proto = PROTO_ETHER, .ether = eth_arr(req->saddr)};
            addr_t ipv4 = {.proto = PROTO_IPV4, .ipv4 = ntohl(req->sipv4)};

            bool updated = arp_update_entry(frame->intf, &ether, &ipv4);

            // Only cache ARP entry if it was sent to us
            if (!updated && intf_has_addr(frame->intf, &ipv4))
                arp_cache_entry(frame->intf, &ether, &ipv4);

            if (updated && frame->intf->arptbl.length > 0)
                arp_log_tbl(frame->intf, LINFO);

            // TODO: Check for queued outgoing packets that can
            //       now be sent with the ARP information recv'd

            switch (ntohs(msg->op)) {
                case ARP_OP_REQUEST: {
                    // If asking for us, send a reply with our LL address
                    addr_t ip = {.proto = PROTO_IPV4, .ipv4 = ntohl(req->dipv4)};
                    if (intf_has_addr(frame->intf, &ip))
                        arp_send_reply(frame->intf, ARP_HW_ETHER,
                                       ntohl(req->dipv4), ntohl(req->sipv4),
                                       req->saddr);
                    break;
                }
                case ARP_OP_REPLY:
                default:
                    break;
            }
            break;
        default:
            LOG(LINFO, "ARP protocol %s not supported",
                    fmt_ethertype(ntohs(msg->proto)));
    };
}

void arp_log_tbl(struct intf *intf, loglvl_t level) {
    struct log_trans trans = LOG_TRANS(level);
    LOGT(&trans, "Intf\tProtocol\tHW Address\t\tState\n");
    for_each_llist(&intf->arptbl) {
        struct arp_entry *entry = llist_elem_data();
        LOGT(&trans, "%s\t", intf->name);
        LOGT(&trans, "%s\t", straddr(&entry->protoaddr));
        LOGT(&trans, "%s\t", (entry->state & ARP_PENDING) ?
                              "(pending)\t" : straddr(&entry->hwaddr));
        LOGT(&trans, "%s\n", fmt_arp_state(entry->state));
    }
    LOGT_COMMIT(&trans);
}

/* Retrieves IPv4 address from table, otherwise NULL */
addr_t *arp_get_hwaddr(struct intf *intf, uint16_t hwtype, addr_t *protoaddr) {

    for_each_llist(&intf->arptbl) {
        struct arp_entry *entry = llist_elem_data();

        if (entry == NULL) {
            LOG(LERR, "arp_entry_ipv4 is null?\t");
            return NULL;
        }
        // Check matching protocols
        if (addreq(&entry->protoaddr, protoaddr)) {
            // Ignore entry if it doesn't have ARP_RESOLVED
            if (!(entry->state & ARP_RESOLVED))
                continue;
            if (entry->hwaddr.proto == hwtype)
                return &entry->hwaddr;
        }
    }

    return NULL;
}

bool arp_update_entry(struct intf *intf, addr_t *hwaddr, addr_t *protoaddr) {

    // TODO: Use hashtable for ARP lookups on IPv4

    for_each_llist(&intf->arptbl) {
        struct arp_entry *entry = llist_elem_data();

        // If existing IP match, update it
        // TODO: This doesn't account for protocol addresses that change hw
        if (addreq(&entry->protoaddr, protoaddr)) {
            // Only update hwaddr if it has actually changed
            if (!addreq(&entry->hwaddr, hwaddr)) {
                LOG(LINFO, "ARP cache entry %s changed", straddr(protoaddr));

                // Update hwaddr for IP
                memcpy(&entry->hwaddr, hwaddr, sizeof(addr_t));
            }
            // Remove PENDING and add RESOLVED
            entry->state &= ~ARP_PENDING;
            entry->state |= ARP_RESOLVED;

            // An entry was updated
            return true;
        }
    }
    // Nothing was updated
    return false;
}

bool arp_cache_entry(struct intf *intf, addr_t *hwaddr, addr_t *protoaddr) {

    LOG(LDBUG, "Storing new ARP entry for %s\n", straddr(protoaddr));

    struct arp_entry *entry = malloc(sizeof(struct arp_entry));
    entry->state = ARP_RESOLVED;
    memcpy(&entry->hwaddr, hwaddr, sizeof(addr_t));
    memcpy(&entry->protoaddr, protoaddr, sizeof(addr_t));

    llist_append(&intf->arptbl, entry);

    return true;
}

int arp_send_req(struct intf *intf, uint16_t hwtype,
                 uint32_t saddr, uint32_t daddr) {

    struct frame *frame = intf_frame_new(intf, intf_max_frame_size(intf));
    struct arp_ipv4 *req = frame_alloc(frame, sizeof(struct arp_ipv4));
    struct arp_hdr *hdr = frame_alloc(frame, sizeof(struct arp_hdr));

    // TODO: Use hwtype to determine length and type of address
    memcpy(&req->saddr, intf->ll_addr, ETH_ADDR_LEN);
    memcpy(&req->daddr, ETH_BRD_ADDR, ETH_ADDR_LEN);
    req->sipv4 = htonl(saddr);
    req->dipv4 = htonl(daddr);
    hdr->hwtype = htons(hwtype);
    hdr->proto = htons(ETH_P_IP);
    hdr->hlen = ETH_ADDR_LEN;
    hdr->plen = (uint8_t) addrlen(PROTO_IPV4);
    hdr->op = htons(ARP_OP_REQUEST);

    int ret = ether_send(frame, ETH_P_ARP, ETH_BRD_ADDR);
    // Ensure frame is free'd if it was never actually sent
    frame_decref(frame);

    // Sending ARP request was successful, add incomplete cache entry
    if (!ret) {
        // Check if partial entry already exists, so to not add multiple
        addr_t protoaddr = {.proto = PROTO_IPV4, .ipv4 = daddr};
        bool entry_exist = false;
        for_each_llist(&intf->arptbl) {
            struct arp_entry *entry = llist_elem_data();
            if (entry == NULL)
                continue;
            if (addreq(&entry->protoaddr, &protoaddr)) {
                entry_exist = true;
                break;
            }
        }
        // Don't add another partial entry if one is there already
        if (entry_exist)
            return ret;

        struct arp_entry *entry = malloc(sizeof(struct arp_entry));
        *entry = (struct arp_entry) {
            .state = ARP_PENDING,
            .hwaddr = {.proto = PROTO_ETHER, .ether = eth_arr(ETH_NUL_ADDR)},
            .protoaddr = {.proto = PROTO_IPV4, .ipv4 = daddr}
        };
        llist_append(&intf->arptbl, entry);

        arp_log_tbl(intf, LINFO);
    }

    return ret;
}

int arp_send_reply(struct intf *intf, uint8_t hwtype, uint32_t sip,
                   uint32_t dip, uint8_t *daddr) {
    // TODO: Add 'incomplete' entry to arp cache

    struct frame *frame = intf_frame_new(intf, intf_max_frame_size(intf));
    struct arp_ipv4 *req = frame_alloc(frame, sizeof(struct arp_ipv4));
    struct arp_hdr *hdr = frame_alloc(frame, sizeof(struct arp_hdr));

    // TODO: Use hwtype to determine length and type of address
    memcpy(&req->saddr, intf->ll_addr, ETH_ADDR_LEN);
    memcpy(&req->daddr, daddr, ETH_ADDR_LEN);
    req->sipv4 = htonl(sip);
    req->dipv4 = htonl(dip);
    hdr->hwtype = htons(hwtype);
    hdr->proto = htons(ETH_P_IP);
    hdr->hlen = ETH_ADDR_LEN;
    hdr->plen = (uint8_t) addrlen(PROTO_IPV4);
    hdr->op = htons(ARP_OP_REPLY);

    int ret = ether_send(frame, ETH_P_ARP, ETH_BRD_ADDR);
    // Ensure frame is free'd if it was never actually sent
    frame_decref(frame);
    return ret;
}
