add_executable(photo-snap-phase0-tests
    phase0_baseline_test.cpp
)

target_link_libraries(photo-snap-phase0-tests
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME photo-snap-phase0-baseline COMMAND photo-snap-phase0-tests)
