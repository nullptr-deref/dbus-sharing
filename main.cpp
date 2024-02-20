#include "./sharingservice.hpp"

#include <sdbus-c++/sdbus-c++.h>

#include <iostream>

using namespace std::literals::string_literals;

int main() {
    ipc::SharingService service;
    service.run();

    return 0;
}
