
#include <iostream>

#include <boost/asio.hpp>
#include <boost/array.hpp>

#include <thread>

#define BUFFER_SZ_INCOMING_MSG 1024

using boost::asio::ip::tcp;

void receive_messages(char* port)
{
    boost::asio::io_context io_context;
    tcp::endpoint endpoint = tcp::endpoint(tcp::v4(), atoi(port));

    tcp::acceptor acceptor(io_context);
    acceptor.open(endpoint.protocol());
    acceptor.bind(endpoint);
    acceptor.listen();

    tcp::socket socket(io_context);

    bool has_received_connection = false;
    while (!has_received_connection)
    {
        try
        {
            acceptor.accept(socket);
            has_received_connection = true;
        }
        catch (boost::system::system_error e)
        {
            std::cout << "[SERVER] " << e.what() << std::endl;
            // do nothing, re-attempt connection.
        }
    }


    for (;;)
    {

        boost::array<char, BUFFER_SZ_INCOMING_MSG> buffer;
        auto msg_buffer = boost::asio::buffer(buffer);

        boost::system::error_code error;
        
        std::string str_msg;
        // do
        // {
            size_t sz = socket.read_some(msg_buffer, error);
            str_msg += std::string(buffer.data(), sz);
        // } while(str_msg[str_msg.size() - 1] != '\0');

        if (!error)
            std::cout << "[MESSAGE] " << str_msg << std::endl;
        else if (error == boost::asio::error::eof)
        {
            std::cout << "[SERVER] Connection closed." << std::endl;
            break;
        }
        else if (error == boost::asio::error::timed_out)
            continue; // No incoming messages.
        else if (error == boost::asio::error::connection_aborted)
        {
            std::cout << "[SERVER] Connection aborted." << std::endl;
            break;
        }
        else if (error)
        {
            std::cout << error.what() << std::endl;
            throw boost::system::system_error(error); // Some other error.
        }
    }
}

void send_messages(char* port)
{
    boost::asio::io_context io_context;
    tcp::resolver resolver(io_context);
    tcp::resolver::results_type endpoints = resolver.resolve("localhost", port);

    tcp::socket socket(io_context);

    bool has_connected = false;
    while (!has_connected)
    {
        try 
        {
            std::string message;
            std::cout << "Attempt connection to " << port << "? (y/n) | > ";
            std::cin >> message;
            if (message == "n")
                return;

            std::cout << "Connecting to server..." << std::endl;
            boost::asio::connect(socket, endpoints);
            has_connected = true;
        }
        catch (boost::system::system_error e)
        {
            std::cout << "Connection failed." << std::endl;
        }
    }

    std::cout << "Connected to server." << std::endl;

    for (;;)
    {
        // Text prompt.
        std::string message;
        std::getline(std::cin, message);

        if (message == "quit")
            break;

        auto msg_buffer = boost::asio::buffer(message);

        boost::system::error_code error;
        socket.send(msg_buffer, 0, error);

        if (error == boost::asio::error::eof || error == boost::asio::error::connection_reset
          || error == boost::asio::error::connection_aborted || error == boost::asio::error::connection_reset)
        {
            std::cout << "Connection closed." << std::endl;
            return;
        }
        else if (error)
        {
            std::cout << error.what() << std::endl;
            throw boost::system::system_error(error); // Some other error.
        }
    }
}

int main(int argc, char *argv[])
{
    char* send_port = argv[1];
    char* receive_port = argv[2];

    std::thread t_receive(receive_messages, receive_port);

    // No need to join receiving thread, as it should run as long
    // as we are sending messages. This will probably throw an error
    // on exit, tho.
    while (1)
    {
        try
        {
            send_messages(send_port);
        }
        catch(const std::exception& e)
        {
            std::cout << "Connection closed. (" << e.what() << ")" << std::endl;
        }
    }
    return 0;
}