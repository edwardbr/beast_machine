hunter_config(Boost VERSION 1.70.0-p0)
hunter_config(OpenSSL VERSION 1.1.0j)

hunter_config(Catch2
    URL https://github.com/catchorg/Catch2/archive/v2.7.1.tar.gz
    SHA1 45b3f4ad38f3a5cace6edabd42099de740185237
    CMAKE_ARGS
        CATCH_BUILD_TESTING=OFF
        CATCH_ENABLE_WERROR=OFF
        CATCH_INSTALL_DOCS=OFF
        CATCH_INSTALL_HELPERS=ON
)
