find_package(Qt6 6.5 REQUIRED COMPONENTS
    Quick QuickControls2 Multimedia Network Concurrent Sql LinguistTools
)
qt_standard_project_setup()

# Find CURL via vcpkg (Manifest mode)
find_package(CURL REQUIRED)


