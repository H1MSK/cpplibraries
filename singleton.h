/**
 * @file singleton.h
 * @author H1MSK (ksda47832338@outlook.com)
 * @brief A handy singleton library
 * @version 0.5
 * @date 2022-06-26
 */

#ifndef SINGLETON_H
#define SINGLETON_H

#include <cassert>
#include <cstdio>
#include <mutex>
#include <type_traits>
#include <vector>

/**
 * @brief Wrapper class for the instance to support lazy construct and recursion
 * reference detection
 *
 * @tparam Type The class type
 *
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
 * ```
 *
 * In this example, the instance if A is used during its instantiation.
 *
 * To solve this problem, there are 3 solutions:
 * 1. (Most expensive) Refactor relevant logic to avoid use of the instance
 * now
 * 2. Reimplements @ref SingletonBase::postConstruction, and move the relevant
 * logic that calls this function.
 * 3. (Most risky)call @ref Singleton::getInstanceDuringBuilding to get the
 * pointer
 *
 * Choosing solution 2, an adapted version of B is as follows:
 *
 * ```cpp
 * class B : public Singleton<B> {
 *     B() {}
 *     void postConstruction() override {
 *         A::getInstance()->func();
 *     }
 * };
 * ```
 */
template <typename Type>
struct PointerWrapper {
  bool initialized;
  Type *raw_pointer;

  Type *operator->() {
    // If this line caused an assert failure,
    // that's mostly because you have a recursion reference in instantiation
    // An example is above
    assert(initialized == true);
    return raw_pointer;
  }
  const Type *operator->() const {
    // If this line caused an assert failure,
    // that's mostly because you have a recursion reference in instantiation
    // An example is above
    assert(initialized == true);
    return raw_pointer;
  }

