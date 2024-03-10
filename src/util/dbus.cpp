#include "util/dbus.hpp"
#include <stdexcept>

DbusUtil::DbusUtil(const std::string &address)
    : connection(dbus_connection_open(address.c_str(), nullptr))
{
    if (connection == nullptr) {
        throw std::invalid_argument("Could not connect to dbus address");
    }
}

DbusUtil::~DbusUtil()
{
    dbus_connection_unref(connection);
}
