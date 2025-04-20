/*
 * PackageLicenseDeclared: Apache-2.0
 * Copyright (c) 2017 ARM Limited
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
 */

#ifndef _HTTP_SERVER_
#define _HTTP_SERVER_

#include "mbed.h"
#include "http_request_parser.h"
#include "http_response.h"
#include "http_response_builder.h"

// Maximum number of concurrent connections supported
#ifndef HTTP_SERVER_MAX_CONCURRENT
#define HTTP_SERVER_MAX_CONCURRENT      5
#endif

typedef HttpResponse ParsedHttpRequest;

/**
 * \brief HttpServer implements the logic for setting up an HTTP server.
 */
class HttpServer {
public:
    /**
     * HttpRequest Constructor
     *
     * @param[in] network The network interface
    */
    HttpServer(NetworkInterface* network)
    {
        _network = network;
    }

    ~HttpServer() {
        // Terminate all currently running threads
        rtos::ScopedMutexLock lock(thread_pool_mutex);

        main_thread.terminate();

        for(auto & entry : sockets_and_threads) {
            entry.second.first->terminate();
            entry.second.second->close();
        }
    }

    /**
     * Start running the server (it will run on its own thread)
     */
    nsapi_error_t start(uint16_t port, Callback<void(ParsedHttpRequest* request, TCPSocket* socket)> a_handler) {
        nsapi_error_t ret;

        ret = server.open(_network);
        if (ret != NSAPI_ERROR_OK) {
            return ret;
        }

        ret = server.bind(port);
        if (ret != NSAPI_ERROR_OK) {
            return ret;
        }

        server.listen(HTTP_SERVER_MAX_CONCURRENT); // max. concurrent connections...

        handler = a_handler;

        main_thread.start(callback(this, &HttpServer::main));

        return NSAPI_ERROR_OK;
    }

private:

    void receive_data() {

        // Find socket
        TCPSocket * socket = nullptr;

        {
            rtos::ScopedMutexLock lock(thread_pool_mutex);
            auto mapIter = sockets_and_threads.find(rtos::ThisThread::get_id());
            if(mapIter != sockets_and_threads.end()) {
                socket = mapIter->second.second.get();
            }
        }

        MBED_ASSERT(socket != nullptr);

        // needs to keep running until the socket gets closed
        while (1) {

            ParsedHttpRequest response;
            HttpParser parser(&response, HTTP_REQUEST);

            // Set up a receive buffer (on the heap)
            uint8_t* recv_buffer = (uint8_t*)malloc(HTTP_RECEIVE_BUFFER_SIZE);

            // TCPSocket::recv is called until we don't have any data anymore
            nsapi_size_or_error_t recv_ret;
            while ((recv_ret = socket->recv(recv_buffer, HTTP_RECEIVE_BUFFER_SIZE)) > 0) {
                // Pass the chunk into the http_parser
                size_t nparsed = parser.execute((const char*)recv_buffer, recv_ret);
                if (static_cast<nsapi_size_or_error_t>(nparsed) != recv_ret) {
                    printf("Parsing failed... parsed %d bytes, received %d bytes\n", nparsed, recv_ret);
                    recv_ret = -2101;
                    break;
                }

                if (response.is_message_complete()) {
                    break;
                }
            }
            // error?
            if (recv_ret <= 0) {
                free(recv_buffer);

                // Bail out of the thread if we had an error not related to parsing
                if (recv_ret < -3000 || recv_ret == 0) {
                    printf("Error reading from socket %d\n", recv_ret);
                    break;
                }
                else {
                    continue;
                }
            }

            // When done, call parser.finish()
            parser.finish();

            // Free the receive buffer
            free(recv_buffer);

            // Let user application handle the request, if user needs a handle to response they need to memcpy themselves
            if (recv_ret > 0) {
                handler(&response, socket);
            }
        }

        socket->close();
    }

    void main() {
        while (1) {
            nsapi_error_t accept_result;
            TCPSocket* clt_sock = server.accept(&accept_result);

            rtos::ScopedMutexLock lock(thread_pool_mutex);

            // Create new thread
            Thread* t = nullptr;
            if (clt_sock != nullptr) {
                t = new Thread(osPriorityNormal, 2048);
            }

            // Next, find and delete any terminated threads.
            // This needs to be done first just in case the new thread got the same ID as a previously
            // terminated thread.
            for (auto socketThreadIter = sockets_and_threads.begin(); socketThreadIter != sockets_and_threads.end(); socketThreadIter++) {
                if (socketThreadIter->second.first->get_state() == Thread::Deleted) {
                    CriticalSectionLock criticalSection;
                    socketThreadIter = sockets_and_threads.erase(socketThreadIter);
                }
            }

            // Now, start the thread running. However, note that it won't be able to actually begin executing
            // until we release the thread_pool_mutex lock.
            if (clt_sock != nullptr) {
                t->start(callback(this, &HttpServer::receive_data));

                // Once the thread has been started, store it and the socket in the map by its ID.
                sockets_and_threads.emplace(t->get_id(), std::make_pair(t, clt_sock));
            }
            else {
                printf("Failed to accept connection: %d\n", accept_result);
            }
        }
    }

    TCPSocket server;
    NetworkInterface* _network;
    Thread main_thread;

    // Mutex which must be locked to create, delete, or iterate threads
    rtos::Mutex thread_pool_mutex;

    // Maps server thread IDs to their thread objects and sockets.
    // unique_ptrs are used so that the objects will get deleted when the map entry is removed.
    std::map<osThreadId_t, std::pair<std::unique_ptr<Thread>, std::unique_ptr<TCPSocket>>> sockets_and_threads;

    Callback<void(ParsedHttpRequest* request, TCPSocket* socket)> handler;
};

#endif // _HTTP_SERVER