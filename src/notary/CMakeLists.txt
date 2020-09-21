#CEG main module CmakeLists.txt -- CEG

set(APP_CEG notary)

set(APP_CEG_SRC
    main.cpp
    configure.cpp
	chain_rpc.cpp
	http_client.cpp
	notary_mgr.cpp
)


set(INNER_LIBS CEG_common CEG_utils CEG_proto)


#Generate executable files
add_executable(${APP_CEG} ${APP_CEG_SRC})
#Specify dependent libraries for target objects
	

MESSAGE(STATUS "current platform: Linux ")  
target_link_libraries(${APP_CEG}
-Wl,-dn ${INNER_LIBS} -Wl,--start-group -Wl,--end-group ${CEG_DEPENDS_LIBS} ${CEG_LINKER_FLAGS})



#Specify compiling options for target objets
target_compile_options(${APP_CEG}
    PUBLIC -std=c++11 
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D_WEBSOCKETPP_CPP11_STL_
    PUBLIC -D${OS_NAME}
)

install(TARGETS ${APP_CEG}
    RUNTIME DESTINATION ${CEGCHAIN}/bin
)
