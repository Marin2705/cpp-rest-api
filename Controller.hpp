#ifndef Controller_hpp
#define Controller_hpp

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/config.hpp>
#include <cstdlib>
#include <memory>
#include <thread>
#include <iostream>
#include <string>

namespace beast = boost::beast;   // from <boost/beast.hpp>
namespace http = beast::http;     // from <boost/beast/http.hpp>
namespace net = boost::asio;      // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

class Controller
{
public:
    Controller();
    Controller(std::string route, std::string response);
    std::string getRoute() const;
    std::string getResponse() const;
private:
    std::string m_route;
    std::string m_response;
};


#endif /* Controller_hpp */
