#include "singleton.h"

std::vector<SingletonBase*> SingletonPostConstructionHelper::s_constructing_classes;
int SingletonPostConstructionHelper::s_construct_stack;
