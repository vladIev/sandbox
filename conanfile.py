from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout


class Template(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def requirements(self):
        self.requires("gtest/1.14.0")
        self.requires("benchmark/1.8.4")

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    