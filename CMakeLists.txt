cmake_minimum_required(VERSION 3.30)

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")

project(Shader-Playground LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_MODULE_STD 1)

add_executable(${PROJECT_NAME})

# Includes
set(INCLUDE_DIRS
	"External/Glfw/include"
	"Source"
	"Source/Core"
)

target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRS})

# Sources
file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "Source/*.cpp")

set(ALL_SOURCES
	${SOURCES}
)

target_sources(${PROJECT_NAME} PUBLIC ${ALL_SOURCES})


# Modules
file(GLOB_RECURSE MODULE_SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "Source/*.cppm")

target_sources(${PROJECT_NAME} PUBLIC
	FILE_SET CXX_MODULES
	BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
	FILES ${MODULE_SOURCES}
)

# Optional
target_compile_features(
	${PROJECT_NAME}
	PRIVATE   cxx_std_23
	INTERFACE cxx_std_20
)


find_package(Vulkan REQUIRED)
if( ${Vulkan_VERSION} VERSION_LESS "1.3.296" )
message( FATAL_ERROR "Minimum required Vulkan version for C++ modules is 1.3.296. Found ${Vulkan_VERSION}.")
endif()

set(VULKAN_HPP_DEFINITIONS
	VULKAN_HPP_NO_EXCEPTIONS
	VULKAN_HPP_RAII_NO_EXCEPTIONS
	VULKAN_HPP_NO_SMART_HANDLE
	VULKAN_HPP_NO_CONSTRUCTORS
	VULKAN_HPP_NO_UNION_CONSTRUCTORS
	VULKAN_HPP_NO_SPACESHIP_OPERATOR
)

# Add Vulkan module
add_library( VulkanHppModule )
target_sources( VulkanHppModule PUBLIC
	FILE_SET CXX_MODULES
	BASE_DIRS ${Vulkan_INCLUDE_DIR}
	FILES ${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm
)
target_compile_features( VulkanHppModule PUBLIC cxx_std_20 )
target_link_libraries( VulkanHppModule PUBLIC Vulkan::Vulkan )

target_link_libraries( ${PROJECT_NAME} PUBLIC VulkanHppModule )
target_compile_definitions(VulkanHppModule PUBLIC ${VULKAN_HPP_DEFINITIONS})


target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)

#Glfw
add_subdirectory(External/Glfw)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

# Compile vertex shader
set(SHADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Shaders)
set(SHADERS_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/Shaders)

function(generate_spv_shader_glsl input_file output_file)
	add_custom_command(
		OUTPUT ${output_file}
		COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${input_file} -fentry-point=main --target-spv=spv1.6 -o ${output_file} -g
		DEPENDS ${input_file}
		COMMENT "Compiling SPIR-V: ${input_file}"
		VERBATIM
		COMMAND_EXPAND_LISTS
	)
endfunction()
generate_spv_shader_glsl(${SHADERS_DIR}/Quad.vert ${SHADERS_OUTPUT_DIR}/Quad.vert.spv)


# cmake . -B Build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS="-stdlib=libc++ -fno-rtti -fno-exceptions" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
# cmake --build Build