//
// Created by caida-si on 16/07/2026.
//

#ifndef ITER_HPP
#define ITER_HPP

#include <iostream>
#include <string>

template <typename T>
void iter(T *array, const int length, void (*func)(T &)) {
    for (int i = 0; i < length; i++)
        func(array[i]);
}

template <typename T>
void iter(T *array, const int length, void (*func)(const T &)) {
    for (int i = 0; i < length; i++)
        func(array[i]);
}

#endif