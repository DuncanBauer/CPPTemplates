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

// Consts
const int RECV_BUFFER_SIZE = 128;

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
			this->receiveBuffer.assign(0);
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
			// Async receive
			this->socket.async_receive_from(boost::asio::buffer(this->receiveBuffer),
											this->remoteEndpoint,
											boost::bind(&UDPClient::handleReceive,
														shared_from_this(),
														boost::asio::placeholders::error,
														boost::asio::placeholders::bytes_transferred));
		}

		void send()
		{
			// Async send
			if(this->sendBufferQueue.size() > 0)
			{
				this->socket.async_send_to(boost::asio::buffer(this->sendBufferQueue.front(), this->sendBufferQueue.front().size()),
										this->remoteEndpoint,
										boost::bind(&UDPClient::handleSend,
													shared_from_this(),
													boost::asio::placeholders::error,
													boost::asio::placeholders::bytes_transferred));
			}
		}

		void pushOntoSendQueue(std::string _str)
		{
			std::lock_guard<std::mutex> lock(this->m);
			this->sendBufferQueue.push(_str + '\0');
		}

		/*****************
		 * Getters & Setters
		 ****************/
		udp::socket& getSocket() { return this->socket; }
		bool isSocketActive() { return this->mSocketActive; }

    private:
		void handleReceive(const boost::system::error_code& _error, size_t _bytes_transferred)
		{
			if (!_error)
			{
				std::string str(this->receiveBuffer.begin(), this->receiveBuffer.begin() + _bytes_transferred);

				std::cout << "Received: ";
				for(int i = 0; i < str.size(); ++i)
				{
					std::cout << std::hex << (unsigned int)str[i] << ' ';
				}
				std::cout << "\n\n";

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
				std::string str = this->sendBufferQueue.front();
				this->sendBufferQueue.pop();

				std::cout << "Sending: ";
				for(int i = 0; i < str.size(); ++i)
				{
					std::cout << std::hex << (unsigned int)str[i] << ' ';
				}
				std::cout << "\n\n";

				
				if(this->sendBufferQueue.size() > 0)
				{
					this->send();
				}
    		}
			else
			{
				std::cout << "Error: " << _error.message() << '\n';
			}
		}

		std::mutex m;
		bool mSocketActive = false;
        udp::socket socket;
        udp::resolver resolver;
		udp::endpoint localEndpoint;
		udp::endpoint remoteEndpoint;
		boost::array<unsigned char, RECV_BUFFER_SIZE> receiveBuffer;
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
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 
            switch(cmd)
            {
                case 'Q': [[fallthrough]];
                case 'q':
                    q = true;
					stopEverything(client);
                    break;
				case 'S': [[fallthrough]];
				case 's':
					client->send();
					break;
				case 'W': [[fallthrough]];
				case 'w':
				// Must brace this block for the initialization of str
				{
					std::string str;
					std::cout << "Enter message to send: ";
					std::getline(std::cin, str);
					client->pushOntoSendQueue(str);
					break;
				}
				default:
                    break;
            }
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