#include <libportal/portal.h>
#include <stdio.h>
#include <string.h>

#include <Cpeed/platform/logging.h>

#include "../linuxmain.h"
#include "parent-private.h"
#include "portal-cpeed.h"

struct _GCpeedWindow
{
    GObject parent_object;

    CpdWindow window;
};

G_DEFINE_TYPE(GCpeedWindow, g_cpeed_window, G_TYPE_OBJECT);

enum
{
    PROP_WINDOW = 1,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static const char wayland_id_prefix[] = "wayland:";
static const gchar parent_prop_name[] = "cpeed_handle";

static gboolean _xdp_parent_export_cpeed(XdpParent* parent, XdpParentExported callback, gpointer data) {
    if (parent->object == 0) {
        return FALSE;
    }

    CpdWaylandWindow* wl_window;
    g_object_get((gpointer)parent->object, parent_prop_name, (gpointer*)&wl_window, NULL);

    if (wl_window->handle == 0) {
        char* null_handle = (char*)malloc(sizeof(char));
        if (null_handle == 0) {
            return FALSE;
        }

        null_handle[0] = 0;

        parent->exported_handle = null_handle;
    }
    else {
        size_t len = strlen(wl_window->handle) + sizeof(wayland_id_prefix);
        parent->exported_handle = (char*)malloc(len);
        if (parent->exported_handle == 0) {
            return FALSE;
        }

        sprintf(parent->exported_handle, "%s%s", wayland_id_prefix, wl_window->handle);
    }

    callback(parent, parent->exported_handle, data);

    return TRUE;
}

static void _xdp_parent_unexport_cpeed(XdpParent* parent) {
    free(parent->exported_handle);
}

static void g_cpeed_window_set_property(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec) {
    if (property_id == PROP_WINDOW) {
        CPEED_WINDOW(object)->window = (CpdWindow)g_value_get_pointer(value);
    }
}

static void g_cpeed_window_get_property(GObject* object, guint property_id, GValue* value, GParamSpec* pspec) {
    if (property_id == PROP_WINDOW) {
        g_value_set_pointer(value, CPEED_WINDOW(object)->window);
    }
}

static void g_cpeed_window_class_init(GCpeedWindowClass* klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS(klass);

    object_class->set_property = g_cpeed_window_set_property;
    object_class->get_property = g_cpeed_window_get_property;

    obj_properties[PROP_WINDOW] = g_param_spec_pointer(
        "cpeed_handle",
        "Cpeed Window",
        "Cpeed window handle.",
        G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void g_cpeed_window_init(GCpeedWindow* self) {
    self->window = 0;
}

XdpParent* xdp_parent_new_cpeed(CpdWindow window) {
    XdpParent* parent = g_new0(XdpParent, 1);
    gpointer gobject = g_object_new(CPEED_WINDOW_TYPE, parent_prop_name, (gpointer)window, NULL);

    if (parent == 0 || gobject == 0) {
        xdp_parent_free(parent);
        g_object_unref(gobject);

        return 0;
    }

    parent->parent_export = _xdp_parent_export_cpeed;
    parent->parent_unexport = _xdp_parent_unexport_cpeed;
    parent->object = (GObject*)gobject;

    return parent;
}
