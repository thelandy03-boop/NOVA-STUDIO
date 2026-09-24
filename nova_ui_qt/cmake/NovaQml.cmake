function(nova_collect_qml out_var)
    file(GLOB_RECURSE _abs_qml_files
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/qml/*.qml"
    )
    list(FILTER _abs_qml_files EXCLUDE REGEX ".*/build/.*")
    list(FILTER _abs_qml_files EXCLUDE REGEX ".*\\.bak$")

    # Convertimos cada ruta absoluta a ruta relativa para Qt 6
    set(_rel_qml_files "")
    foreach(_abs_file IN LISTS _abs_qml_files)
        file(RELATIVE_PATH _rel_file "${CMAKE_CURRENT_SOURCE_DIR}" "${_abs_file}")
        list(APPEND _rel_qml_files "${_rel_file}")
    endforeach()

    list(SORT _rel_qml_files)
    set(${out_var} "${_rel_qml_files}" PARENT_SCOPE)
endfunction()

function(nova_mark_singletons)
    set_source_files_properties(
        qml/theme/Theme.qml
        PROPERTIES QT_QML_SINGLETON_TYPE TRUE
    )
endfunction()