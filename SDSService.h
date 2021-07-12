#pragma once

// System libraries

#include <mutex>
#include <condition_variable>
#include <cstdio>
#include <memory>

// ASIO

#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/signal_set.hpp>
#include <asio/write.hpp>
#include <asio/awaitable.hpp>
#include <asio/use_awaitable.hpp>

// ASIO Using declerations

using asio::ip::tcp;
using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::use_awaitable;
namespace this_coro = asio::this_coro;

#define SIGNAGE_PORT 17540

struct client_info
{
    tcp::endpoint ip;
    std::string name;
    std::string location;
};

class SDSService
{
public:

    int run()
    {
        std::unique_lock<std::mutex> lock(mtx);
        
        try {
            asio::signal_set signals(io, SIGINT, SIGTERM);
            signals.async_wait([&](auto, auto) { io.stop(); });

            co_spawn(io, listener(), detached);

            ping_timer = asio::steady_timer(io, asio::chrono::seconds(10));
            ping_timer.async_wait(std::bind(&SDSService::ping_clients, this, std::placeholders::_1));

            io.run();
        }
        catch (std::exception& e)
        {
            std::printf("Error on start: %s\n", e.what());
            curstate = EXIT_FAILURE;
        }

        return state();
    }

    void handle_write() {
        ;
    }

    awaitable<bool> ping_client(client_info client) {
        try {
            tcp::socket sock(io);
            co_await sock.async_connect(client.ip,use_awaitable);
            co_await sock.async_send(asio::buffer({ 'p' }),use_awaitable);
            char out = 'x';
            co_await sock.async_read_some(asio::buffer(&out, 1),use_awaitable);
            co_return out == 'y';
        }
        catch (asio::error_code ec) {
            printf("Client %s failed the ping (error: %s)", client.ip.address().to_string().c_str(), ec.message().c_str());
            co_return false;
        }
    }

    awaitable<void> ping_clients(const asio::error_code&) {
        for (auto it = clients.begin(); it != clients.end(); ++it) {
            auto i = std::distance(clients.begin(), it);
            if (!co_await ping_client(clients[i])) {
                clients.erase(it);
            }
        }

        ping_timer.async_wait(std::bind(&SDSService::ping_clients, this, std::placeholders::_1));
    }

    awaitable<void> send_client_list(tcp::socket* socket) {
        std::stringstream stream;
        stream << 'r';
        std::string ip;
        for (auto client : clients) {
            ip = client.ip.address().to_string();
            stream << '\0' << ip.length() << client.name.length() << client.location.length() << ip << client.name << client.location << '\0';
        }
        co_await (*socket).async_send(asio::buffer(stream.str()), use_awaitable);
    }

    awaitable<void> on_data(tcp::socket socket) {
        try {
            char inst;
            co_await socket.async_read_some(asio::buffer(&inst,1), use_awaitable);

            char len[2];
            co_await socket.async_read_some(asio::buffer(len, 2), use_awaitable);

            std::string name;
            co_await socket.async_read_some(asio::buffer(name, len[0]), use_awaitable);

            std::string location;
            co_await socket.async_read_some(asio::buffer(location, len[1]), use_awaitable);

            switch (inst) {
            case 'a':
                clients.emplace_back(
                    client_info{
                        socket.remote_endpoint(),
                        name,
                        location
                    }
                );
                break;
            case 'c':
                // Get config
                break;
            case 's':
                // Set config
                break;
            case 'r':
                co_await send_client_list(&socket);
                break;
            default:
                co_await socket.async_send(asio::buffer("x"), use_awaitable);
            }
        }
        catch (std::exception& e) {
            printf("Error on recv: %s\n", e.what());
        }
    }

    awaitable<void> listener() {
        auto executor = co_await this_coro::executor;
        tcp::acceptor acceptor(executor, { tcp::v4(), SIGNAGE_PORT });
        for (;;)
        {
            tcp::socket socket = co_await acceptor.async_accept(use_awaitable);
            co_spawn(executor, on_data(std::move(socket)), detached);
        }
    }

    void stop()
    {
        std::lock_guard<std::mutex> lock(mtx);
        io.stop();
        running = false;
    }

    //@return 0 for A_OK and anything else for failures.
    int state() const { return curstate; }

private:
    mutable std::mutex mtx;
    bool running = true;
    asio::io_context io;
    asio::steady_timer ping_timer = asio::steady_timer(io, asio::chrono::seconds(10));;
    std::vector<client_info> clients;
    int curstate = EXIT_SUCCESS;
};