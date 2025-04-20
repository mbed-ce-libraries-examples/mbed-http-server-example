#include "mbed.h"
#include "http_server.h"
#include "http_response_builder.h"

#ifndef LED1
#error Your Mbed board does not have an LED to blink defined!
#endif

DigitalOut led(LED1);

// Requests come in here
void request_handler(ParsedHttpRequest* request, TCPSocket* socket) {

    printf("Request came in: %s %s\n", http_method_str(request->get_method()), request->get_url().c_str());

    if (request->get_method() == HTTP_GET && request->get_url() == "/") {
        HttpResponseBuilder builder(200);
        builder.set_header("Content-Type", "text/html; charset=utf-8");

        char response[] = "<html><head><title>Hello from mbed</title></head>"
            "<body>"
                "<h1>mbed webserver</h1>"
                "<button id=\"toggle\">Toggle LED</button>"
                "<script>document.querySelector('#toggle').onclick = function() {"
                    "var x = new XMLHttpRequest(); x.open('POST', '/toggle'); x.send();"
                "}</script>"
            "</body></html>";

        builder.send(socket, response, sizeof(response) - 1);
    }
    else if (request->get_method() == HTTP_POST && request->get_url() == "/toggle") {
        printf("toggle LED called\n");
        led = !led;

        HttpResponseBuilder builder(200);
        builder.send(socket, NULL, 0);
    }
    else {
        HttpResponseBuilder builder(404);
        builder.send(socket, NULL, 0);
    }
}

int main() {
    // Connect to the network
    NetworkInterface* network = NetworkInterface::get_default_instance();
    if (!network) {
        printf("No default network interface defined for this target\n");
        return 1;
    }

    // To use wi-fi, uncomment the below code instead of the above code.
    // auto *const network = WiFiInterface::get_default_instance();
    // network->set_credentials("my network name",
    //                                 "my network password",
    //                                 NSAPI_SECURITY_WPA2);

    auto ret = network->connect();
    if(ret != NSAPI_ERROR_OK) {
        printf("Failed to initialize networking. Error: %d", ret);
        return 1;
    }

    // Construct a server, allowing it to persist in memory permanently
    HttpServer * server = new HttpServer(network);
    nsapi_error_t res = server->start(8080, &request_handler);

    if (res == NSAPI_ERROR_OK) {
        SocketAddress ourAddr;
        network->get_ip_address(&ourAddr);
        printf("Server is listening at http://%s:8080\n", ourAddr.get_ip_address());
    }
    else {
        printf("Server could not be started... %d\n", res);
    }
    
    // Main thread will now exit but server threads keep running
}
