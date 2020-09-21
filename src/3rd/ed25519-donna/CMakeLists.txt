#CEG ed25519 module CmakeLists.txt -- libCEG_ed25519.a

set(LIB_CEG_ED25519 CEG_ed25519)
aux_source_directory(${CEG_SRC_DIR}/3rd/ed25519-donna/ ED25519_SRC)
add_library(${LIB_CEG_ED25519} ${ED25519_SRC})

target_compile_options(${LIB_CEG_ED25519}
	PUBLIC -DASIO_STANDALONE 
	PUBLIC -fPIC
)
