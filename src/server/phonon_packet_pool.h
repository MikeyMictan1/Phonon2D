#pragma once

#include "server/phonon_packet.h"
#include <vector>

// Owns the pool of PhononPackets. Hands out packets on request and takes them back when they are 'dead'.
class PhononPacketPool {
public:
    static constexpr std::size_t POOL_SIZE_DEFAULT = 4096;

    explicit PhononPacketPool(std::size_t pool_size = POOL_SIZE_DEFAULT) : packets(pool_size) {}

    PhononPacket* request_packet();
    void recycle_packet(PhononPacket& packet);
    void clear_pool();

private:
    std::vector<PhononPacket> packets;
};