  operator Type *() {
    // If this line caused an assert failure,
    // that's mostly because you have a recursion reference in instantiation
    // An example is above
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

  static InstanceSafetyHelper *Helper() {
    static InstanceSafetyHelper<Type> helper;
    return &helper;
  }
  InstanceSafetyHelper() : wrapper{false, nullptr}, mutex() {}

  ~InstanceSafetyHelper() {
    // If this line caused an assert failure,
    // please manually call Type::destructInstance() on exit
    assert(wrapper.initialized == false && wrapper.raw_pointer == nullptr);
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
 *
 * @todo Maybe this can be achieved by template metaprogramming?
 */
class SingletonPostConstructionHelper {
 public:
  static void push(SingletonBase *pointer) {
    s_construct_stack_size++;
    s_classes_under_construction.push_back(pointer);
  }
  static void pop() {
    assert(s_construct_stack_size > 0);
    if (!--s_construct_stack_size) {
      for (auto ptr : s_classes_under_construction) ptr->postConstruction();
      s_classes_under_construction.clear();
    }
  }

 private:
  static std::vector<SingletonBase *> s_classes_under_construction;
  static int s_construct_stack_size;
};

/**
 * @brief Singleton class implementing Instance and getInstance static functions
 *
 * @tparam Type Type of the class
 *
 * To apply singleton pattern on a class A, just let it inherit Singleton<A>.
 *
 * Here's a short example:
 * ```cpp
 * #include "singleton.h"
 *
 * class A : public Singleton<A> {
 *   //...
 * };
 *
 * int main() {
 *   A* a = A::createInstance();
 *   //a->...
 *   A::destructInstance();
 *   return 0;
 * }
 * ```
 */
template <typename Type>
class Singleton : public SingletonBase {
 public:
  /**
   * @brief Get the instance of Type, or construct it on need
   *
   * @tparam ConstructorArguments Argument types for construction
   * @param args Arguments for construction
   * @return PointerWrapper<Type> Pointer wrapper class of type, can be used as
   * raw pointer
   *
   * @deprecated User should separate creation logic from instance obtaining
   */
  template <typename... ConstructorArguments>
  [[deprecated]] static PointerWrapper<Type> Instance(
      ConstructorArguments... args) {
    InstanceSafetyHelper<Type> *helper = InstanceSafetyHelper<Type>::Helper();
    extern SingletonPostConstructionHelper s_singleton_post_construction_helper;
    if (helper->wrapper.raw_pointer == nullptr) {
      helper->mutex.lock();
      if (helper->wrapper.raw_pointer == nullptr) {
        Type *data = reinterpret_cast<Type *>(malloc(sizeof(Type)));
        helper->wrapper.raw_pointer = data;
        s_singleton_post_construction_helper.push(data);
        helper->wrapper.raw_pointer = new (data) Type(args...);
        fprintf(stderr, "[SINGLETON] Constructed pointer at 0x%08p\n",
                helper->wrapper.raw_pointer);
        helper->wrapper.initialized = true;
        s_singleton_post_construction_helper.pop();
      }
      helper->mutex.unlock();
    }
    return helper->wrapper;
  }

  /**
   * @brief Create the instance of Type, returning its pointer
   *
   * @tparam ConstructorArguments Argument types for construction
   * @param args Arguments for construction
   * @return PointerWrapper<Type> Pointer wrapper class of type, can be used as
   * raw pointer
   */
  template <typename... ConstructorArguments>
  static PointerWrapper<Type> createInstance(ConstructorArguments... args) {
    InstanceSafetyHelper<Type> *helper = InstanceSafetyHelper<Type>::Helper();
    extern SingletonPostConstructionHelper s_singleton_post_construction_helper;
    assert(helper->wrapper.raw_pointer == nullptr);
    helper->mutex.lock();
    assert(helper->wrapper.raw_pointer == nullptr);
    Type *data = reinterpret_cast<Type *>(malloc(sizeof(Type)));
    helper->wrapper.raw_pointer = data;
    s_singleton_post_construction_helper.push(data);
    helper->wrapper.raw_pointer = new (data) Type(args...);
    fprintf(stderr, "[SINGLETON] Constructed pointer at 0x%08p\n",
            helper->wrapper.raw_pointer);
    helper->wrapper.initialized = true;
    s_singleton_post_construction_helper.pop();
    helper->mutex.unlock();
    return helper->wrapper;
  }

  //  template <typename Other>
  //  static std::enable_if_t<std::is_base_of_v<Singleton, Other>, void>
  //  setDestructBefore() {
  //    // TODO: destruction helper InstanceSafetyHelper
  //    // UNIMPLEMENTED!!
  //    assert(false);
  //  }

  /**
   * @brief Destruct the instance of Type
   */
  static void destructInstance() {
    InstanceSafetyHelper<Type> *helper = InstanceSafetyHelper<Type>::Helper();
    assert(helper->wrapper.raw_pointer != nullptr &&
           helper->wrapper.initialized);

    // This will call Singleton<Type>::~Singleton, in which the wrapper will be
    // reset
    //
    delete helper->wrapper.raw_pointer;
  }

  /**
   * @brief Get the instance of Type, asserting it's constructed
   *
   * @return Type* Pointer to the instance of Type
   *
   * @note
   * When this function asserts failed, there are two situations:
   * 1. You are trying to get the instance before calling @ref
   * Singleton::createInstance,
   * 2. You are trying to use the instance of Type during its construction
   *
   * @note
   * For the first scenario, please construct it before using;
   * For the second one, please refer to @ref PointerWrapper for solution.
   */
  static Type *getInstance() {
    InstanceSafetyHelper<Type> *helper = InstanceSafetyHelper<Type>::Helper();
    assert(helper->wrapper.raw_pointer != nullptr &&
           helper->wrapper.initialized);
    return helper->wrapper.raw_pointer;
  }

  /**
   * @brief Get the instance of Type during its construction
   *
   * @return Type* Pointer to the instance of Type
   *
   * @deprecated Use of instance during its construction is always risky, and
   * will very likely to cause instability / crash of the program, so use at
   * your own risk
   */
  [[deprecated]] static Type *getInstanceDuringBuilding() {
    InstanceSafetyHelper<Type> *helper = InstanceSafetyHelper<Type>::Helper();
    assert(helper->wrapper.raw_pointer != nullptr &&
           !helper->wrapper.initialized);
    return helper->wrapper.raw_pointer;
  }

  /**
   * @brief Destroy the instance of Type, and update its InstanceSafetyHelper
   * to avoid multiple deletion / memory leak
   */
  ~Singleton() {
    InstanceSafetyHelper<Type> *helper = InstanceSafetyHelper<Type>::Helper();
    assert(helper->wrapper.initialized);
    helper->mutex.lock();
    // If this line caused an assertion fail, you are probably trying to
    // construct a new instance while destroying the old one
    // It's mostly because you are using Singleton::Instace if it's not intended
    assert(helper->wrapper.initialized);
    helper->wrapper.initialized = false;
    helper->wrapper.raw_pointer = nullptr;
    helper->mutex.unlock();
  }
};

#endif  // SINGLETON_H
