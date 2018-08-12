﻿#include <future>
#include <memory>
#include <chrono>

#include <brynet/net/SyncConnector.h>

brynet::net::TcpSocket::PTR brynet::net::SyncConnectSocket(std::string ip,
    int port,
    std::chrono::milliseconds timeout,
    brynet::net::AsyncConnector::PTR asyncConnector)
{
    if (asyncConnector == nullptr)
    {
        asyncConnector = AsyncConnector::Create();
        asyncConnector->startWorkerThread();
    }

    auto socketPromise = std::make_shared<std::promise<TcpSocket::PTR>>();
    asyncConnector->asyncConnect(
        ip,
        port,
        timeout,
        [socketPromise](TcpSocket::PTR socket) {
        socketPromise->set_value(std::move(socket));
    }, [socketPromise]() {
        socketPromise->set_value(nullptr);
    });

    return socketPromise->get_future().get();
}

brynet::net::DataSocket::PTR brynet::net::SyncConnectSession(std::string ip,
    int port,
    std::chrono::milliseconds timeout,
    brynet::net::TcpService::PTR service,
    const std::vector<TcpService::AddSocketOption::AddSocketOptionFunc>& options,
    brynet::net::AsyncConnector::PTR asyncConnector)
{
    if (service == nullptr)
    {
        return nullptr;
    }

    if (asyncConnector == nullptr)
    {
        asyncConnector = AsyncConnector::Create();
        asyncConnector->startWorkerThread();
    }

    auto sessionPromise = std::make_shared<std::promise<DataSocket::PTR>>();
    asyncConnector->asyncConnect(
        ip,
        port,
        timeout,
        [=](TcpSocket::PTR socket) mutable {
        socket->SocketNodelay();

        auto enterCallback = [sessionPromise](const DataSocket::PTR& session) mutable {
            sessionPromise->set_value(session);
        };
        std::vector < TcpService::AddSocketOption::AddSocketOptionFunc> tmpOptions = options;
        tmpOptions.push_back(brynet::net::TcpService::AddSocketOption::WithEnterCallback(enterCallback));
        service->addDataSocket(std::move(socket), tmpOptions);
    }, [sessionPromise]() {
        sessionPromise->set_value(nullptr);
    });

    auto future = sessionPromise->get_future();
    if (future.wait_for(timeout) != std::future_status::ready)
    {
        return nullptr;
    }
    
    return future.get();
}