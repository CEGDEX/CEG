#CEG main module CmakeLists.txt -- CEG

set(APP_CEG CEG)

set(APP_CEG_SRC
    main.cpp
    configure.cpp
    ../api/web_server.cpp
    ../api/web_server_query.cpp
    ../api/web_server_update.cpp
    ../api/web_server_command.cpp
    ../api/web_server_helper.cpp
    ../api/websocket_server.cpp
    ../api/console.cpp
)

set(INNER_LIBS CEG_glue CEG_contract CEG_ledger CEG_consensus CEG_overlay CEG_common CEG_utils CEG_proto CEG_http CEG_ed25519 CEG_monitor)
set(V8_LIBS v8_base v8_libbase v8_external_snapshot v8_libplatform v8_libsampler icui18n icuuc inspector)

#Generate executable files
add_executable(${APP_CEG} ${APP_CEG_SRC})

#Specify dependent libraries for target objects
	
IF (${OS_NAME} MATCHES "OS_LINUX")  
    MESSAGE(STATUS "current platform: Linux ")  
	target_link_libraries(${APP_CEG}
    -Wl,-dn ${INNER_LIBS} -Wl,--start-group ${V8_LIBS} -Wl,--end-group ${CEG_DEPENDS_LIBS} ${CEG_LINKER_FLAGS})
ELSE ()  
	MESSAGE(STATUS "current platform: MAC ")  
	add_definitions(${CEG_LINKER_FLAGS})
	target_link_libraries(${APP_CEG} ${INNER_LIBS} ${V8_LIBS} ${CEG_DEPENDS_LIBS})
ENDIF () 


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
