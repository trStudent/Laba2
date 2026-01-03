SUBDIRLIST(SUBDIRS ${CMAKE_CURRENT_SOURCE_DIR})

FOREACH(subdir ${SUBDIRS})
  SET(TestName ${subdir}Test)

  FILE(GLOB SRC_FILES ${subdir}/*.cpp)
  
  IF(SRC_FILES)
    add_executable(${TestName})  

    TARGET_SOURCES(${TestName} PRIVATE ${SRC_FILES})
    TARGET_LINK_LIBRARIES(${TestName} PUBLIC gtest_main core::${subdir})

    ADD_TEST(
        NAME ${TestName}
        COMMAND ${TestName}
    )
  ENDIF()
ENDFOREACH(subdir ${SUBDIRS})

