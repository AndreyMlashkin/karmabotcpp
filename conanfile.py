from conan import ConanFile


class KarmaBotCppConan(ConanFile):
    name = "karmabotcpp"
    version = "0.1"
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeDeps", "CMakeToolchain"

    default_options = {
        "boost/*:shared": False,
        "boost/*:without_test": True,
        "boost/*:without_math": False,
        "boost/*:without_locale": False,
        "openssl/*:shared": False,
        "curl/*:shared": False,
    }

    def requirements(self):
        self.requires("tgbot/1.9.1")
        self.requires("cpr/1.14.2")
        self.requires("nlohmann_json/3.11.3")
