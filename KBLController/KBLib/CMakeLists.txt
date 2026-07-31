add_library(KBLib STATIC src/driver.c)

target_include_directories(KBLib PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/includes
)

target_link_libraries(KBLib
        pico_stdlib
        hardware_pwm
        hardware_gpio
)