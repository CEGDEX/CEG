#CEG glue module CmakeLists.txt -- libCEG_glue.a

set(LIB_CEG_GLUE CEG_glue)

#Automatically get src files from the specified directory
aux_source_directory(${CEG_SRC_DIR}/glue/ GLUE_SRC)

#Generate static library files
add_library(${LIB_CEG_GLUE} ${GLUE_SRC})

#Specify compiling options for target objects
target_compile_options(${LIB_CEG_GLUE}
    PUBLIC -std=c++11 
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D_WEBSOCKETPP_CPP11_STL_
    PUBLIC -D${OS_NAME}
)
