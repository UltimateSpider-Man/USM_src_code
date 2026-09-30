#include "wds_ai_manager.h"

#include "func_wrapper.h"
#include "osassert.h"
#include "path_graph.h"
#include "resource_key.h"

#include <cassert>

wds_ai_manager::wds_ai_manager()
{

}

path_graph *wds_ai_manager::get_path_graph(const resource_key &id)
{
    assert(id.get_type() == RESOURCE_KEY_TYPE_PATH);

    if constexpr (1)
    {
        for ( auto &pg : this->path_graph_list )
        {
            if ( pg != nullptr )
            {
                if ( pg->id == id ) {
                    return pg;
                }
            }
        }

        auto &v6 = id.m_hash;
        auto *v5 = v6.to_string();
        warning("Path graph %s does not exist", v5);
        return nullptr;
    } else {
        return (path_graph *) THISCALL(0x0053CE80, this, &id);
    }
}

void wds_ai_manager::add_path_graph(path_graph *pg)
{
    assert(pg != nullptr);

    if constexpr (1) {
        for ( auto it = this->path_graph_list.begin(); it != this->path_graph_list.end(); )
        {
            if ( (*it) != nullptr && (*it) == pg ) {
#ifdef OPENUSM_XBPACK_V10
                // The legacy single-element erase uses unchecked_copy, whose
                // scalar memmove is stubbed out. Use the range overload's
                // std::copy so unloading one path preserves the other paths.
                it = this->path_graph_list.erase(it, it + 1);
#else
                it = this->path_graph_list.erase(it);
#endif
            } else {
                ++it;
            }
        }
    } else {
        THISCALL(0x00537330, this, pg);
    }
}
