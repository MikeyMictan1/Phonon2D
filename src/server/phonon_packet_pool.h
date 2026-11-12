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
    std::size_t get_active_count() const;

    // Takes a function and applies it to every active phonon
    template <typename Func>
    void for_each_active(Func&& p_func) {
        for (PhononPacket& packet : packets) {
            if (packet.active) {
                p_func(packet);
            }
        }
    }

    // Read-only version, for things that only look at packets (e.g. drawing)
    template <typename Func>
    void for_each_active(Func&& p_func) const {
        for (const PhononPacket& packet : packets) {
            if (packet.active) {
                p_func(packet);
            }
        }
    }

private:
    std::vector<PhononPacket> packets;
};
