add_executable ( test01 main.c)
target_link_libraries ( test01 cyan cyan_hwcam )

add_test(NAME test_01 COMMAND ./test01 )

