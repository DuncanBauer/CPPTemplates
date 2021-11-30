// C++
#include <iostream>
#include <memory>
#include <queue>
#include <vector>

// Boost
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/bind/bind.hpp>

// Namespaces
using boost::asio::ip::udp;

// Typedefs
typedef std::vector<unsigned char> ByteBuffer;

class UDPClient : public std::enable_shared_from_this<UDPClient>
{
    public:
		/*****************
		 * Constructors
		 ****************/
		// Default constructor
        UDPClient() = delete;

		// Parameterized constructor
		UDPClient(boost::asio::io_context& _io_context, const std::string& _host, const std::string& _port) : socket(_io_context), resolver(_io_context)
		{
			// Initialize the endpoint we're talking to
			this->remoteEndpoint = *resolver.resolve(udp::v4(), _host, _port).begin();
			// Open the socket for communication
			this->socket.open(udp::v4());
			this->socket.bind(this->localEndpoint);
			this->mSocketActive = true;
		}

		// Deconstructor
		~UDPClient()
		{
			this->shutdown();
		}

		/*****************
		 * Server Functions
		 ****************/
		void start()
		{
			this->send();
			this->receive();
		}

		void shutdown()
		{
			// Handles and ignores
			// `The I/O operation has been aborted because of either a thread exit or an application request` exception
			// for a quick and dirty shutdown
			try
			{
				// Shutdown send/receive and the socket itself
				this->socket.shutdown(boost::asio::ip::udp::socket::shutdown_both);
				this->socket.close();
				this->mSocketActive = false;
			}
			catch(std::exception _error)
			{
				std::cout << "Error: " << _error.what() << '\n';
			}
		}

		void receive()
		{
			// Read into readBuffer until we reach a null terminator '\0'
			this->socket.async_receive_from(boost::asio::buffer(this->receiveBuffer),
											this->remoteEndpoint,
											boost::bind(&UDPClient::handleReceive,
														shared_from_this(),
														boost::asio::placeholders::error,
														boost::asio::placeholders::bytes_transferred));
		}

		void send()
		{
			// Async write
			this->sendBufferQueue.push(std::string("Hello"));
			std::cout << "Sending: " << this->sendBufferQueue.front() << '\n';

			this->socket.async_send_to(boost::asio::buffer(this->sendBufferQueue.front(), this->sendBufferQueue.front().size()),
									   this->remoteEndpoint,
									   boost::bind(&UDPClient::handleSend,
												   shared_from_this(),
												   boost::asio::placeholders::error,
												   boost::asio::placeholders::bytes_transferred));
		}

		void handleReceive(const boost::system::error_code& _error, size_t _bytes_transferred)
		{
			if (!_error)
			{
				std::string str(this->receiveBuffer.begin(), this->receiveBuffer.end());
				std::cout << "Num bytes read: " << _bytes_transferred << '\n';
				std::cout << "Read: " << str << '\n';
				this->receive();
    		}
			else
			{
				std::cout << "Error: " << _error.message() << '\n';
			}
		}

		void handleSend(const boost::system::error_code& _error, size_t _bytes_transferred)
		{
			if (!_error)
			{
				std::cout << "Num bytes written: " << _bytes_transferred << '\n';
				std::cout << "Writing: " << this->sendBufferQueue.front() << '\n';
				this->sendBufferQueue.pop();
    		}
			else
			{
				std::cout << "Error: " << _error.message() << '\n';
			}
		}

    private:
		bool mSocketActive = false;
        udp::socket socket;
        udp::resolver resolver;
		udp::endpoint localEndpoint;
		udp::endpoint remoteEndpoint;
		boost::array<char, 128> receiveBuffer;
		std::queue<std::string> sendBufferQueue;
};

boost::asio::io_context io_context;

void stopEverything(std::shared_ptr<UDPClient> _client);
void stopEverything(std::shared_ptr<UDPClient> _client)
{
	if(_client.use_count() == 1)
	{
		_client.reset();
	}
	io_context.stop();
}

int main(int argc, char* argv[])
{
	// Initialize the UDPClient
	std::shared_ptr<UDPClient> client = std::make_shared<UDPClient>(io_context, "127.0.0.1", "1111");

	// Create an input loop inside a lambda function
	auto inputLoop = [&client]()
	{
        bool q = false;
        char cmd;
      
        while(!q)
        {
            std::cout << "Quit: (Q or q)" << '\n';
            std::cout << "Enter command: " << '\n' << "> ";
            std::cin >> cmd;
            switch(cmd)
            {
                case 'Q':
                    [[fallthrough]];
                case 'q':
                    q = true;
					stopEverything(client);
                    break;
                default:
                    break;
            }
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    };
	// Thread our input loop
    std::jthread t1(inputLoop);

	// Start the client proper
	client->start();

	// If the client can't connect to the server, this won't block
	io_context.run();
	
	return 0;
}