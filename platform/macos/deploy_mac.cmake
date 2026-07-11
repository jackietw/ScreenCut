# Helper CMake script to conditionally run macdeployqt only when Qt frameworks are not yet deployed.
# Avoids re-copying 100MB+ of dylibs and running install_name_tool hundreds of times on every incremental link.

set(NEEDS_DEPLOY FALSE)
if(NOT EXISTS "${APP_DIR}/Contents/Frameworks/QtCore.framework"
   OR NOT EXISTS "${APP_DIR}/Contents/Frameworks/QtWidgets.framework"
   OR NOT EXISTS "${APP_DIR}/Contents/Frameworks/QtGui.framework"
   OR NOT EXISTS "${APP_DIR}/Contents/PlugIns/platforms/libqcocoa.dylib")
    set(NEEDS_DEPLOY TRUE)
endif()

if(NEEDS_DEPLOY)
    message(STATUS "Deploying Qt Frameworks and plugins for ${APP_DIR}...")
    execute_process(
        COMMAND "${MACDEPLOYQT}" "${APP_DIR}" -always-overwrite -no-codesign
        RESULT_VARIABLE res
    )
    if(NOT res EQUAL 0)
        message(FATAL_ERROR "macdeployqt failed with return code ${res}")
    endif()
else()
    message(STATUS "Qt Frameworks and cocoa plugin already deployed in ${APP_DIR} (skipping macdeployqt)")
endif()

# Always ensure any newly relinked main binaries inside Contents/MacOS have @executable_path/../Frameworks
# in rpath and have all absolute Qt framework links updated to @rpath/Qt*.framework so the app never loads dual Qt frameworks.
file(GLOB BINARIES "${APP_DIR}/Contents/MacOS/*")
foreach(BIN ${BINARIES})
    if(NOT IS_DIRECTORY "${BIN}")
        execute_process(
            COMMAND python3 "${CMAKE_CURRENT_LIST_DIR}/patch_rpath.py" "${BIN}"
        )
    endif()
endforeach()
