#include "make_arg_list.inl"
#include "get_arg_count.inl"

#define SEEK_WIDGET_BY_NAME(Layer, Obj) Obj = dynamic_cast<decltype(Obj)>(ui::Helper::seekWidgetByName((Layer), "@" #Obj));
#define SEEK_ALL_WIDGET(Layer, N, ...) MARCO_EXPAND_ALL(MAKE_ARG_LIST(N, SEEK_WIDGET_BY_NAME, Layer, __VA_ARGS__)) 


#define SeekAllWidget(Layer, ...) SEEK_ALL_WIDGET(Layer, GET_ARG_COUNT(__VA_ARGS__), __VA_ARGS__)
