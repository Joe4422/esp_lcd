set(COMPONENT_ADD_INCLUDEDIRS ".")
idf_component_register(SRCS "storage_manager.c"
                       INCLUDE_DIRS "."
                       REQUIRES "nvs_flash")