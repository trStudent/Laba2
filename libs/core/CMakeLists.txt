subdirlist(SUBDIRS ${CMAKE_CURRENT_SOURCE_DIR}/src)

foreach(subdir ${SUBDIRS})
  set(LibName ${subdir}Core)

  file(GLOB SRC_FILES src/${subdir}/*.cpp)
  file(GLOB HDR_FILES include/core/${subdir}/*.h)
  
  if(SRC_FILES)
    add_library(${LibName} STATIC)  
    add_library(core::${subdir} ALIAS ${LibName})

    target_sources(${LibName} PRIVATE ${SRC_FILES})
    target_sources(${LibName} PUBLIC FILE_SET HEADERS FILES ${HDR_FILES})

    target_include_directories(${LibName} PUBLIC include)
  endif()
endforeach()