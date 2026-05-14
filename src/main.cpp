#include <iostream>
#include <string>
#include "mbnwrite.h"
#include "mbnemu.h"

int main(int argc, char** argv) {
    if (argc > 2) {
        assemble_file(argv[1], std::stoull(argv[2]));
    } else if (argc == 2) {
        if (argv[1][strlen(argv[1])-1] == 'n' && argv[1][strlen(argv[1])-2] == 'b' && argv[1][strlen(argv[1])-3] == 'm') {
            exec(std::string(argv[1]));
        } else {
            std::cout << "warn; no dynamic memory requirement specified. assuming zero." << std::endl;
            assemble_file(argv[1], 0);
        }
    } else {
        std::cout << "err; no file provided" << std::endl;
    }
    return 0;
}