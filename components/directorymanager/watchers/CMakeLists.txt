if(UNIX)
    file(GLOB SOURCES *.cpp linux/*.cpp)
elseif(WIN32)
    file(GLOB SOURCES *.cpp windows/*.cpp)
endif(UNIX)
list (REMOVE_ITEM SOURCES moc_*.cpp)
list (REMOVE_ITEM SOURCES *_automoc.cpp)
add_library(directorywatchers STATIC ${SOURCES})
target_link_libraries(directorywatchers Qt5::Core)
