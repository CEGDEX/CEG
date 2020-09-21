#CEG tools module CmakeLists.txt -- libCEGtools.so

set(LIB_CEG_TOOLS CEGtools)
set(TOOLS_SRC
    lib_CEG_tools.cpp
)

set(INNER_TOOLS_LIBS CEG_common CEG_utils CEG_ed25519)
set(CEG_TOOLS_DEPENDS_LIBS json ssl crypto scrypt)


#Generate share library files
add_library(${LIB_CEG_TOOLS} SHARED ${TOOLS_SRC})

target_link_libraries(${LIB_CEG_TOOLS} ${INNER_TOOLS_LIBS} ${CEG_TOOLS_DEPENDS_LIBS})

#Specify compiling options for target objects
target_compile_options(${LIB_CEG_TOOLS}
    PUBLIC -std=c++11 
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D_WEBSOCKETPP_CPP11_STL_
    PUBLIC -D${OS_NAME}
)
