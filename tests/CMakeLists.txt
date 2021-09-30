add_subdirectory(googletest)

option(FPMAS_INSTALL_MOCKS "Enables mocks installation." OFF)

add_subdirectory(mocks)
add_subdirectory(local)
add_subdirectory(mpi)
