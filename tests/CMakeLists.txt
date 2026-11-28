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

add_executable(photo-snap-phase1-tests
    phase1_document_test.cpp
    ../src/core/ImageDocument.cpp
    ../src/core/ImageDocument.h
    ../src/core/PreviewRenderer.cpp
    ../src/core/PreviewRenderer.h
)

target_include_directories(photo-snap-phase1-tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(photo-snap-phase1-tests
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME photo-snap-phase1-document COMMAND photo-snap-phase1-tests)
