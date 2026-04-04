#include <Cpeed/platform/dialog.h>
#include <Cpeed/platform/lock.h>
#include <Cpeed/platform.h>

#include <libportal/portal-helpers.h>
#include <libportal/parent.h>
#include <libportal/filechooser.h>
#include <malloc.h>
#include <string.h>
#include <time.h>

#include "libportal/portal-cpeed.h"
#include "linuxdialog.h"

#define WAIT_SLEEP_TIME_USEC 100000 // 100ms

typedef GVariant* (*XdpFinishFunction)(XdpPortal*, GAsyncResult*, GError**);

typedef struct CpdFileDialogInternal {
    CpdLock lock;
    GCancellable* cancellable;
    GVariant* result;
    XdpFinishFunction finish_function;
} CpdFileDialogInternal;

XdpPortal* g_portal;

bool file_dialog_supported() {
    return g_portal != 0;
}

static void dialog_callback(GObject* src, GAsyncResult* result, gpointer data) {
    CpdFileDialogInternal* fdi = (CpdFileDialogInternal*)data;
    XdpPortal* portal = (XdpPortal*)src;

    fdi->result = fdi->finish_function(portal, result, 0);

    leave_lock(fdi->lock);
}

CpdFileDialogResult request_open_file_dialog(
    CpdWindow parent, const char* title,
    const CpdFileDialogFilter* filters, size_t filter_count,
    bool multiple, bool add_wildcard,
    CpdFileDialogHandle* out_handle
) {
    if (g_portal == 0) {
        return CpdFileDialogResult_Unsupported;
    }

    if (out_handle == 0) {
        return CpdFileDialogResult_InvalidCall;
    }

    if (filters == 0 && filter_count > 0) {
        return CpdFileDialogResult_InvalidCall;
    }

    XdpParent* xparent = 0;
    if (parent != 0) {
        xparent = xdp_parent_new_cpeed(parent);
        if (xparent == 0) {
            return CpdFileDialogResult_InvalidCall;
        }
    }

    CpdFileDialogInternal* fdi = (CpdFileDialogInternal*)malloc(sizeof(CpdFileDialogInternal));
    if (fdi == 0) {
        if (xparent != 0) {
            xdp_parent_free(xparent);
        }

        return CpdFileDialogResult_OutOfMemory;
    }

    CpdLock lock = create_lock();
    if (lock == 0) {
        if (xparent != 0) {
            xdp_parent_free(xparent);
        }

        free(fdi);
        return CpdFileDialogResult_InternalError;
    }

    GCancellable* cancellable = g_cancellable_new();
    if (cancellable == 0) {
        if (xparent != 0) {
            xdp_parent_free(xparent);
        }

        destroy_lock(lock);
        free(fdi);
        return CpdFileDialogResult_InternalError;
    }

    if (!enter_lock(lock)) {
        g_object_unref(cancellable);
        destroy_lock(lock);
        free(fdi);
        return CpdFileDialogResult_InternalError;
    }

    fdi->lock = lock;
    fdi->result = 0;
    fdi->cancellable = cancellable;
    fdi->finish_function = xdp_portal_open_file_finish;

    *out_handle = (CpdFileDialogHandle)fdi;

    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE("a(sa(us))"));

    GVariantBuilder single_filter_builder;
    for (size_t i = 0; i < filter_count; i++) {
        g_variant_builder_init(&single_filter_builder, G_VARIANT_TYPE("a(us)"));

        size_t selector_count = filters[i].selector_count;

        for (size_t j = 0; j < selector_count; j++) {
            g_variant_builder_add(&single_filter_builder, "(us)", 0, filters[i].selectors[j].pattern);

            const char* mime_type = filters[i].selectors[j].mime_type;
            if (mime_type != 0) {
                g_variant_builder_add(&single_filter_builder, "(us)", 1, mime_type);
            }
        }

        g_variant_builder_add(&builder, "(s@a(us))", filters[i].name, g_variant_builder_end(&single_filter_builder));
    }

    if (add_wildcard) {
        g_variant_builder_init(&single_filter_builder, G_VARIANT_TYPE("a(us)"));
        g_variant_builder_add(&single_filter_builder, "(us)", 0, "*");

        g_variant_builder_add(&builder, "(s@a(us))", "All files", g_variant_builder_end(&single_filter_builder));
    }

    GVariant* gfilters = g_variant_builder_end(&builder);

    XdpOpenFileFlags flags = multiple ? XDP_OPEN_FILE_FLAG_MULTIPLE : XDP_OPEN_FILE_FLAG_NONE;
    
    xdp_portal_open_file(g_portal, xparent, title, gfilters, 0, 0, flags, cancellable, dialog_callback, fdi);

    if (xparent != 0) {
        xdp_parent_free(xparent);
    }

    return CpdFileDialogResult_Success;
}

