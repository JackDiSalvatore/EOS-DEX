

add_library(EosioTester INTERFACE)
target_link_libraries(EosioTester INTERFACE 
       ${LLVM}
       ${libtester}
       ${libchain}
       ${libfc}
       ${libwast}
       ${libwasm}
       ${libwabt}
       ${libruntime}
       ${libplatform}
       ${libir}
       ${libsoftfloat}
       ${liboscrypto}
       ${libosssl}
       ${liblogging}
       ${libchainbase}
       ${libbuiltins}
       ${libsecp256k1}
       ${GMP_LIBRARIES}

       LLVMX86Disassembler
       LLVMX86AsmParser
       LLVMX86AsmPrinter
       LLVMX86CodeGen

       LLVMSelectionDAG

       LLVMDebugInfoDWARF
       LLVMAsmPrinter
       LLVMMCParser
       LLVMX86Info

       LLVMOrcJIT
       LLVMExecutionEngine

       LLVMCodeGen
       LLVMScalarOpts
       LLVMTransformUtils

       LLVMipo
       LLVMAnalysis
       LLVMTarget
       LLVMMC
       LLVMCore
       LLVMSupport
       ${Boost_LIBRARIES}
 
       ${PLATFORM_SPECIFIC_LIBS}
)


target_include_directories(EosioTester INTERFACE 
                               ${Boost_INCLUDE_DIRS}
                               /usr/local/opt/openssl/include
                               /usr/local
                               /usr/local/include
                               /usr/local/include/wasm-jit
                               /usr/local/include/softfloat
)


macro(add_eosio_test test_name)
   add_executable( ${test_name} ${ARGN} )
   target_link_libraries( ${test_name} PRIVATE EosioTester)

   #Manually run unit_test for all supported runtimes
   #To run unit_test with all log from blockchain displayed, put --verbose after --, i.e. unit_test -- --verbose
   add_test(NAME ${test_name}_binaryen COMMAND ${test_name}
    --report_level=detailed --color_output -- --binaryen)
   add_test(NAME ${test_name}_wavm COMMAND ${test_name}
    --report_level=detailed --color_output --catch_system_errors=no -- --wavm)
endmacro()


function(add_eosio_contract_test testname) 
    set(CONTRACTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../contracts)
    add_executable( ${testname} ${ARGN} )
    target_compile_definitions(${testname} PRIVATE O1TEST)
    target_link_libraries(${testname} PRIVATE  EosioTester)
    target_include_directories(${testname} PRIVATE ${CONTRACTS_DIR}/token.exchange/src ${CONTRACTS_DIR}/token.exchange/include)
    add_test(NAME ${testname}
            COMMAND ./${testname}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endfunction(add_eosio_contract_test)

function(add_eosio_contract_action_test testname) 
    add_executable( ${testname} ${ARGN} )
    target_link_libraries(${testname} PRIVATE  EosioTester)
    target_include_directories(${testname} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
    add_test(NAME ${testname}
            COMMAND ./${testname}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endfunction(add_eosio_contract_action_test)