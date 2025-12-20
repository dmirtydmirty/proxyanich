#include <gtest/gtest.h>
#include <string>
#include <indexed_list.h>


TEST(Test, TestConnect) {
    indexed_list<int> list;

    list.init(3);

    list.allocate(1);
    list.allocate(2);
    list.allocate(3);
    
    
}



