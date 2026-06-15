cmake_minimum_required(VERSION 3.16)
project(BeatOven VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Network)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(OpenSSL REQUIRED)

qt_standard_project_setup()

# --- miniupnpc (vendored, static, pinned at miniupnpc_2_3_3) ---
set(UPNPC_BUILD_STATIC ON  CACHE BOOL "" FORCE)
set(UPNPC_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(UPNPC_BUILD_TESTS  OFF CACHE BOOL "" FORCE)
set(UPNPC_BUILD_SAMPLE OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/miniupnpc)

qt_add_executable(BeatOven
    main.cpp
    beatoven.conf
    BeatOvenServer.cpp
    BeatOvenServer.h
    Config.h
    BeatOvenClient.h BeatOvenClient.cpp
    Config.cpp
    TransferWorker.h TransferWorker.cpp
    ProjectFiles.h
    SendWorker.h SendWorker.cpp
    Identity.h Identity.cpp
    ConnectionString.h ConnectionString.cpp
    images.qrc
)

target_link_libraries(BeatOven PRIVATE Qt6::Widgets)
target_link_libraries(BeatOven PRIVATE Qt6::Network)
target_link_libraries(BeatOven PRIVATE Qt6::Core)
target_link_libraries(BeatOven PRIVATE OpenSSL::Crypto)
target_link_libraries(BeatOven PRIVATE libminiupnpc-static)

# Required when linking miniupnpc statically (no-op on Linux, needed on
# Windows so its headers don't declare dllimport symbols)
target_compile_definitions(BeatOven PRIVATE MINIUPNP_STATICLIB)

# --- Phase 1 test harness (console app, no Widgets) ---
qt_add_executable(BeatOvenPhase1Test
    test_phase1.cpp
    Identity.h Identity.cpp
    ConnectionString.h ConnectionString.cpp
)
target_link_libraries(BeatOvenPhase1Test PRIVATE Qt6::Core Qt6::Network OpenSSL::Crypto)
