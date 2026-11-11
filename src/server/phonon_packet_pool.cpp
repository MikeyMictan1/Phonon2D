#include "server/phonon_packet_pool.h"
#include "errors/error.h"

#include <algorithm>

// Iteratively go through the packet vector until a free packet is found
PhononPacket* PhononPacketPool::request_packet() {
    for (PhononPacket& packet : packets) {
        if (!packet.active) {
            packet.active = true;
            return &packet;
        }
    }

    return PacketPoolError("Currently no free phonon packets");
}

// reset packets to being active=false so they can be requested again
void PhononPacketPool::recycle_packet(PhononPacket& packet) {
    packet = PhononPacket{};
}

// Resets all the packets in the packet pool
void PhononPacketPool::clear_pool() { std::fill(packets.begin(), packets.end(), PhononPacket{}); }