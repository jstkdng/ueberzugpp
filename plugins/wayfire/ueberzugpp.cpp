#include <memory>
#include <unordered_map>
#include <wayfire/core.hpp>
#include <wayfire/scene-render.hpp>
#include <wayfire/util.hpp>
#include <wayfire/view.hpp>
#include <wayfire/output.hpp>
#include <wayfire/plugin.hpp>
#include <wayfire/scene-operations.hpp>
#include <wayfire/plugins/ipc/ipc-method-repository.hpp>
#include <wayfire/plugins/common/shared-core-data.hpp>
#include <wayfire/unstable/wlr-view-events.hpp>
#include <wayfire/unstable/wlr-surface-node.hpp>
#include <wayfire/unstable/translation-node.hpp>

namespace wf
{
class ueberzugpp_surface_t
{
    std::weak_ptr<wf::view_interface_t> terminal;
    std::shared_ptr<scene::wlr_surface_node_t> surface;
    std::shared_ptr<scene::translation_node_t> translation;
    wlr_xdg_toplevel *toplevel;

    wf::wl_listener_wrapper on_toplevel_destroy;

  public:

    ueberzugpp_surface_t(const std::weak_ptr<wf::view_interface_t>& _terminal,
        wlr_xdg_toplevel *_toplevel):
        terminal(_terminal),
        toplevel(_toplevel)
    {
        // We create a custom surface node for the ueberzugpp window and wrap it in a translation node, so that
        // we can control its position.
        // We add the translation node as a subsurface of the terminal.
        surface = std::make_shared<scene::wlr_surface_node_t>(_toplevel->base->surface, true);
        translation = std::make_shared<scene::translation_node_t>();

        translation->set_children_list({surface});

        auto term = _terminal.lock();
        wf::scene::add_front(term->get_surface_root_node(), translation);

        // Finally, wait for the toplevel to be destroyed
        on_toplevel_destroy.set_callback([this] (auto)
        {
            destroy_callback();
        });
        on_toplevel_destroy.connect(&toplevel->base->events.destroy);
    }

    std::function<void()> destroy_callback;

    ~ueberzugpp_surface_t()
    {
        wf::scene::remove_child(translation);
    }

    void set_offset(int xcoord, int ycoord)
    {
        translation->set_offset({xcoord, ycoord});
        const auto term = terminal.lock();
        if (!term) {
            return;
        }
        term->damage();
    }
};


class ueberzugpp_mapper : public wf::plugin_interface_t
{
  public:
    void init() override
    {
        ipc_repo->register_method("ueberzugpp/set_offset", set_offset);
        wf::get_core().connect(&on_new_xdg_surface);
    }

    void fini() override
    {
        ipc_repo->unregister_method("ueberzugpp/set_offset");
    }

    ipc::method_callback set_offset = [this] (nlohmann::json json)
    {
        // NOLINTBEGIN
        WFJSON_EXPECT_FIELD(json, "app-id", string);
        WFJSON_EXPECT_FIELD(json, "x", number_integer);
        WFJSON_EXPECT_FIELD(json, "y", number_integer);
        // NOLINTEND

        const std::string app_id = json["app-id"];
        const int xcoord = json["x"];
        const int ycoord = json["y"];

        const auto search = surfaces.find(app_id);
        if (search == surfaces.end())
        {
            return ipc::json_error("Unknown ueberzugpp window with appid " + app_id);
        }

        search->second->set_offset(xcoord, ycoord);
        return ipc::json_ok();
    };

    shared_data::ref_ptr_t<ipc::method_repository_t> ipc_repo;

    // When a new xdg_toplevel is created, we need to check whether it is an ueberzugpp window by looking at
    // its app-id. If it is indeed an ueberzugpp window, we take over the toplevel (by setting
    // use_default_implementation=false) and create our own ueberzugpp_surface. In addition, we directly map
    // the surface to the currently focused view, if it exists.
    wf::signal::connection_t<new_xdg_surface_signal> on_new_xdg_surface = [this] (new_xdg_surface_signal *event)
    {
        if (!event->use_default_implementation) {
            return;
        }
        if (event->surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
            return;
        }

        const auto* toplevel_title = event->surface->toplevel->title;
        const std::string appid = toplevel_title != nullptr ? toplevel_title : "null";
        const std::string search_for = "ueberzugpp_";
        if (appid.find(search_for) != std::string::npos)
        {
            auto terminal = wf::get_core().get_active_output()->get_active_view();
            if (!terminal)
            {
                LOGE("Which window to map ueberzugpp to????");
                return;
            }

            event->use_default_implementation = false;
            const auto [iter, was_inserted] = surfaces.insert_or_assign(appid,
                    std::make_unique<ueberzugpp_surface_t>(terminal->weak_from_this(), event->surface->toplevel));
            iter->second->destroy_callback = [this, appid]
            {
                surfaces.erase(appid);
            };
        }
    };

    std::unordered_map<std::string, std::unique_ptr<ueberzugpp_surface_t>> surfaces;
};
} // end namespace wf

// NOLINTNEXTLINE
DECLARE_WAYFIRE_PLUGIN(wf::ueberzugpp_mapper);
