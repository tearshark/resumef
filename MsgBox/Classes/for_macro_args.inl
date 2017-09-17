#pragma once

#include "make_arg_list.inl"
#include "get_arg_count.inl"

#define FOR_MACRO_ARGS(params, op, ...) MARCO_EXPAND_ALL(MAKE_ARG_LIST(GET_ARG_COUNT(__VA_ARGS__), op, params, __VA_ARGS__)) 
