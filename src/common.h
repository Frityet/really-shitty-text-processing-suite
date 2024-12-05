#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <iso646.h>

#pragma clang assume_nonnull begin

#define nullable _Nullable
#define nonnull _Nonnull
//Clang does not define `nullptr` as being nullable :(
#define nullptr ((void *nullable)nullptr)
#define $assert_nonnull(...) (typeof(*(__VA_ARGS__)) *nonnull)({\
    auto _x = __VA_ARGS__;\
    if (!_x) {\
        fprintf(stderr, "[%s:%d] `%s` is null!\n", __FILE__, __LINE__, #__VA_ARGS__);\
        exit(1);\
    }\
    _x;\
});

#define $_CONCAT(x, y) x##y
#define $concat(...) $_CONCAT(__VA_ARGS__)

#define defer void (^$concat($_defer_, __LINE__))(void) __attribute__((cleanup($_defer_execute))) = ^()
static inline void $_defer_execute(void (^nonnull *nonnull block)(void))
{ (*block)(); }

#define $malloc(...) $assert_nonnull(malloc(__VA_ARGS__))
#define $realloc(...) $assert_nonnull(realloc(__VA_ARGS__))
#define $calloc(...) $assert_nonnull(calloc(__VA_ARGS__))
#define $strdup(...) $assert_nonnull(strdup(__VA_ARGS__))
#define $fopen(...) $assert_nonnull(fopen(__VA_ARGS__))

#pragma clang assume_nonnull end
