if(WIN32)
	if ( "${CMAKE_SIZEOF_VOID_P}" EQUAL "8" )
		set(BOOST_ADDRESS_MODEL 64)
		set(PYTHON_ARCH x64)
		set(PYTHON_ARCH2 win-AMD64)
		set(PYTHON_OUTPUTDIR ${CMAKE_CURRENT_BINARY_DIR}/build/python/src/external_python/pcbuild/amd64/)
	else()
		set(BOOST_ADDRESS_MODEL 32)
		set(PYTHON_ARCH x86)
		set(PYTHON_ARCH2 win32)
		set(PYTHON_OUTPUTDIR ${CMAKE_CURRENT_BINARY_DIR}/build/python/src/external_python/pcbuild/win32/)
	endif()
	if (MSVC12)
		set(BOOST_TOOLSET toolset=msvc-12.0)
		set(BOOST_COMPILER_STRING -vc120)
		set(PYTHON_COMPILER_STRING v120)
	endif()
	if (MSVC14)
		set(BOOST_TOOLSET toolset=msvc-14.0)
		set(BOOST_COMPILER_STRING -vc140)
		set(PYTHON_COMPILER_STRING v140)
	endif()
	set(JAM_FILE ${CMAKE_CURRENT_BINARY_DIR}/build/boost/src/external_boost/user-config.jam)
	set(semi_path "${CMAKE_CURRENT_SOURCE_DIR}/Diffs/semi.txt")
	FILE(TO_NATIVE_PATH ${semi_path} semi_path)
	set(BOOST_CONFIGURE_COMMAND bootstrap.bat &&
								echo using python : 3.5 : ${PYTHON_OUTPUTDIR}\\python.exe > "${JAM_FILE}"  && 
								echo.   : ${CMAKE_CURRENT_BINARY_DIR}/build/python/src/external_python/include ${CMAKE_CURRENT_BINARY_DIR}/build/python/src/external_python/pc >> "${JAM_FILE}" && 
								echo.   : ${CMAKE_CURRENT_BINARY_DIR}/build/python/src/external_python/pcbuild >> "${JAM_FILE}" &&
								type ${semi_path} >> "${JAM_FILE}"
	)
	set(BOOST_BUILD_COMMAND bjam)
	set(BOOST_BUILD_OPTIONS --user-config=user-config.jam )
else()
	set(BOOST_CONFIGURE_COMMAND ./bootstrap.sh)
	set(BOOST_BUILD_COMMAND ./bjam)
	set(BOOST_BUILD_OPTIONS toolset=clang cxxflags=${PLATFORM_CXXFLAGS} linkflags=${PLATFORM_LDFLAGS})
endif()

set(BOOST_OPTIONS --with-filesystem
					--with-locale
					--with-thread
					--with-regex
					--with-system
					--with-date_time
					--with-wave
					--with-atomic
					--with-serialization
					--with-program_options
					--with-iostreams
					--with-python
					${BOOST_TOOLSET})

string(TOLOWER ${BUILD_MODE} BOOST_BUILD_TYPE)

ExternalProject_Add(external_boost
  URL ${BOOST_URI}
  DOWNLOAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Downloads
  URL_HASH MD5=${BOOST_MD5}
  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/build/boost
  UPDATE_COMMAND  ""
  CONFIGURE_COMMAND ${BOOST_CONFIGURE_COMMAND}
  BUILD_COMMAND ${BOOST_BUILD_COMMAND} ${BOOST_BUILD_OPTIONS} -j${MAKE_THREADS} architecture=x86 address-model=${BOOST_ADDRESS_MODEL} variant=${BOOST_BUILD_TYPE} link=static runtime-link=static threading=multi ${BOOST_OPTIONS}  --prefix=${LIBDIR}/boost install
  BUILD_IN_SOURCE 1
  INSTALL_COMMAND ""
)
