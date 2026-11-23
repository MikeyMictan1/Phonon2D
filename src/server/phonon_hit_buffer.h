#pragma once

#include "server/phonon_hit.h"

#include <cstddef>
#include <vector>

// Fixed-size ring buffer (circular queue) of hits, oldest to newest.
// Hits arrive in time order, so expiring old ones only ever removes from the front.
// When full, a new hit overwrites the oldest one: the window then effectively gets shorter.
// Plain C++ (no GDCLASS), like PhononPacketPool: internal storage, never exposed to GDScript.
class PhononHitBuffer {
public:
    static constexpr std::size_t CAPACITY_DEFAULT = 1024;

    explicit PhononHitBuffer(std::size_t p_capacity = CAPACITY_DEFAULT) : hits(p_capacity) {}

    // Add at the back. Overwrites the oldest hit if full.
    void push(const PhononHit &p_hit) {
        if (hits.empty()) {
            return;
        }
        const std::size_t back = (front + count) % hits.size();
        hits[back] = p_hit;
        if (count < hits.size()) {
            count++;
        } else {
            front = (front + 1) % hits.size(); // full: the oldest was just overwritten, so the front moves on
        }
    }

    // Remove hits from the front while they are older than p_cutoff
    void drop_older_than(float p_cutoff) {
        while (count > 0 && hits[front].time < p_cutoff) {
            front = (front + 1) % hits.size();
            count--;
        }
    }

    // i = 0 is the oldest hit, i = size() - 1 the newest
    const PhononHit &operator[](std::size_t i) const { return hits[(front + i) % hits.size()]; }

    template <typename Func>
    void for_each(Func &&p_func) const {
        for (std::size_t i = 0; i < count; i++) {
            p_func((*this)[i]);
        }
    }

    std::size_t size() const { return count; }
    std::size_t capacity() const { return hits.size(); }
    bool is_full() const { return count == hits.size(); }

    void clear() {
        front = 0;
        count = 0;
    }

    // Changing capacity drops all stored hits (only expected from the inspector, not mid-game)
    void set_capacity(std::size_t p_capacity) {
        hits.assign(p_capacity, PhononHit());
        clear();
    }

private:
    std::vector<PhononHit> hits; // sized once, never grows on push
    std::size_t front = 0;        // index of the oldest hit
    std::size_t count = 0;        // how many slots are in use
};
