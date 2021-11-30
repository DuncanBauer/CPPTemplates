// C++
#include <ios>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

// Boost
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

// Namespaces
using boost::asio::ip::tcp;

// Aliases
using ByteBuffer = std::vector<unsigned char>;

class TCPClient : public std::enable_shared_from_this<TCPClient>
{
    public:
		/*****************
		 * Constructors
		 ****************/
		// Default constructor
        TCPClient() = delete;

		// Parameterized constructor
		TCPClient(boost::asio::io_context& io_context, const std::string& server, size_t port) : resolver(io_context), socket(io_context)
        {
			// Initialize and connect the socket
			tcp::endpoint endpoint(boost::asio::ip::address::from_string(server), port);
			try
			{
				this->socket.connect(endpoint);
				this->mSocketActive = true;
    		}
			catch(const std::exception& e)
			{
				std::cerr << e.what() << '\n';
			}
	    }
		
		// Deconstructor
		~TCPClient()
		{
			this->socket.close();
		}

		/*****************
		 * Server Functions
		 ****************/
		void start()
		{
			this->read();
		}

		void shutdown()
		{
			// Handles and ignores
			// `The I/O operation has been aborted because of either a thread exit or an application request` exception
			// for a quick and dirty shutdown
			try
			{
				// Shutdown read/write and the socket itself
				this->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
				this->socket.close();
				this->mSocketActive = false;
			}
			catch(std::exception _error)
			{
				std::cout << "Error: " << _error.what() << '\n';
			}
		}

        void read()
        {
			// Read into readBuffer until we reach a null terminator '\0'
			boost::asio::async_read_until(this->socket,
										  this->readBuffer,
										  '\0',
										  boost::bind(&TCPClient::handleRead,
													  shared_from_this(),
													  boost::asio::placeholders::error,
													  boost::asio::placeholders::bytes_transferred));
		}

		void write()
		{
			// Async write
			if(this->writeBufferQueue.size() > 0)
			{
				boost::asio::async_write(this->socket,
										boost::asio::buffer(this->writeBufferQueue.front()),
										boost::bind(&TCPClient::handleWrite,
													shared_from_this(),
													boost::asio::placeholders::error,
													boost::asio::placeholders::bytes_transferred));
			}
		}

		void pushOntoWriteQueue(std::string _str)
		{
			std::lock_guard<std::mutex> lock(this->m);
			this->writeBufferQueue.push(_str + '\0');
		}

		/*****************
		 * Getters & Setters
		 ****************/
		bool isSocketActive() { return this->mSocketActive; }

    private:
		void handleRead(const boost::system::error_code& _error, size_t _bytes_transferred)
        {
			// If theres an async error, close the connection
			if (!_error)
			{
				// Construct a std::string from a boost::asio::streambuf
				// We might read in extra data past the delimiter (According to Boost docs) so we don't read the whole buffer.
				// Only read the number of bytes before the \0 
				boost::asio::streambuf::const_buffers_type bufs = this->readBuffer.data();
				std::string str(boost::asio::buffers_begin(bufs), boost::asio::buffers_begin(bufs) + _bytes_transferred);

				// Process data
				std::cout << "Bytes received: ";
				for(int i = 0; i < _bytes_transferred; ++i)
				{
					std::cout << std::hex << (unsigned int)str[i] << ' ';
				}
				std::cout << "\n\n";

				// Clear the read buffer
				this->readBuffer.consume(_bytes_transferred);
				this->read();
			}
			else
			{
				std::cout << "Error: " << _error.message() << '\n';
			}
        }

        void handleWrite(const boost::system::error_code& _error, size_t _bytes_transferred)
        {
			// If theres an async error, close the connection
			if (!_error)
			{
				// Pop the written packet from the write buffer
				std::string str(this->writeBufferQueue.front());
				this->writeBufferQueue.pop();
				
				// Keep this for testing/examples
				std::cout << "Bytes written: ";
				for(int i = 0; i < _bytes_transferred; ++i)
				{
					std::cout << std::hex << (unsigned int)str[i] << ' ';
				}
				std::cout << "\n\n";

				if(this->writeBufferQueue.size() > 0)
				{
					this->write();
				}
			}
			else
			{
				std::cout << "Error: " << _error.message() << '\n';
			}
        }

		std::mutex m;
		bool mSocketActive = false;
        tcp::socket socket;
        tcp::resolver resolver;
		boost::asio::streambuf readBuffer;
		std::queue<std::string> writeBufferQueue;
};

/****************************
* Global variables and functions used in main
****************************/
boost::asio::io_context io_context;

void stopEverything(std::shared_ptr<TCPClient> _client);
void pushPacket(std::shared_ptr<TCPClient> _client);

void stopEverything(std::shared_ptr<TCPClient> _client)
{
	if(_client.use_count() == 1)
	{
		_client.reset();
	}
	io_context.stop();
}

int main(int argc, char* argv[])
{
	// Initialize the TCPClient
	std::shared_ptr<TCPClient> client = std::make_shared<TCPClient>(io_context, "127.0.0.1", 1111);

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
					client->write();
					break;
				case 'W': [[fallthrough]];
				case 'w':
				// Must brace this block for the initialization of str
				{
					std::string str;
					std::cout << "Enter message to send: ";
					std::getline(std::cin, str);
					client->pushOntoWriteQueue(str);
					break;
				}
				default:
                    break;
            }
        }
    };
	// Thread our input loop
    std::jthread t1(inputLoop);
	
	// If the client fails to connect to the server, don't attempt to start communicating
	if(client->isSocketActive())
	{ 
		client->start(); 
	}

	// If the client can't connect to the server, this won't block
	io_context.run();
	
	return 0;
}