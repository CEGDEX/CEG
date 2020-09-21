#CEG http module CmakeLists.txt -- libceg_http.a

set(LIB_CEG_HTTP CEG_http)
aux_source_directory(${CEG_SRC_DIR}/3rd/http/ HTTP_SRC)

add_library(${LIB_CEG_HTTP} ${HTTP_SRC})

target_compile_options(${LIB_CEG_HTTP}
    PUBLIC -std=c++11 
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D${OS_NAME}
)
