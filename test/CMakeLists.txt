find_program(BASH_EXECUTABLE NAMES bash REQUIRED)

function(CREATE_TEST NAME_TEST)
    set(NAME_EXE ${NAME_TEST}_test)
    add_executable(${NAME_EXE} ${NAME_TEST}.cpp)
    target_link_libraries(${NAME_EXE} usm GTest::gtest_main)
    add_test(
        NAME ${NAME_EXE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND ${BASH_EXECUTABLE} run.sh $<TARGET_FILE:${NAME_EXE}>
    )
endfunction()

create_test(simple_list)
create_test(string_hash)
create_test(mash_info_struct)
create_test(matrix4x4)
create_test(mavltree)
create_test(memory)
create_test(mstring)
create_test(mvector)
create_test(quaternion)
create_test(vector3d)
create_test(vector4d)
create_test(vm_stack)
create_test(resource_key)
create_test(resource_location)
create_test(resource_pack_directory)
create_test(resource_pack_slot)
create_test(resource_pack_streamer)
create_test(resource_partition)
create_test(slab_allocator)
create_test(tlresource_directory)
create_test(tlresource_location)
create_test(fixedstring)
if(SETUP_STANDALONE_SYSTEM)
    create_test(resource_manager)
endif()


function(CREATE_STD_TEST NAME_TEST)
    set(STD_NAME_TEST std_${NAME_TEST})
    add_executable(${STD_NAME_TEST} ${NAME_TEST}.cpp)
    target_link_libraries(${STD_NAME_TEST} usm GTest::gtest_main)
    add_test(
        NAME ${STD_NAME_TEST}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND ${BASH_EXECUTABLE} run.sh $<TARGET_FILE:${STD_NAME_TEST}>
    )
endfunction()

create_std_test(vector)
create_std_test(list)
create_std_test(map)
create_std_test(hash_map)
create_std_test(deque)
create_std_test(stack)
create_std_test(set)
