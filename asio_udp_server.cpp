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

class UDPServer : public std::enable_shared_from_this<UDPServer>
{
	public:
		/*****************
		 * Constructors
		 ****************/
		// Default constructor
		UDPServer() = delete;

		// Parameterized constructors
		UDPServer(boost::asio::io_context& _ioContext, size_t _port) : ioContext(_ioContext), socket(_ioContext, udp::endpoint(udp::v4(), _port)), mSocketActive(true)
		{
			this->receiveBuffer.assign(0);
		}
			
		// Destructor
		~UDPServer() {}
		
		/****************
		 * Server functions
		 ***************/
		void start()
		{
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
											boost::bind(&UDPServer::handleReceive,
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
										boost::bind(&UDPServer::handleSend,
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
		boost::asio::io_context& ioContext;
		udp::socket socket;
		udp::endpoint remoteEndpoint;
		boost::array<unsigned char, RECV_BUFFER_SIZE> receiveBuffer;
		std::queue<std::string> sendBufferQueue;
};

boost::asio::io_context io_context;

void stopEverything(std::shared_ptr<UDPServer> _server);
void stopEverything(std::shared_ptr<UDPServer> _server)
{
	if(_server.use_count() == 1)
	{
		_server.reset();
	}
	io_context.stop();
}

int main(int argc, char* argv[])
{
	// Initialize the UDPServer
	std::shared_ptr<UDPServer> server = std::make_shared<UDPServer>(io_context, 1111);

	// Create an input loop inside a lambda function
	auto inputLoop = [&server]()
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
					stopEverything(server);
                    break;
                default:
                    break;
            }
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    };
	// Thread our input loop
    std::jthread t1(inputLoop);

	// Start the server proper
	server->start();

	// If the client can't connect to the server, this won't block
	io_context.run();
	
	return 0;
}