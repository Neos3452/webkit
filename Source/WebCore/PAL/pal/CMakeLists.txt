set(PAL_SOURCES
)

set(PAL_INCLUDE_DIRECTORIES
    "${PAL_DIR}"
)

set(PAL_PRIVATE_INCLUDE_DIRECTORIES
    "${PAL_DIR}/pal"
    "${PAL_DIR}/pal/crypto"
    "${PAL_DIR}/pal/system"
)

set(PAL_LIBRARIES
    WTF
)

WEBKIT_INCLUDE_CONFIG_FILES_IF_EXISTS()

WEBKIT_WRAP_SOURCELIST(${PAL_SOURCES})
WEBKIT_FRAMEWORK(PAL)
