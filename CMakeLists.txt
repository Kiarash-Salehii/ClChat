# CMakeList.txt : CMake project for ClChat, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("ClChat")

# Add source to this project's executable.
add_executable (ClChat "ClChat.c" "ClChat.h" "main.c" "chat_core.c" "chat_core.h" "http_client.c" "chat_packet.h" "chat_packet.c" "base32_6.h" "base32_6.c" "win32_thread.c" "linux_thread.c" "thread.h" "color_printer.h" "color_printer.c" "mutex.h" "mutex.c" "console.h" "linux_console.c" "win32_console.c" "terminal.h" "terminal.c" "server.c" "server.h" "client.c" "client.h" "logger.h" "logger.c" "image.h" "image.c" "net_interface.c" "net_interface.h" "tui_selector.c" "tui_selector.h" "atomic.h" "socket.h" "vector.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ClChat PROPERTY CXX_STANDARD 20)
endif()

if(DEFINED WINDOWS OR DEFINED WIN32)
	target_link_libraries(ClChat PUBLIC ws2_32 user32 iphlpapi)
else()
	target_link_libraries(ClChat PUBLIC pthread)
endif()
# TODO: Add tests and install targets if needed.
