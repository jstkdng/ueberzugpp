// Display images inside a terminal
// Copyright (C) 2023  JustKidding
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef __UTIL_PTR__
#define __UTIL_PTR__

#include <memory>
#include <openssl/evp.h>
#include <gmodule.h>
#ifdef ENABLE_X11
#   include <xcb/xproto.h>
#endif

struct free_deleter{
    template <typename T>
    void operator()(T *ptr) const {
        // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
        std::free(const_cast<std::remove_const_t<T>*>(ptr));
    }
};

struct evp_md_ctx_deleter {
    void operator()(EVP_MD_CTX* ctx) const {
        EVP_MD_CTX_free(ctx);
    }
};

struct gstring_deleter {
    void operator()(GString* str) const {
        g_string_free(str, true);
    }
};

struct gchar_deleter {
    void operator()(gchar **str) const {
        g_strfreev(str);
    }
};

#ifdef ENABLE_X11
struct x11_connection_deleter {
    void operator()(xcb_connection_t* connection) const {
        xcb_disconnect(connection);
    }
};
#endif

template <typename T>
using unique_C_ptr = std::unique_ptr<T, free_deleter>;

static_assert(sizeof(char *) == sizeof(unique_C_ptr<char>));

#endif
