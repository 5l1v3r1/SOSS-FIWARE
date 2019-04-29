/*
 * Copyright (C) 2018 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#include "Listener.hpp"

#include <iostream>

namespace soss {
namespace fiware {


Listener::Listener(
        uint16_t port,
        DataReceivedCallback callback)

    : port_{port}
    , listen_thread_{}
    , running_{false}
    , errors_{false}
    , read_callback_{callback}
{
}

Listener::~Listener()
{
    stop();
}

void Listener::run()
{
    listen_thread_ = std::thread(&Listener::listen, this);
    running_ = true;

    //TODO: should wait until the port was open, use a condition variable?
}

void Listener::stop()
{
    if(running_ && listen_thread_.joinable())
    {
        running_ = false;
        listen_thread_.join();
        //TODO Add a timeout to the acceptor to not blocking the thread forever here
    }
}

void Listener::listen()
{
    try
    {
        asio::io_service service;
        asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), port_);
        asio::ip::tcp::acceptor acceptor(service, endpoint);

        std::array<char, BUFFER_SIZE> buffer;

        std::cout << "[soss-fiware][listener]: listening fiware at port " << port_ << std::endl;

        while(running_)
        {
            asio::error_code error;
            asio::ip::tcp::socket socket(service);
            acceptor.accept(socket);

            std::cout << "[soss-fiware][listener]: message from: "
                << socket.remote_endpoint().address().to_string() << std::endl;

            std::stringstream ss;
            std::size_t length = socket.read_some(asio::buffer(buffer, BUFFER_SIZE), error);
            ss.write(buffer.data(), length);

            //Connection problem
            if (error && asio::error::eof != error)
            {
                throw asio::system_error(error);
            }

            //Problem with this socket
            if (0 == length || asio::error::eof == error)
            {
                break;
            }

            read_callback_(ss.str());
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "[soss-fiware][listener]: connection error: " << e.what() << std::endl;
        errors_ = true;
    }

    std::cout << "[soss-fiware][listener]: stop listening" << std::endl;
}


} // namespace fiware
} // namespace soss