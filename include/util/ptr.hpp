#ifndef __UTIL_PTR__
#define __UTIL_PTR__

#include <memory>
#include <openssl/evp.h>
#include <gmodule.h>

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

template <typename T>
using unique_C_ptr = std::unique_ptr<T, free_deleter>;

static_assert(sizeof(char *) == sizeof(unique_C_ptr<char>));

#endif
