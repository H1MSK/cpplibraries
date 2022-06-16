#ifndef COLLECTABLE_H
#define COLLECTABLE_H

#include <cassert>
#include <type_traits>
#include <unordered_set>

#include "singleton.h"

template <class Type>
class AutoCollectable;

template <class Type, class CollectableType = AutoCollectable<Type>,
          class ItemContainer = std::unordered_set<Type *>>
class Collector
    : public Singleton<Collector<Type, CollectableType, ItemContainer>> {
 private:
  ItemContainer m_container;
  friend class AutoCollectable<Type>;
  bool insert(Type *c) {
    bool success = m_container.insert(c).second;
    assert(success);
    return true;
  }
  //   bool replace(CollectableType *old, CollectableType *c) {
  //     auto ret = m_container.insert(c);
  //     assert(ret.second);
  //     auto old_itr = m_container.find(old);
  //     assert(old_itr != m_container.end());
  //     m_container.erase(old_itr);
  //     return true;
  //   }
  bool erase(Type *c) {
    auto itr = m_container.find(c);
    assert(itr != m_container.end());
    m_container.erase(itr);
    return true;
  }

 public:
  const ItemContainer &container() const { return m_container; }
  typename ItemContainer::iterator begin() { return m_container.begin(); }
  typename ItemContainer::const_iterator cbegin() {
    return m_container.cbegin();
  }
  typename ItemContainer::iterator end() { return m_container.end(); }
  typename ItemContainer::const_iterator cend() { return m_container.cend(); }
};

template <class Type>
class AutoCollectable {
 protected:
  AutoCollectable() {
    m_registered_in_collector =
        Collector<Type>::Instance()->insert(static_cast<Type *>(this));
    assert(m_registered_in_collector);
  }
  //   Collectable(const Collectable<Type> &c) :
  //   m_registered_in_collector(false) {
  //     assert(c.m_registered_in_collector);
  //     m_registered_in_collector = Collector<Type>::Instance()->insert(this);
  //     assert(m_registered_in_collector);
  //   }
  //   Collectable(Collectable<Type> &&other) : m_registered_in_collector(false)
  //   {
  //     assert(other.m_registered_in_collector);
  //     m_registered_in_collector =
  //         Collector<Type>::Instance()->replace(&other, this);
  //     assert(m_registered_in_collector);
  //     other.m_registered_in_collector = false;
  //   }

 public:
  ~AutoCollectable() {
    if (m_registered_in_collector) {
      m_registered_in_collector =
          Collector<Type>::Instance()->erase(static_cast<Type *>(this));
    }
  }

 private:
  bool m_registered_in_collector{false};
};

#endif  // COLLECTABLE_H
