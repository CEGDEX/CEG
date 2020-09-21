cmake_minimum_required(VERSION 2.8)

project(CEG)
add_compile_options(-g -O2)
add_definitions(-DSVNVERSION=\"${SVNVERSION}\")
message(STATUS "-DSVNVERSION="${SVNVERSION})

set( V8_LIB_PATH "linux")
set( OS_NAME "OS_LINUX")
IF (CMAKE_SYSTEM_NAME MATCHES "Linux")  
    MESSAGE(STATUS "current platform: Linux ")  
	set( OS_NAME "OS_LINUX")
	set(CEG_LINKER_FLAGS "-Wl,-dy -lpthread -lrt -ldl")
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Darwin")  
    MESSAGE(STATUS "current platform: Darwin")  
	set( OS_NAME "OS_MAC")
	set(CEG_LINKER_FLAGS "-Wl, -lpthread -ldl")
	set( V8_LIB_PATH "mac")
ELSE ()  
    MESSAGE(STATUS "other platform: ${CMAKE_SYSTEM_NAME}")  
ENDIF ()  

set(CEGCHAIN buchain)
set(CEG_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CEG_ROOT_DIR ${CEG_SRC_DIR}/..)
set(LIBRARY_OUTPUT_PATH ${CEG_ROOT_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${CEG_ROOT_DIR}/bin)

set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)

execute_process(COMMAND make all WORKING_DIRECTORY ${CEG_SRC_DIR}/3rd)

include_directories(
    ${CEG_SRC_DIR}
    ${CEG_SRC_DIR}/3rd/basic/include/v8
    ${CEG_SRC_DIR}/3rd/protobuf/src
    ${CEG_SRC_DIR}/3rd/jsoncpp/include
    ${CEG_SRC_DIR}/3rd/sqlite
    ${CEG_SRC_DIR}/3rd/rocksdb/include
    ${CEG_SRC_DIR}/3rd/pcre-8.39
    ${CEG_SRC_DIR}/3rd/websocketpp
    ${CEG_SRC_DIR}/3rd/bzip2-1.0.6
    ${CEG_SRC_DIR}/3rd/zlib-1.2.8
    ${CEG_SRC_DIR}/3rd/asio/include
    ${CEG_SRC_DIR}/3rd/openssl/include
    ${CEG_SRC_DIR}/3rd/http
    ${CEG_SRC_DIR}/3rd/libscrypt
)

link_directories(
    ${CEG_SRC_DIR}/3rd/protobuf/src/.libs
    ${CEG_SRC_DIR}/3rd/rocksdb
    ${CEG_SRC_DIR}/3rd/pcre-8.39/.libs
    ${CEG_SRC_DIR}/3rd/jsoncpp/libs
    ${CEG_SRC_DIR}/3rd/openssl
    ${CEG_SRC_DIR}/3rd/zlib-1.2.8
    ${CEG_SRC_DIR}/3rd/libscrypt
    ${CEG_SRC_DIR}/3rd/bzip2-1.0.6
    ${CEG_SRC_DIR}/3rd/basic/lib
    ${CEG_SRC_DIR}/3rd/curl/lib/.libs/
    ${CEG_SRC_DIR}/3rd/v8_target/${V8_LIB_PATH}/
    ${LIBRARY_OUTPUT_PATH}
)

set(CEG_DEPENDS_LIBS protobuf rocksdb pcreposix pcrecpp pcre json ssl crypto z bz2 scrypt)

add_subdirectory(3rd/http)
add_subdirectory(3rd/ed25519-donna)
add_subdirectory(glue)
add_subdirectory(utils)
add_subdirectory(proto)
add_subdirectory(contract)
add_subdirectory(ledger)
add_subdirectory(common)
add_subdirectory(libCEG_tools)
add_subdirectory(overlay)
add_subdirectory(consensus)
add_subdirectory(monitor)
add_subdirectory(main)

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")  
	add_subdirectory(daemon)
	set(CEG_SCRIPTS ${CEG_ROOT_DIR}/deploy)
	install(
		PROGRAMS ${CEG_SCRIPTS}/CEG ${CEG_SCRIPTS}/CEGd ${CEG_SCRIPTS}/start-stop-daemon
		DESTINATION ${CEGCHAIN}/scripts
	)
ENDIF ()  

install(
    DIRECTORY ${CEG_ROOT_DIR}/bin/
    DESTINATION ${CEGCHAIN}/bin
    FILES_MATCHING PATTERN "*.bin" PATTERN "*.dat"
)

install(
    DIRECTORY ${CEG_ROOT_DIR}/build/win32/config/
    DESTINATION ${CEGCHAIN}/config
    FILES_MATCHING PATTERN "CEG.json" PATTERN "CEG-mainnet.json" PATTERN "CEG-testnet.json" PATTERN "CEG-single.json"
)

install(
    DIRECTORY ${CEG_ROOT_DIR}/build/win32/jslib/
    DESTINATION ${CEGCHAIN}/jslib
    FILES_MATCHING PATTERN "*.js"
)

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")  
	set(SUPPLEMENT ${CEG_ROOT_DIR}/build/linux/MakeSupplement)
	set(FIX_DIR ${CMAKE_INSTALL_PREFIX}/${CEGCHAIN})
	message(STATUS "install dir: ${FIX_DIR}")

	file(WRITE ${SUPPLEMENT} "soft_link:\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s ${FIX_DIR}/scripts/CEG /etc/init.d/CEG;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s ${FIX_DIR}/scripts/CEGd /etc/init.d/CEGd;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/CEG /etc/rc1.d/S99CEG;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/CEG /etc/rc1.d/S99CEG;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/CEG /etc/rc2.d/S99CEG;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/CEG /etc/rc3.d/S99CEG;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/CEG /etc/rc4.d/S99CEG;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/CEG /etc/rc5.d/S99CEG;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/CEGd /etc/rc1.d/S50CEGd;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/CEGd /etc/rc1.d/S50CEGd;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/CEGd /etc/rc2.d/S50CEGd;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/CEGd /etc/rc3.d/S50CEGd;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/CEGd /etc/rc4.d/S50CEGd;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/CEGd /etc/rc5.d/S50CEGd;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo mkdir -p ${FIX_DIR}/data;\n")
	file(APPEND ${SUPPLEMENT} "\tsudo mkdir -p ${FIX_DIR}/coredump;\n")

	file(APPEND ${SUPPLEMENT} "\nuninstall:\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc1.d/S99CEG;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc2.d/S99CEG;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc3.d/S99CEG;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc4.d/S99CEG;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc5.d/S99CEG;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/init.d/CEG;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc1.d/S50CEGd;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc2.d/S50CEGd;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc3.d/S50CEGd;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc4.d/S50CEGd;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc5.d/S50CEGd;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/init.d/CEGd;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -rf ${FIX_DIR}/bin;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -rf ${FIX_DIR}/cfca;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -rf ${FIX_DIR}/config;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -rf ${FIX_DIR}/scripts\n")
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Darwin")  
    set(SUPPLEMENT ${CEG_ROOT_DIR}/build/mac/MakeSupplement)
	set(FIX_DIR ${CMAKE_INSTALL_PREFIX}/${CEGCHAIN})
	message(STATUS "install dir: ${FIX_DIR}")

	file(WRITE ${SUPPLEMENT} "soft_link:\n")
	file(APPEND ${SUPPLEMENT} "\tsudo mkdir -p ${FIX_DIR}/data;\n")
	file(APPEND ${SUPPLEMENT} "\tsudo mkdir -p ${FIX_DIR}/coredump;\n")

	file(APPEND ${SUPPLEMENT} "\nuninstall:\n")
	file(APPEND ${SUPPLEMENT} "\trm -rf ${FIX_DIR}/bin;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -rf ${FIX_DIR}/cfca;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -rf ${FIX_DIR}/config;\\\n")
ELSE ()  
    MESSAGE(STATUS "other platform: ${CMAKE_SYSTEM_NAME}")  
ENDIF ()  
