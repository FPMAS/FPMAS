add_subdirectory(googletest)

set(INSTALL_MOCKS NO CACHE BOOL "Enables mocks install")

add_subdirectory(mocks)
add_subdirectory(local)
add_subdirectory(mpi)
