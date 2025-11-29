#pragma once
#include <memory>
#include <stdlib.h>

template<typename T>
struct node 
{
    size_t idx:24;
    size_t used:8;
    T* next = nullptr;
    T* prev = nullptr;
    T value;
    node(T* n, T* p):
        next(n),
        prev(p)
    {}
};


template<typename T>
struct indexed_list
{
    using node_t = node<T>; 
    
    int init(size_t size) {

        int rc = posix_memalign((void**)&mem_pool_, 64, size * sizeof(node_t));
        if (rc == 0) {
            for (size_t i = 0; i < size; ++i){
                node_t* node = &mem_pool_[i];
                insert(end(&free_root, node));
                node->idx = i;
                node->used = 0;
            }
        }
        return rc;
    }
    int64_t allocate(const T& val){
        int rc = -ENOMEM;
        if (begin(&free_root) != end(&free_root)) {
            node_t* node = begin(&free_root);
            node->value = val;
            node->used = 1;
            insert(end(&used_root), node);
            rc = node->idx;
        }
        return rc;
    }

    int deallocate(size_t idx) {
        int rc = -ENOENT;
        node_t* node = &mem_pool_[idx];
        if (node->used == 1) {
            erase(node);
            node->used = 0;
            rc = 0;
        }
        return rc;
    }

    T* operator[](size_t idx) {
        T* rv = nullptr;
        if (idx < size_ and mem_pool_[idx].used) {
            rv = &mem_pool_[idx].value;
        }
        return rv;
    }

    node_t* begin(node_t* root) {
        return root->next;
    }

    node_t* end(node_t* root) {
        return root;
    }

    node_t* insert(node_t* it, node_t* node) {
        it->prev->next = node;
        node->prev = it->prev;

        node->next = it;
        it->prev = node;
        return node;
    }

    void erase(node_t* it) {

        it->prev->next = it->next;
        it->next->prev = it->prev;
        it->next = it->prev = nullptr;
    }

private:
    node_t free_root = node_t(&free_root, &free_root);
    node_t used_root = node_t(&used_root, &used_root);;

    node_t* mem_pool_ = nullptr;
    size_t size_ = 0;

};
