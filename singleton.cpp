#include "singleton.h"

std::vector<SingletonBase *>
    SingletonPostConstructionHelper::s_classes_under_construction;
int SingletonPostConstructionHelper::s_construct_stack_size;
