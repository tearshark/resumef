#pragma once

#include "for_macro_args.inl"

#define SEEK_WIDGET_BY_NAME(Layer, Variable) Variable = dynamic_cast<decltype(Variable)>(ui::Helper::seekWidgetByName((Layer), "@" #Variable));
#define SeekAllWidget(Layer, ...) FOR_MACRO_ARGS(Layer, SEEK_WIDGET_BY_NAME, __VA_ARGS__)
