
add_custom_target( build_hardfork_hpp
                   COMMAND cat-parts "${CMAKE_CURRENT_SOURCE_DIR}/hardfork.d" "${CMAKE_CURRENT_BINARY_DIR}/include/graphene/chain/hardfork.hpp" )
set_source_files_properties( "${CMAKE_CURRENT_BINARY_DIR}/include/graphene/chain/hardfork.hpp" PROPERTIES GENERATED TRUE )

add_dependencies( build_hardfork_hpp cat-parts )

file(GLOB HEADERS "include/graphene/chain/*.hpp")
file(GLOB PROTOCOL_HEADERS "include/graphene/chain/protocol/*.hpp")

if( GRAPHENE_DISABLE_UNITY_BUILD )
   set( GRAPHENE_DB_FILES
        db_balance.cpp
        db_block.cpp
        db_debug.cpp
        db_getter.cpp
        db_init.cpp
        db_management.cpp
        db_update.cpp
        db_voter.cpp
        db_witness_schedule.cpp
        db_notify.cpp
			db_market.cpp
      )
   message( STATUS "Graphene database unity build disabled" )
else( GRAPHENE_DISABLE_UNITY_BUILD )
   set( GRAPHENE_DB_FILES
        database.cpp )
   message( STATUS "Graphene database unity build enabled" )
endif( GRAPHENE_DISABLE_UNITY_BUILD )

## SORT .cpp by most likely to change / break compile
add_library( graphene_chain

             # As database takes the longest to compile, start it first
             ${GRAPHENE_DB_FILES}
             fork_database.cpp
             exceptions.cpp
	     name.cpp

             protocol/types.cpp
             protocol/authority.cpp
             protocol/asset.cpp
             protocol/account.cpp
             protocol/transfer.cpp
             protocol/csaf.cpp
             protocol/content.cpp
             protocol/committee_member.cpp
             protocol/witness.cpp
             protocol/proposal.cpp
             protocol/asset_ops.cpp
             protocol/memo.cpp
             protocol/operations.cpp
             protocol/transaction.cpp
             protocol/block.cpp
             protocol/chain_parameters.cpp
             protocol/fee_schedule.cpp
             protocol/advertising.cpp
             protocol/custom_vote.cpp
             protocol/balance_lock.cpp
             protocol/pledge_mining.cpp
			 protocol/market.cpp
			 
             genesis_state.cpp
             get_config.cpp

             evaluator.cpp
             account_evaluator.cpp
             csaf_evaluator.cpp
             content_evaluator.cpp
             witness_evaluator.cpp
             committee_member_evaluator.cpp
             asset_evaluator.cpp
             transfer_evaluator.cpp
             proposal_evaluator.cpp
             advertising_evaluator.cpp
             custom_vote_evaluator.cpp
             balance_lock_evaluator.cpp
             pledge_mining_evaluator.cpp
	     market_evaluator.cpp
	     contract_evaluator.cpp
	     apply_context.cpp
             transaction_context.cpp
             wast_to_wasm.cpp
             wasm_interface.cpp
             wasm_validation.cpp
             wasm_injection.cpp

             webassembly/wavm.cpp
             webassembly/binaryen.cpp
             webassembly/wabt.cpp


             account_object.cpp
             asset_object.cpp
             committee_member_object.cpp
             proposal_object.cpp

             block_database.cpp

             is_authorized_asset.cpp
	     abi_serializer.cpp
	     

             ${HEADERS}
             ${PROTOCOL_HEADERS}
             "${CMAKE_CURRENT_BINARY_DIR}/include/graphene/chain/hardfork.hpp"
           )

add_dependencies( graphene_chain build_hardfork_hpp )
target_link_libraries( graphene_chain fc graphene_db Logging IR WAST WASM Runtime wasm
    asmjs passes ast emscripten-optimizer support softfloat builtins wabt)
target_include_directories( graphene_chain
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" 
			    "${CMAKE_CURRENT_SOURCE_DIR}/../wasm-jit/Include"
		           "${CMAKE_CURRENT_SOURCE_DIR}/../../externals/binaryen/src"
		           "${CMAKE_SOURCE_DIR}/libraries/wabt"
		           "${CMAKE_BINARY_DIR}/libraries/wabt")

if(MSVC)
  set_source_files_properties( db_init.cpp db_block.cpp database.cpp block_database.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

INSTALL( TARGETS
   graphene_chain

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/graphene/chain" )
INSTALL( FILES ${PROTOCOL_HEADERS} DESTINATION "include/graphene/chain/protocol" )
