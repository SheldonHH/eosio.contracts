add_contract(bosio.issuance bosio.issuance ${CMAKE_CURRENT_SOURCE_DIR}/src/bosio.issuance.cpp)
target_include_directories(bosio.issuance.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(bosio.issuance.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
