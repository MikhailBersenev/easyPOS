cmake_minimum_required(VERSION 3.19)
project(easyPOS LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Sql PrintSupport Test LinguistTools)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Core)

qt_standard_project_setup()

qt_add_executable(easyPOS
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    ui/authwindow.h ui/authwindow.cpp ui/authwindow.ui
    ui/setupwizard.h ui/setupwizard.cpp
    ui/setupdbpage.h ui/setupdbpage.cpp ui/setupdbpage.ui
    db/structures.h
    db/databaseconnection.h db/databaseconnection.cpp
    RBAC/structures.h
    RBAC/authmanager.h RBAC/authmanager.cpp
    RBAC/accountmanager.h RBAC/accountmanager.cpp
    RBAC/rolemanager.h RBAC/rolemanager.cpp
    easyposcore.h easyposcore.cpp
    ui/signupwindow.h ui/signupwindow.cpp ui/signupwindow.ui
    ui/windowcreator.h
    ui/referencedialog.h ui/referencedialog.cpp ui/referencedialog.ui
    ui/goodcatsdialog.h ui/goodcatsdialog.cpp
    ui/categoryeditdialog.h ui/categoryeditdialog.cpp
    ui/goodsdialog.h ui/goodsdialog.cpp
    ui/goodeditdialog.h ui/goodeditdialog.cpp
    ui/serviceeditdialog.h ui/serviceeditdialog.cpp
    ui/vatrateeditdialog.h ui/vatrateeditdialog.cpp
    ui/discountdialog.h ui/discountdialog.cpp
    ui/dbsettingsdialog.h ui/dbsettingsdialog.cpp ui/dbsettingsdialog.ui
    ui/checkhistorydialog.h ui/checkhistorydialog.cpp
    ui/salesreportdialog.h ui/salesreportdialog.cpp
    ui/checkdetailsdialog.h ui/checkdetailsdialog.cpp
    ui/checkpdfexporter.h ui/checkpdfexporter.cpp
    ui/servicesdialog.h ui/servicesdialog.cpp
    ui/vatratesdialog.h ui/vatratesdialog.cpp
    goods/structures.h
    goods/categorymanager.h goods/categorymanager.cpp
    settings/settingsmanager.h settings/settingsmanager.cpp
    sales/structures.h
    sales/salesmanager.h sales/salesmanager.cpp
    sales/stockmanager.h sales/stockmanager.cpp
    ui/sales/goodfind.h ui/sales/goodfind.cpp ui/sales/goodfind.ui



)

# qt_add_translations(
#     easyPOS
#     TS_FILES easyPOS_ru_RU.ts
# )
# Примечание: временно отключено из-за проблемы с путями в Qt6 CMake

target_link_libraries(easyPOS
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Sql
        Qt6::PrintSupport
)
target_link_libraries(easyPOS PRIVATE Qt6::Core)
target_link_libraries(easyPOS PRIVATE Qt6::Core)
target_link_libraries(easyPOS PRIVATE Qt6::Core)
target_link_libraries(easyPOS PRIVATE Qt6::Core)
target_link_libraries(easyPOS PRIVATE Qt6::Core)
target_link_libraries(easyPOS PRIVATE Qt6::Core)
target_link_libraries(easyPOS PRIVATE Qt6::Core)

include(GNUInstallDirs)

install(TARGETS easyPOS
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET easyPOS
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

# Тесты
enable_testing()
qt_add_executable(unit_tests
    unit_tests/main.cpp
    unit_tests/test_databaseconnection.cpp
    unit_tests/test_databaseconnection.h
    unit_tests/test_AuthManager.h unit_tests/test_AuthManager.cpp
    unit_tests/test_AccountManager.h unit_tests/test_AccountManager.cpp
    unit_tests/test_RoleManager.h unit_tests/test_RoleManager.cpp
    unit_tests/test_StockManager.h unit_tests/test_StockManager.cpp
    unit_tests/test_SalesManager.h unit_tests/test_SalesManager.cpp
    unit_tests/test_SettingsManager.h unit_tests/test_SettingsManager.cpp
    db/databaseconnection.cpp
    db/databaseconnection.h
    db/structures.h
    RBAC/structures.h
    RBAC/authmanager.h RBAC/authmanager.cpp
    RBAC/accountmanager.h RBAC/accountmanager.cpp
    RBAC/rolemanager.h RBAC/rolemanager.cpp
    sales/stockmanager.h sales/stockmanager.cpp
    sales/salesmanager.h sales/salesmanager.cpp
    settings/settingsmanager.h settings/settingsmanager.cpp
    easyposcore.cpp easyposcore.h
)

target_link_libraries(unit_tests
    PRIVATE
        Qt::Core
        Qt::Test
        Qt::Sql
)

add_test(NAME DatabaseConnectionTests COMMAND unit_tests)
