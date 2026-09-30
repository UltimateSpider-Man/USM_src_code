#include "traffic_light_interface.h"

#include "common.h"

VALIDATE_SIZE(traffic_light_interface, 0x10);

traffic_light_interface::traffic_light_interface(
        actor *a2) : actor_interface(a2)
{
    this->m_vtbl = 0x00883048;
    this->field_C = 0;
}
