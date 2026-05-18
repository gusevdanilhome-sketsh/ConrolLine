cmake_minimum_required(VERSION 3.19)
project(ConrolLine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets LinguistTools)

qt_standard_project_setup()

qt_add_executable(ConrolLine
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    models/line_calculator.cpp
    models/line_calculator.h
    data/data_generator.cpp
    data/data_generator.h
    classifiers/logistic_regression.cpp
    classifiers/logistic_regression.h
    classifiers/lda.cpp
    classifiers/lda.h
    classifiers/naive_bayes.cpp
    classifiers/naive_bayes.h
    classifiers/classifier_base.h
)

# Добавляем корневую директорию в пути поиска заголовков
target_include_directories(ConrolLine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_translations(
    TARGETS ConrolLine
    TS_FILES ConrolLine_ru_RU.ts
)

target_link_libraries(ConrolLine
    PRIVATE
        Qt::Core
        Qt::Widgets
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