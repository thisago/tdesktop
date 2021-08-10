# This file is part of Forkgram.

add_library(td_forkgram OBJECT)
init_target(td_forkgram)
add_library(tdesktop::td_forkgram ALIAS td_forkgram)

nice_target_sources(td_forkgram ${src_loc}
PRIVATE
    forkgram/uri_open.cpp
    forkgram/uri_open.h
)

if (WIN32)
    nuget_add_winrt(td_forkgram)
endif()

target_include_directories(td_forkgram
PUBLIC
    ${src_loc}
)

target_link_libraries(td_forkgram
PUBLIC
    tdesktop::td_lang
    desktop-app::lib_base
)
