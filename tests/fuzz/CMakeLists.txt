# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Fuzz Testing

# Check if fuzzing is enabled
if(ENABLE_FUZZ_TESTING)
    # Find libFuzzer or AFL
    set(FUZZ_ENGINE "libfuzzer" CACHE STRING "Fuzzing engine: libfuzzer or afl")

    # Compiler flags for fuzzing
    if(FUZZ_ENGINE STREQUAL "libfuzzer")
        set(FUZZ_FLAGS "-fsanitize=fuzzer,address,undefined")
    elseif(FUZZ_ENGINE STREQUAL "afl")
        set(CMAKE_CXX_COMPILER "afl-clang++")
        set(FUZZ_FLAGS "-fsanitize=address,undefined")
    endif()

    # Fuzz targets
    set(FUZZ_TARGETS
        fuzz_transaction
        fuzz_block
        fuzz_p2p_message
        fuzz_script
        fuzz_rpc
        fuzz_wallet
        fuzz_contract
        fuzz_lightning
        fuzz_bridge
        fuzz_pqc
        fuzz_mempool
        fuzz_consensus
    )

    foreach(target ${FUZZ_TARGETS})
        add_executable(${target} ${target}.cpp)

        target_link_libraries(${target}
            PRIVATE
                intcoin_core
        )

        target_compile_options(${target}
            PRIVATE
                ${FUZZ_FLAGS}
        )

        target_link_options(${target}
            PRIVATE
                ${FUZZ_FLAGS}
        )

        # Add to fuzz testing group
        add_custom_target(run_${target}
            COMMAND ${target} -max_total_time=60 corpus_${target}/
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Running ${target} fuzzer"
        )
    endforeach()

    # Target to run all fuzz tests
    add_custom_target(fuzz_all
        DEPENDS ${FUZZ_TARGETS}
        COMMENT "Running all fuzz tests"
    )

    # Create corpus directories
    foreach(target ${FUZZ_TARGETS})
        file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/corpus_${target})
    endforeach()

    message(STATUS "Fuzz testing enabled with ${FUZZ_ENGINE}")
else()
    message(STATUS "Fuzz testing disabled (use -DENABLE_FUZZ_TESTING=ON)")
endif()
