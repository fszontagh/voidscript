# Modules/StableDiffusion/CMakeLists.txt
#
# Wraps stable-diffusion.cpp. Heavy and opt-in: needs a prebuilt stable-diffusion.cpp
# (built with -DCMAKE_POSITION_INDEPENDENT_CODE=ON so its static libs link into this .so)
# and CUDA. Point SDCPP_ROOT at that checkout; its build/ dir must contain the archives.

if (NOT BUILD_MODULE_STABLEDIFFUSION)
    return()
endif()

project(
    modules-stablediffusion
    VERSION 0.0.1
    DESCRIPTION "Stable Diffusion (stable-diffusion.cpp) module for voidscript"
    LANGUAGES CXX
    HOMEPAGE_URL "https://github.com/fszontagh/voidshell"
)

set(SDCPP_ROOT "/data/stable-diffusion.cpp" CACHE PATH "Path to a built stable-diffusion.cpp checkout")
set(SDCPP_BUILD "${SDCPP_ROOT}/build" CACHE PATH "stable-diffusion.cpp build directory")

if (NOT EXISTS "${SDCPP_ROOT}/include/stable-diffusion.h")
    message(FATAL_ERROR "StableDiffusion module: SDCPP_ROOT='${SDCPP_ROOT}' has no include/stable-diffusion.h")
endif()

find_package(CUDAToolkit REQUIRED)
find_package(OpenMP)

# The static archives, dependents before dependencies (matters for static linking).
set(_sd_main "${SDCPP_BUILD}/libstable-diffusion.a")
file(GLOB _sd_ggml "${SDCPP_BUILD}/ggml/src/libggml*.a")
file(GLOB _sd_cuda "${SDCPP_BUILD}/ggml/src/ggml-cuda/libggml-cuda.a")
file(GLOB _sd_webp "${SDCPP_BUILD}/thirdparty/libwebp/*.a")

if (NOT EXISTS "${_sd_main}")
    message(FATAL_ERROR "StableDiffusion module: '${_sd_main}' not found - build stable-diffusion.cpp first "
                        "(cmake -S ${SDCPP_ROOT} -B ${SDCPP_BUILD} -DSD_CUDA=ON -DCMAKE_POSITION_INDEPENDENT_CODE=ON && cmake --build ${SDCPP_BUILD})")
endif()

add_library(voidscript-module-stablediffusion SHARED)
target_sources(voidscript-module-stablediffusion
    PRIVATE
        src/SDModule.cpp
        src/PluginInit.cpp
)

target_include_directories(voidscript-module-stablediffusion PRIVATE
    "${SDCPP_ROOT}/include"
    "${SDCPP_ROOT}/thirdparty"     # stb_image.h / stb_image_write.h
    "${SDCPP_ROOT}/ggml/include"
)

# ggml registers each backend (CPU, CUDA) via static initialisers / *_reg symbols that
# nothing in this module references directly. Without --whole-archive the linker drops
# those objects, so at runtime the module fails with e.g. undefined ggml_backend_cpu_reg.
# Whole-archive the entire ggml group to keep the registrations.
target_link_libraries(voidscript-module-stablediffusion PRIVATE
    "${_sd_main}"
    -Wl,--whole-archive ${_sd_ggml} ${_sd_cuda} -Wl,--no-whole-archive
    ${_sd_webp}
    CUDA::cudart
    CUDA::cublas
    CUDA::cuda_driver   # ggml-cuda uses the Driver API (cuMemCreate, VMM)
    $<$<TARGET_EXISTS:OpenMP::OpenMP_CXX>:OpenMP::OpenMP_CXX>
)

install(TARGETS voidscript-module-stablediffusion
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}/Modules
        COMPONENT "modules-stablediffusion")
