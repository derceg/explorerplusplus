#ifndef MACROS_INCLUDED
#define MACROS_INCLUDED

#define EMPTY_STRING _T("")

template <typename T,size_t N>
char (&ArraySizeHelper(T (&array)[N]))[N];
#define SIZEOF_ARRAY(array)	(sizeof(ArraySizeHelper(array)))

#endif