#pragma once
#include <string>
#include <vector>
namespace usm::online {
struct LocalAddress {std::string adapter,ipv4;};
// Synchronous OS enumeration. Call only on opening the session panel/hosting,
// never from a rendering callback or on every game frame.
std::vector<LocalAddress> local_ipv4_addresses();
}
