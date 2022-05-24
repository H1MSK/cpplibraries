/**
 * @file singleton.h
 * @author H1MSK (ksda47832338@outlook.com)
 * @brief A handy singleton library
 * @version 0.4
 * @date 2021-01-24
 *
 * @copyright Copyright (c) 2020
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef SINGLETON_H
#define SINGLETON_H

#include <cassert>
#include <mutex>
#include <type_traits>
#include <vector>

#include <stdio.h>

/**
 * @brief Wrapper class for the instance to support contruct-on-need
 *
 * @tparam Type The class type
 */
template <typename Type>
struct PointerWrapper {
    bool initialized;
    Type* raw_pointer;

    Type* operator->() {
        // If this line caused an assert failure,
        // that's most because you have a recursion reference in instantiation
        // An example is at the end of this file
        assert(initialized == true);
        return raw_pointer;
    }
    const Type* operator->() const {
        // If this line caused an assert failure,
        // that's most because you have a recursion reference in instantiation
        // An example is at the end of this file
        assert(initialized == true);
        return raw_pointer;
    }

    operator Type*() {
        // If this line caused an assert failure,
        // that's most because you have a recursion reference in instantiation
        // An example is at the end of this file
        assert(initialized == true);
        return raw_pointer;
    }
};

/**
 * @brief Class that actually stores the instance and the access mutex
 *
 * @tparam Type The type of the class
 */
template <typename Type>
struct InstanceSafetyHelper {
    PointerWrapper<Type> wrapper;
    std::mutex mutex;

    static InstanceSafetyHelper* Helper() {
        static InstanceSafetyHelper<Type> helper;
        return &helper;
    }
    InstanceSafetyHelper() : wrapper{false, nullptr}, mutex() {}
    ~InstanceSafetyHelper() {
        if (wrapper.raw_pointer == nullptr)
            return;
        fprintf(stderr, "[SINGLETON] Destructed pointer at 0x%08p\n", wrapper.raw_pointer);
        wrapper.raw_pointer->~Type();
        free(wrapper.raw_pointer);
        wrapper.raw_pointer = nullptr;
    }
};

/**
 * @brief Base singleton class declaring post construction interface
 *
 */
class SingletonBase {
 public:
    /**
     * @brief Post construction interface
     *
     * This function will be called from all the constructed classes immediately
     * after top instance call returns
     */
    virtual void postConstruction() {}
};

/**
 * @brief Helper class for post construction calls
 */
class SingletonPostConstructionHelper {
 public:
    static void push(SingletonBase* pointer) {
        s_construct_stack++;
        s_constructing_classes.push_back(pointer);
    }
    static void pop() {
        assert(s_construct_stack > 0);
        if (!--s_construct_stack) {
            for (auto ptr : s_constructing_classes)
                ptr->postConstruction();
            s_constructing_classes.clear();
        }
    }

 private:
    static std::vector<SingletonBase*> s_constructing_classes;
    static int s_construct_stack;
};

/**
 * @brief Singleton class implementing Instance and getInstance static functions
 *
 * @tparam Type Type of the class
 *
 * To apply singleton pattern on a class A, just let it inherit Singleton<A>
 */
template <typename Type>
class Singleton : public SingletonBase {
 public:
    template <typename... ConstructorArguments>
    static PointerWrapper<Type> Instance(ConstructorArguments... args) {
        InstanceSafetyHelper<Type>* helper =
            InstanceSafetyHelper<Type>::Helper();
        extern SingletonPostConstructionHelper
            s_singleton_post_construction_helper;
        if (helper->wrapper.raw_pointer == nullptr) {
            helper->mutex.lock();
            if (helper->wrapper.raw_pointer == nullptr) {
                Type* data = reinterpret_cast<Type*>(malloc(sizeof(Type)));
                helper->wrapper.raw_pointer = data;
                s_singleton_post_construction_helper.push(data);
                helper->wrapper.raw_pointer = new (data) Type(args...);
                fprintf(stderr, "[SINGLETON] Constructed pointer at 0x%08p\n", helper->wrapper.raw_pointer);
                helper->wrapper.initialized = true;
                s_singleton_post_construction_helper.pop();
            }
            helper->mutex.unlock();
        }
        return helper->wrapper;
    }

    template <typename Other>
    static std::enable_if_t<std::is_base_of_v<Singleton, Other>, void>
    setDestructBefore() {
        // TODO: destruction helper InstanceSafetyHelper
        // UNIMPLEMENTED!!
        assert(false);
    }

    static Type* getInstance() {
        InstanceSafetyHelper<Type>* helper =
            InstanceSafetyHelper<Type>::Helper();
        // If this line caused an assert failure,
        // you may need to instantiate the used class before getting its
        // instance
        assert(helper->wrapper.raw_pointer != nullptr);
        return helper->wrapper.raw_pointer;
    }

    ~Singleton() {
        InstanceSafetyHelper<Type>* helper =
            InstanceSafetyHelper<Type>::Helper();
        helper->wrapper.raw_pointer = nullptr;
        helper->wrapper.initialized = false;
    }
};

#define SINGLETON(class_name)           \
    friend class Singleton<class_name>; \
    friend struct InstanceSafetyHelper<class_name>;

/**
 * An example of a recursion reference in instantiation:
 *
 * ```cpp
 * class B;
 *
 * class A : public Singleton<A> {
 *     B* b;
 *     A() : b(B::Instance()) {}
 *     void func() {};
 * };
 *
 * class B : public Singleton<B> {
 *     B() { A::getInstance()->func(); }
 * };
 *
 * In this example, the instance if A is used during its instantiation.
 *
 * To solve this, we can use postConstruction function in B. The adapted version
 * of B is as follows:
 *
 * ```cpp
 * class B : public Singleton<B> {
 *     B() {}
 *     void postConstruction() override {
 *         A::getInstance()->func();
 *     }
 * };
 * ```
 **/

#endif  // SINGLETON_H
