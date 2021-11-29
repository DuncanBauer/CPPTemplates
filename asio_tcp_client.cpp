// C++
#include <iostream>
#include <memory>
#include <queue>
#include <vector>

// Boost
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

// Namespaces
using boost::asio::ip::tcp;

// Typedefs
typedef std::vector<unsigned char> ByteBuffer;

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
			ByteBuffer buff;
			buff.push_back('H');
			buff.push_back('e');
			buff.push_back('l');
			buff.push_back('l');
			buff.push_back('o');
			buff.push_back('\0');

			this->writeBufferQueue.push(std::string(buff.begin(), buff.end()));
			this->write();
			this->read();
		}
		void shutdown()
		{
			this->socket.close();
		}
        void read()
        {
			std::cout << "Read" << '\n';
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
			std::cout << "Write" << '\n';
			boost::asio::async_write(this->socket,
									 boost::asio::buffer(this->writeBufferQueue.front()),
								     boost::bind(&TCPClient::handleWrite,
									 shared_from_this(),
								     boost::asio::placeholders::error,
								     boost::asio::placeholders::bytes_transferred));
		}
		bool isSocketActive() { return this->mSocketActive; }

    private:
		void handleRead(const boost::system::error_code& _error, size_t _bytes_transferred)
        {
			std::cout << "Handle read" << '\n';
			// if (!(boost::asio::error::eof == _error && boost::asio::error::connection_reset == _error))
			if (!_error)
			{
				boost::asio::streambuf::const_buffers_type bufs = this->readBuffer.data();
				std::string str(boost::asio::buffers_begin(bufs), boost::asio::buffers_end(bufs));

				std::cout << "Num bytes read: " << _bytes_transferred << '\n';
				std::cout << "Bytes received: " << str << '\n';
				this->readBuffer.consume(_bytes_transferred);
				this->read();
			}
			else
			{
				std::cout << _error.message() << '\n';
				this->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
				// this->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, _error);
			}
        }
        void handleWrite(const boost::system::error_code& _error, size_t _bytes_transferred)
        {
			std::cout << "Handle write" << '\n';
			// if (!(boost::asio::error::eof == _error && boost::asio::error::connection_reset == _error))
			if (!_error)
			{
				std::string str(this->writeBufferQueue.front());
				this->writeBufferQueue.pop();
				
				std::cout << "Num bytes written: " << _bytes_transferred << '\n';
				std::cout << "Bytes written: " << str << '\n';
			}
			else
			{
				std::cout << _error.message() << '\n';
				this->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
				// this->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, _error);
			}
        }

        tcp::resolver resolver;
        tcp::socket socket;
		bool mSocketActive = false;
		boost::asio::streambuf readBuffer;
		std::queue<std::string> writeBufferQueue;
};

int main(int argc, char* argv[])
{
	auto inputLoop = []() {
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
                    break;
                default:
                    break;
            }
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    };
    std::jthread t1(inputLoop);

	boost::asio::io_context io_context;
	std::shared_ptr<TCPClient> client = std::make_shared<TCPClient>(io_context, "127.0.0.1", 1111);
	if(client->isSocketActive()) 
	{ 
		client->start(); 
	}
	io_context.run();
	
	std::cout << "Shutting down client\n";
	return 0;
}