#CEG consensus module CmakeLists.txt -- libCEG_consensus.a

set(LIB_CEG_CONSENSUS CEG_consensus)
#Automatically get src files from the specified directory
aux_source_directory(${CEG_SRC_DIR}/consensus/ CONSENSUS_SRC)

#Generate static library files
add_library(${LIB_CEG_CONSENSUS} ${CONSENSUS_SRC})

#Specify compiling options for target objects
target_compile_options(${LIB_CEG_CONSENSUS}
    PUBLIC -std=c++11 
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D_WEBSOCKETPP_CPP11_STL_
    PUBLIC -D${OS_NAME}
)