CpdFileDialogResult request_save_file_dialog(
    CpdWindow parent, const char* title,
    const CpdFileDialogFilter* filters, size_t filter_count,
    const char* name, const char* folder, bool add_wildcard,
    CpdFileDialogHandle* out_handle
) {
    if (g_portal == 0) {
        return CpdFileDialogResult_Unsupported;
    }

    if (out_handle == 0) {
        return CpdFileDialogResult_InvalidCall;
    }

    if (filters == 0 && filter_count > 0) {
        return CpdFileDialogResult_InvalidCall;
    }

    XdpParent* xparent = 0;
    if (parent != 0) {
        xparent = xdp_parent_new_cpeed(parent);
        if (xparent == 0) {
            return CpdFileDialogResult_InvalidCall;
        }
    }

    CpdFileDialogInternal* fdi = (CpdFileDialogInternal*)malloc(sizeof(CpdFileDialogInternal));
    if (fdi == 0) {
        if (xparent != 0) {
            xdp_parent_free(xparent);
        }

        return CpdFileDialogResult_OutOfMemory;
    }

    CpdLock lock = create_lock();
    if (lock == 0) {
        if (xparent != 0) {
            xdp_parent_free(xparent);
        }

        free(fdi);
        return CpdFileDialogResult_InternalError;
    }

    GCancellable* cancellable = g_cancellable_new();
    if (cancellable == 0) {
        if (xparent != 0) {
            xdp_parent_free(xparent);
        }

        destroy_lock(lock);
        free(fdi);
        return CpdFileDialogResult_InternalError;
    }

    if (!enter_lock(lock)) {
        g_object_unref(cancellable);
        destroy_lock(lock);
        free(fdi);
        return CpdFileDialogResult_InternalError;
    }

    fdi->lock = lock;
    fdi->result = 0;
    fdi->cancellable = cancellable;
    fdi->finish_function = xdp_portal_save_file_finish;

    *out_handle = (CpdFileDialogHandle)fdi;

    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE("a(sa(us))"));

    GVariantBuilder single_filter_builder;
    for (size_t i = 0; i < filter_count; i++) {
        g_variant_builder_init(&single_filter_builder, G_VARIANT_TYPE("a(us)"));

        size_t selector_count = filters[i].selector_count;

        for (size_t j = 0; j < selector_count; j++) {
            g_variant_builder_add(&single_filter_builder, "(us)", 0, filters[i].selectors[j].pattern);

            const char* mime_type = filters[i].selectors[j].mime_type;
            if (mime_type != 0) {
                g_variant_builder_add(&single_filter_builder, "(us)", 1, mime_type);
            }
        }

        g_variant_builder_add(&builder, "(s@a(us))", filters[i].name, g_variant_builder_end(&single_filter_builder));
    }

    if (add_wildcard) {
        g_variant_builder_init(&single_filter_builder, G_VARIANT_TYPE("a(us)"));
        g_variant_builder_add(&single_filter_builder, "(us)", 0, "*");

        g_variant_builder_add(&builder, "(s@a(us))", "All files", g_variant_builder_end(&single_filter_builder));
    }

    GVariant* gfilters = g_variant_builder_end(&builder);
    
    xdp_portal_save_file(g_portal, xparent, title, name, folder, 0, gfilters, 0, 0, XDP_SAVE_FILE_FLAG_NONE, cancellable, dialog_callback, fdi);

    if (xparent != 0) {
        xdp_parent_free(xparent);
    }

    return CpdFileDialogResult_Success;
}

CpdFileDialogResult request_directory_dialog(
    CpdWindow parent, const char* title,
    CpdFileDialogHandle* out_handle
) {
    // Supported version of libportal (https://github.com/flatpak/libportal/issues/195) is not yet released (0.10)
    // It is currently only available in 'main' branch of libportal
    return CpdFileDialogResult_Unsupported;
}

