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

class TCPConnection : public std::enable_shared_from_this<TCPConnection>
{
	public:
		/*****************
		 * Constructors
		 ****************/
		// Default constructor
		TCPConnection() = delete;

		// Deconstructor
		~TCPConnection() {};

		static std::shared_ptr<TCPConnection> create(boost::asio::io_context& _ioContext) { return std::shared_ptr<TCPConnection>(new TCPConnection(_ioContext)); }

		/*****************
		 * Server Functions
		 ****************/
		void start()
		{
			char iv1[4] = { 70, 114, 122, rand() % 127 };
			char iv2[4] = { 82,  48, 120, rand() % 127 };
		
			// This is the packet for the v62 MapleStory Global(NA) handshake
			ByteBuffer buff;
			buff.push_back(0x0E);
			buff.push_back(83);
			buff.push_back(1);
			buff.push_back(49);
			for(int i = 0; i < 4; ++i) { buff.push_back(iv1[i]); }
			for(int i = 0; i < 4; ++i) { buff.push_back(iv2[i]); }
			buff.push_back(8);
			buff.push_back('\0');

			this->writeBufferQueue.push(std::string(buff.begin(), buff.end()));
			this->write();
			this->read();
		}

		void shutdown()
		{
			// Shutdown read/write and the socket itself
			this->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
			this->socket.close();
		}

		void read()
		{
			// Read into readBuffer until we reach a null terminator '\0'
			boost::asio::async_read_until(this->socket,
										  this->readBuffer,
										  '\0',
										  boost::bind(&TCPConnection::handleRead,
										  shared_from_this(),
										  boost::asio::placeholders::error,
										  boost::asio::placeholders::bytes_transferred));
		}

		void write()
		{
			// Async write
			boost::asio::async_write(this->socket,
									 boost::asio::buffer(this->writeBufferQueue.front()),
									 boost::bind(&TCPConnection::handleWrite,
									 shared_from_this(),
									 boost::asio::placeholders::error,
									 boost::asio::placeholders::bytes_transferred));
		}

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
				std::cout << "Num bytes read: " << _bytes_transferred << '\n';
				std::cout << "Bytes received: " << str << '\n';

				// Clear the read buffer
				this->readBuffer.consume(_bytes_transferred);
				this->read();
			}
			else
			{
				std::cout << _error.message() << '\n';
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
				std::cout << "Num bytes written: " << _bytes_transferred << '\n';
				std::cout << "Bytes written: " << str << '\n';
			}
			else
			{
				std::cout << _error.message() << '\n';
			}
		}

		/*****************
		 * Getters & Setters
		 ****************/
		tcp::socket& getSocket() { return this->socket; }

	private:
		TCPConnection(boost::asio::io_context& _ioContext) : socket(_ioContext) {}

		tcp::socket socket;
		boost::asio::streambuf readBuffer;
		std::queue<std::string> writeBufferQueue;
};

class TCPServer : public std::enable_shared_from_this<TCPServer>
{
	typedef std::shared_ptr<TCPConnection> Connection; 
	typedef std::shared_ptr<std::vector<Connection>> Connections;
	public:
		/*****************
		 * Constructors
		 ****************/
		// Default constructor
		TCPServer() = delete;

		// Parameterized constructors
		TCPServer(boost::asio::io_context& _ioContext, size_t _port) : ioContext(_ioContext), acceptor(_ioContext, tcp::endpoint(tcp::v4(), _port)) 
		{
			this->connections = std::make_shared<std::vector<Connection>>();
		}
			
		// Destructor
		~TCPServer() 
		{
			for(Connection c: *(this->connections))
			{
				c->shutdown();
			}
		}
		
		/****************
		 * Server functions
		 ***************/
		void startAccept()
		{
			// Async accept connection
			std::shared_ptr<TCPConnection> newConnection = TCPConnection::create(this->ioContext);
			this->acceptor.async_accept(newConnection->getSocket(),
										boost::bind(&TCPServer::handleAccept,
										shared_from_this(),
										newConnection,
										boost::asio::placeholders::error));
		}

		void handleAccept(std::shared_ptr<TCPConnection> _newConnection, const boost::system::error_code& _error)
		{
			// If theres an async error, close the connection
			if (!_error)
			{
				// Push new connection onto our list of connections and start communications
				this->connections->push_back(_newConnection);
				_newConnection->start();

				// Async accept new client
				this->startAccept();
			}
			else
			{
				std::cout << _error.message() << '\n';
			}
		}

	private:
		boost::asio::io_context& ioContext;
		tcp::acceptor acceptor;
		Connections connections;
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
	std::shared_ptr<TCPServer> server = std::make_shared<TCPServer>(io_context, 1111);
	server->startAccept();
	io_context.run();
	
	return 0;
}