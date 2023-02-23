#ifndef __FLAGS__
#define __FLAGS__

class Flags
{
public:
    Flags() = default;
    ~Flags() = default;

    int tcp_port = 56988;
    bool force_tcp = false;
    bool force_x11 = false;
    bool force_sixel = false;
    bool silent = false;
};

#endif