CpdFileResult open_file_from_dialog_handle(CpdFileDialogHandle handle, const char* path, CpdFileOpening opening, CpdFileMode mode, CpdFileSharing sharing, CpdFile* file) {
    CpdFileDialogInternal* fdi = (CpdFileDialogInternal*)handle;

    if (fdi->result == 0) {
        return CpdFileResult_InvalidCall;
    }

    GVariantIter iter;
    g_variant_iter_init(&iter, fdi->result);

    GVariant* value;
    gchar* key;
    while (g_variant_iter_next(&iter, "{sv}", &key, &value)) {
        if (strcmp(key, "uris") == 0) {
            GVariantIter path_iter;
            gsize arr_count = g_variant_iter_init(&path_iter, value);

            gchar* gpath;
            for (gsize i = 0; g_variant_iter_next(&path_iter, "s", &gpath) && i < arr_count; i++) {
                size_t offset = strstr(gpath, "file://") == gpath ? strlen("file://") : 0;

                if (path == 0 || strcmp(gpath + offset, path) == 0) {
                    CpdFileResult result = open_file(gpath + offset, opening, mode, sharing, file);

                    g_free(gpath);
                    g_variant_unref(value);
                    g_free(key);

                    return result;
                }

                g_free(gpath);
            }

            g_variant_unref(value);
            g_free(key);
            break;
        }

        g_variant_unref(value);
        g_free(key);
    }

    return CpdFileResult_InvalidPath;
}

bool wait_for_file_dialog(CpdFileDialogHandle handle, uint64_t timeout, char*** paths, uint32_t* count) {
    CpdFileDialogInternal* fdi = (CpdFileDialogInternal*)handle;

    if (timeout == 0) {
        bool result = try_enter_lock(fdi->lock);
        if (result) {
            leave_lock(fdi->lock);
        }

        return result;
    }

    uint64_t timeout_timestamp = timeout == UINT64_MAX ? UINT64_MAX : get_clock_usec() + timeout;

    while (!try_enter_lock(fdi->lock)) {
        if (timeout != UINT64_MAX && timeout_timestamp < get_clock_usec()) {
            return false;
        }

        struct timespec requested = {
            .tv_sec = 0,
            .tv_nsec = WAIT_SLEEP_TIME_USEC * 1000
        };

        nanosleep(&requested, 0);
    }

    leave_lock(fdi->lock);

    if (fdi->result == 0) {
        *count = UINT32_MAX;
        return false;
    }

    if (paths == 0 || count == 0) {
        return true;
    }

    GVariantIter iter;
    g_variant_iter_init(&iter, fdi->result);

    GVariant* value;
    gchar* key;
    while (g_variant_iter_next(&iter, "{sv}", &key, &value)) {
        if (strcmp(key, "uris") == 0) {
            GVariantIter path_iter;
            gsize arr_count = g_variant_iter_init(&path_iter, value);
            char** arr = (char**)malloc(arr_count * sizeof(char*));
            
            if (arr == 0) {
                free(arr);

                g_variant_unref(value);
                g_free(key);
                return false;
            }

            gchar* path;
            for (gsize i = 0; g_variant_iter_next(&path_iter, "s", &path) && i < arr_count; i++) {
                size_t len = (strlen(path) + 1) * sizeof(char);
                size_t offset = 0;

                if (strstr(path, "file://") == path) {
                    offset = strlen("file://");
                    len -= offset;
                }

                arr[i] = (char*)malloc(len);
                
                if (arr[i] == 0) {
                    for (gsize j = 0; j < i; j++) {
                        free(arr[j]);
                    }

                    free(arr);

                    g_free(path);
                    g_variant_unref(value);
                    g_free(key);
                    return false;
                }

                memcpy(arr[i], path + offset, len);
                g_free(path);
            }

            *paths = arr;
            *count = arr_count;
            
            g_variant_unref(value);
            g_free(key);
            break;
        }

        g_variant_unref(value);
        g_free(key);
    }
    
    return true;
}

void cancel_file_dialog(CpdFileDialogHandle handle) {
    CpdFileDialogInternal* fdi = (CpdFileDialogInternal*)handle;

    g_cancellable_cancel(fdi->cancellable);
}

void destroy_file_dialog_handle(CpdFileDialogHandle handle) {
    CpdFileDialogInternal* fdi = (CpdFileDialogInternal*)handle;

    if (fdi->result != 0) {
        g_variant_unref(fdi->result);
    }

    g_object_unref(fdi->cancellable);
    destroy_lock(fdi->lock);

    free(fdi);
}
