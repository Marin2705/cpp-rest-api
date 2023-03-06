#include <iostream>
#include <vector>
#include <string>
#include "Controller.hpp"
#include "Controller.cpp"
#include "lib.cpp"

int main(int argc, char *argv[])
{
    std::vector<Controller> router;
    router.emplace_back("/", "welcome ! available routes: /test, /foo");
    router.emplace_back("/test", "Hello World");
    router.emplace_back("/foo", "bar");

    start(argc, argv, router);
    return 0;
}
