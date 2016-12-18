CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(LevelSyn)

if(MSVC)
   set( BOOST_ROOT "C:/Libraries/boost")
#   set( BOOST_LIBRARYDIR "C:/Program Files/boost/boost_1_54_0/lib64-msvc-10.0" )
endif()

SET(Boost_USE_STATIC_LIBS TRUE)
FIND_PACKAGE( Boost COMPONENTS thread REQUIRED )
LINK_DIRECTORIES    ( ${Boost_LIBRARY_DIRS} )
INCLUDE_DIRECTORIES ( ${Boost_INCLUDE_DIRS} )

if(APPLE)
   FIND_LIBRARY( coreFoundation CoreFoundation )
endif()

ADD_SUBDIRECTORY( tinyxml )
INCLUDE_DIRECTORIES ( tinyxml )
INCLUDE_DIRECTORIES ( clipper )

SET(SOURCES
	clipper/clipper.cpp
	code/PlanarGraph.cpp
	code/Room.cpp
	code/RoomTemplates.cpp
	code/RoomLayout.cpp
	code/LevelSynth.cpp
	code/LevelConfig.cpp
	code/LevelMath.cpp
	code/clipperWrapper.cpp
	code/ConfigSpace.cpp
)

SET(HEADERS
	clipper/clipper.hpp
	code/GraphChain.h
	code/GraphEdge.h
	code/GraphFace.h
	code/GraphNode.h
	code/LineBase.h
	code/PlanarGraph.h
	code/Room.h
	code/RoomTemplates.h
	code/RoomLayout.h
	code/RoomEdge.h
	code/LevelSynth.h
	code/LevelConfig.h
	code/LevelMath.h
	code/LevelTimer.h
	code/clipperWrapper.h
	code/ConfigSpace.h
	code/util.h
	code/vec.h
)

ADD_EXECUTABLE( levels code/levels.cpp ${SOURCES} ${HEADERS} )
TARGET_LINK_LIBRARIES( levels tinyxml ${Boost_LIBRARIES} ${Boost_THREAD_LIBRARY} )
if(APPLE)
   TARGET_LINK_LIBRARIES( levels ${coreFoundation} )
endif()
