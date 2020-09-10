if(${INSTALL_MOCKS})
	install(DIRECTORY . DESTINATION include/fpmas/mocks FILES_MATCHING PATTERN "*.h")
endif()
