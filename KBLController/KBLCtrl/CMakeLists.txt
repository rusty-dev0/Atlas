add_executable(KBLCtrl ${CMAKE_CURRENT_LIST_DIR}/main.c)

# Program meta (name/version) and stdio configuration
pico_set_program_name(KBLCtrl "KBLController")
pico_set_program_version(KBLCtrl "0.1")

# Enable/disable stdio as needed
pico_enable_stdio_uart(KBLCtrl 0)
pico_enable_stdio_usb(KBLCtrl 1)

# Link libraries
target_link_libraries(KBLCtrl
        KBLib
)

# Add the package include dir
target_include_directories(KBLCtrl PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(KBLCtrl)