#CEG daemon module CmakeLists.txt

set(DEAMON_DEPENDS_LIB ssl crypto)

# -- CEG_CEGd -- #
set(BIN_CEG_ID CEGd)

add_executable(${BIN_CEG_ID} ./CEG/main.cpp)


IF (${OS_NAME} MATCHES "OS_LINUX")  
    MESSAGE(STATUS "current platform: Linux ")  
	target_link_libraries(${BIN_CEG_ID} -Wl,-dn ${DEAMON_DEPENDS_LIB} ${CEG_LINKER_FLAGS})
ELSE ()  
	MESSAGE(STATUS "current platform: MAC ")  
	target_link_libraries(${BIN_CEG_ID} -Wl, ${DEAMON_DEPENDS_LIB} ${CEG_LINKER_FLAGS})
ENDIF () 



target_compile_options(${BIN_CEG_ID}
    PUBLIC -std=c++11
    PUBLIC -D${OS_NAME}
)

install(TARGETS ${BIN_CEG_ID}
    RUNTIME DESTINATION ${CEGCHAIN}/bin
)

