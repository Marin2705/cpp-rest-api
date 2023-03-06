// #include <iostream>
// #include <string>
#include "Controller.hpp"

Controller::Controller(std::string route, std::string response) : m_route(route), m_response(response) {}

std::string Controller::getRoute() const
{
    return m_route;
}

std::string Controller::getResponse() const
{
    return m_response;
}