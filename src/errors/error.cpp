#include "errors/error.h"

// Error logging for the phonon packet pool
PhononPacket* PacketPoolError(const std::string& message) {
    std::fprintf(stderr, "Packet Pool Error: %s\n", message.c_str());
    return nullptr;
}