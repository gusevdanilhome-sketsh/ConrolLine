cmake_minimum_required(VERSION 3.19)
project(ConrolLine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EIGEN_ROOT "C:/Project/Libs/eigen-3.4.0")
include_directories(${EIGEN_ROOT})

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets LinguistTools Charts)

qt_standard_project_setup()

qt_add_executable(ConrolLine
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    models/line_calculator.cpp
    models/line_calculator.h
    models/microstrip_full.cpp
    models/microstrip_full.h
    data/data_generator.cpp
    data/data_generator.h
    data/data_loader.cpp
    data/data_loader.h
    classifiers/logistic_regression.cpp
    classifiers/logistic_regression.h
    classifiers/lda.cpp
    classifiers/lda.h
    classifiers/naive_bayes.cpp
    classifiers/naive_bayes.h
    classifiers/bayesian_classifier.cpp
    classifiers/bayesian_classifier.h
    classifiers/classifier_base.h
    utils/complex_plot.cpp
    utils/complex_plot.h
    utils/matrix_utils.cpp
    utils/matrix_utils.h
    utils/sensitivity_analysis.cpp
    utils/sensitivity_analysis.h
    utils/metrics.cpp
    utils/metrics.h
)

target_include_directories(ConrolLine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_translations(
    TARGETS ConrolLine
    TS_FILES ConrolLine_ru_RU.ts
)

target_link_libraries(ConrolLine
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Charts
)

install(TARGETS ConrolLine
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET ConrolLine
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})