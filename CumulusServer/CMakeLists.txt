SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,./../CumulusLib/,-rpath,/usr/local/lib/")

INCLUDE_DIRECTORIES( 
	"../CumulusLib/include/"
)

ADD_SUBDIRECTORY(sources)

# vim:ft=cmake
