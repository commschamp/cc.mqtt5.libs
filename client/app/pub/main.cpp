
#include "Pub.h"

#include <boost/asio.hpp>

#include <iostream>
#include <stdexcept>

int main(int argc, const char* argv[])
{
    try {
        boost::asio::io_context io;
        cc_mqtt5_client_app::Pub app(io);

        if (!app.start(argc, argv)) {
            return -1;
        }

        io.run();
    }
    catch (const std::exception& ec)
    {
        std::cerr << "ERROR: Unexpected exception: " << ec.what() << std::endl;
    }
    return 0;
}