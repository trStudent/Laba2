SUBDIRLIST(SUBDIRS ${CMAKE_CURRENT_SOURCE_DIR}/src)

FOREACH(subdir ${SUBDIRS})
  SET(AppName ${subdir})

  FILE(GLOB SRC_FILES src/${subdir}/*.cpp)
  
  IF(SRC_FILES)
    add_executable(${AppName})  

    TARGET_SOURCES(${AppName} PRIVATE ${SRC_FILES})
  ENDIf()
ENDFOREACH(subdir ${SUBDIRS})

TARGET_LINK_LIBRARIES(Main PRIVATE 
   core::General
)