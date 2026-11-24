from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain


class RpmGatewayConan(ConanFile):
    """Pi gateway for the rpm project.

    Builds on Linux (Raspberry Pi, WSL) and Windows (MSVC or MSYS2 MinGW).
    Rule: plain `conan build .` must always succeed (Release, tests off).
    Extras via options/settings, e.g.:
        conan build . -o "&:with_tests=True"
        conan build . -s build_type=Debug

    No dependencies yet. When this joins the shared cmake-base recipe, move the
    layout/generate/build boilerplate there via python_requires.
    """

    name = "rpm_gateway"
    version = "0.1.0"
    settings = "os", "compiler", "build_type", "arch"
    options = {"with_tests": [True, False]}
    default_options = {"with_tests": False}
    exports_sources = "CMakeLists.txt", "src/*", "tests/*"

    def layout(self):
        # Output folders per toolchain, so it is clear which compiler built what:
        #   Linux / WSL GCC:  build/Release_Linux, build/Debug_Linux
        #   MSYS2 MinGW GCC:  build/Release_Mingw, build/Debug_Mingw
        #   MSVC:             build/Windows (one multi-config solution for Debug + Release)
        build_type = str(self.settings.build_type or "Release")
        self.folders.source = "."
        if self.settings.compiler == "msvc":
            self.folders.build = "build/Windows"
        elif self.settings.os == "Windows":
            self.folders.build = f"build/{build_type}_Mingw"
        else:
            self.folders.build = f"build/{build_type}_{self.settings.os}"
        self.folders.generators = f"{self.folders.build}/generators"

    def generate(self):
        tc = CMakeToolchain(self)
        tc.cache_variables["RPM_GATEWAY_TESTS"] = bool(self.options.with_tests)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        if self.options.with_tests:
            cmake.test()
