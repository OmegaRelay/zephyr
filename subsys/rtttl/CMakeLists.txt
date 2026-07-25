
if(CONFIG_RTTTL)
    zephyr_library()

    zephyr_library_sources(
      rtttl.c
    )
endif()
