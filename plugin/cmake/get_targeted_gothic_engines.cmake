function(get_targeted_gothic_engines out_var)
    set(engines "")

    if(GOTHIC_API_G1)
        list(APPEND engines
            G1 Gothic_I_Classic
        )
    endif()

    if(GOTHIC_API_G1A)
        list(APPEND engines
            G1A Gothic_I_Addon
        )
    endif()

    if(GOTHIC_API_G2)
        list(APPEND engines
            G2 Gothic_II_Classic
        )
    endif()

    if(GOTHIC_API_G2A)
        list(APPEND engines
            G2A Gothic_II_Addon
        )
    endif()

    set(${out_var} "${engines}" PARENT_SCOPE)
endfunction()