# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Tools

# Genesis block miner
add_executable(mine_genesis mine_genesis.cpp)
target_link_libraries(mine_genesis PRIVATE intcoin_core)
target_compile_features(mine_genesis PRIVATE cxx_std_23)

# Install tools
install(TARGETS mine_genesis
    RUNTIME DESTINATION bin
)
