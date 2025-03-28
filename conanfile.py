from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout


class Template(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def requirements(self):
        self.requires("gtest/1.14.0")
        self.requires("benchmark/1.8.4")
        self.requires("boost/1.86.0")
        self.requires("openssl/3.3.2")
        self.requires("nlohmann_json/3.11.3")

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    