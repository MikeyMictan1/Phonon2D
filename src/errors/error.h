#pragma once

#include "server/phonon_packet.h"

#include <cstdio>
#include <cstdlib>
#include <string>

PhononPacket* PacketPoolError(const std::string& message);