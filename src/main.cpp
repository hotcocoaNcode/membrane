#include <iostream>
#include <string>
#include "mbnwrite.h"

int main(int argc, char** argv) {
    if (argc > 2) {
        assemble_file(argv[1], std::stoull(argv[2]));
    } else if (argc == 2) {
        std::cout << "warn; no dynamic memory requirement specified. assuming zero." << std::endl;
        assemble_file(argv[1], 0);
    } else {
        std::cout << "err; no file provided to assemble" << std::endl;
    }
    return 0;
}