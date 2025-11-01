// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    std::cout << "INTcoin CLI v0.1.0-alpha" << std::endl;
    std::cout << "Copyright (c) 2025 INTcoin Core (Maddison Lane)" << std::endl;
    std::cout << std::endl;

    if (argc < 2) {
        std::cout << "Usage: intcoin-cli <command> [params]" << std::endl;
        std::cout << std::endl;
        std::cout << "This is a development build." << std::endl;
        std::cout << "RPC functionality coming in future releases." << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Command: " << argv[1] << std::endl;
    std::cout << "This is a development build - command not yet implemented." << std::endl;

    return EXIT_SUCCESS;
}
