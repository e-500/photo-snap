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
    ../src/core/ImageTransform.cpp
    ../src/core/ImageTransform.h
    ../src/core/ImageFilters.cpp
    ../src/core/ImageFilters.h
)

target_include_directories(photo-snap-phase1-tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(photo-snap-phase1-tests
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME photo-snap-phase1-document COMMAND photo-snap-phase1-tests)

add_executable(photo-snap-phase2-tests
    phase2_transform_test.cpp
    ../src/core/ImageTransform.cpp
    ../src/core/ImageTransform.h
)

target_include_directories(photo-snap-phase2-tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(photo-snap-phase2-tests
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME photo-snap-phase2-transform COMMAND photo-snap-phase2-tests)

add_executable(photo-snap-phase3-tests
    phase3_adjustment_test.cpp
    ../src/core/ImageTransform.cpp
    ../src/core/ImageTransform.h
    ../src/core/ImageFilters.cpp
    ../src/core/ImageFilters.h
    ../src/core/PreviewRenderer.cpp
    ../src/core/PreviewRenderer.h
)

target_include_directories(photo-snap-phase3-tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(photo-snap-phase3-tests
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME photo-snap-phase3-adjustments COMMAND photo-snap-phase3-tests)

add_executable(photo-snap-phase4-tests
    phase4_filter_test.cpp
    ../src/core/ImageFilters.cpp
    ../src/core/ImageFilters.h
)

target_include_directories(photo-snap-phase4-tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(photo-snap-phase4-tests
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME photo-snap-phase4-filters COMMAND photo-snap-phase4-tests)

add_executable(photo-snap-phase5-tests
    phase5_undo_test.cpp
    ../src/core/ImageDocument.cpp
    ../src/core/ImageDocument.h
    ../src/commands/AdjustmentCommand.cpp
    ../src/commands/AdjustmentCommand.h
)

target_include_directories(photo-snap-phase5-tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(photo-snap-phase5-tests
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME photo-snap-phase5-undo COMMAND photo-snap-phase5-tests)

add_executable(photo-snap-phase6-tests
    phase6_export_test.cpp
    ../src/core/ImageExporter.cpp
    ../src/core/ImageExporter.h
    ../src/core/PreviewRenderer.cpp
    ../src/core/PreviewRenderer.h
    ../src/core/ImageTransform.cpp
    ../src/core/ImageTransform.h
    ../src/core/ImageFilters.cpp
    ../src/core/ImageFilters.h
)

target_include_directories(photo-snap-phase6-tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(photo-snap-phase6-tests
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME photo-snap-phase6-export COMMAND photo-snap-phase6-tests)

add_executable(photo-snap-phase7-tests
    phase7_benchmark_test.cpp
    ../src/core/ImageTransform.cpp
    ../src/core/ImageTransform.h
    ../src/core/ImageFilters.cpp
    ../src/core/ImageFilters.h
    ../src/core/PreviewRenderer.cpp
    ../src/core/PreviewRenderer.h
)

target_include_directories(photo-snap-phase7-tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(photo-snap-phase7-tests
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME photo-snap-phase7-benchmarks COMMAND photo-snap-phase7-tests)
