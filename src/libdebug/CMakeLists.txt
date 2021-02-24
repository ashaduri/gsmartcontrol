
add_library(libdebug STATIC)

target_sources(libdebug PRIVATE
	dchannel.cpp
	dchannel.h
	dcmdarg.cpp
	dcmdarg.h
	dexcept.h
	dflags.cpp
	dflags.h
	dout.cpp
	dout.h
	dstate.cpp
	dstate.h
	dstate_pub.h
	dstream.cpp
	dstream.h
	libdebug.h
	libdebug_mini.h
)

target_link_libraries(libdebug
    PRIVATE
		hz
		compiler_warnings
)


add_subdirectory(examples)

