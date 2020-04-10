file(GLOB PY_SCRIPTS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*.py")

foreach(script ${PY_SCRIPTS})
    install(FILES ${script} DESTINATION test)
endforeach(script)