// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_MAP_H_
#define MOJO_PUBLIC_CPP_BINDINGS_MAP_H_

#include <map>

#include "mojo/public/cpp/bindings/lib/map_internal.h"
#include "mojo/public/cpp/bindings/lib/template_util.h"

namespace mojo {

// A move-only map that can handle move-only values. Map has the following
// characteristics:
//   - The map itself can be null, and this is distinct from empty.
//   - Keys must not be move-only.
//   - The Key-type's "<" operator is used to sort the entries, and also is
//     used to determine equality of the key values.
//   - There can only be one entry per unique key.
//   - Values of move-only types will be moved into the Map when they are added
//     using the insert() method.
template <typename Key, typename Value>
class Map {
  MOJO_MOVE_ONLY_TYPE(Map)

 public:
  // Map keys cannot be move only classes.
  static_assert(!internal::IsMoveOnlyType<Key>::value,
                "Map keys cannot be move only types.");

  typedef internal::MapTraits<Key,
                              Value,
                              internal::IsMoveOnlyType<Value>::value> Traits;
  typedef typename Traits::KeyStorageType KeyStorageType;
  typedef typename Traits::KeyRefType KeyRefType;
  typedef typename Traits::KeyConstRefType KeyConstRefType;
  typedef typename Traits::KeyForwardType KeyForwardType;

  typedef typename Traits::ValueStorageType ValueStorageType;
  typedef typename Traits::ValueRefType ValueRefType;
  typedef typename Traits::ValueConstRefType ValueConstRefType;
  typedef typename Traits::ValueForwardType ValueForwardType;

  typedef internal::Map_Data<typename internal::WrapperTraits<Key>::DataType,
                             typename internal::WrapperTraits<Value>::DataType>
      Data_;

  Map() : is_null_(true) {}

  // Constructs a non-null Map containing the specified |keys| mapped to the
  // corresponding |values|.
  Map(mojo::Array<Key> keys, mojo::Array<Value> values) : is_null_(false) {
    MOJO_DCHECK(keys.size() == values.size());
    Traits::InitializeFrom(&map_, keys.Pass(), values.Pass());
  }

  ~Map() { Traits::Finalize(&map_); }

  Map(Map&& other) : is_null_(true) { Take(&other); }
  Map& operator=(Map&& other) {
    Take(&other);
    return *this;
  }

  // Copies the contents of some other type of map into a new Map using a
  // TypeConverter. A TypeConverter for std::map to Map is defined below.
  template <typename U>
  static Map From(const U& other) {
    return TypeConverter<Map, U>::Convert(other);
  }

  // Copies the contents of the Map into some other type of map. A TypeConverter
  // for Map to std::map is defined below.
  template <typename U>
  U To() const {
    return TypeConverter<U, Map>::Convert(*this);
  }

  // Destroys the contents of the Map and leaves it in the null state.
  void reset() {
    if (!map_.empty()) {
      Traits::Finalize(&map_);
      map_.clear();
    }
    is_null_ = true;
  }

  bool is_null() const { return is_null_; }

  // Indicates the number of keys in the map.
  size_t size() const { return map_.size(); }

  void mark_non_null() { is_null_ = false; }

  // Inserts a key-value pair into the map, moving the value by calling its
  // Pass() method if it is a move-only type. Like std::map, this does not
  // insert |value| if |key| is already a member of the map.
  void insert(KeyForwardType key, ValueForwardType value) {
    is_null_ = false;
    Traits::Insert(&map_, key, value);
  }

  // Returns a reference to the value associated with the specified key,
  // crashing the process if the key is not present in the map.
  ValueRefType at(KeyForwardType key) { return Traits::at(&map_, key); }
  ValueConstRefType at(KeyForwardType key) const {
    return Traits::at(&map_, key);
  }

  // Returns a reference to the value associated with the specified key,
  // creating a new entry if the key is not already present in the map. A
  // newly-created value will be value-initialized (meaning that it will be
  // initialized by the default constructor of the value type, if any, or else
  // will be zero-initialized).
  ValueRefType operator[](KeyForwardType key) {
    is_null_ = false;
    return Traits::GetOrInsert(&map_, key);
  }

  // Swaps the contents of this Map with another Map of the same type (including
  // nullness).
  void Swap(Map<Key, Value>* other) {
    std::swap(is_null_, other->is_null_);
    map_.swap(other->map_);
  }

  // Swaps the contents of this Map with an std::map containing keys and values
  // of the same type. Since std::map cannot represent the null state, the
  // std::map will be empty if Map is null. The Map will always be left in a
  // non-null state.
  void Swap(std::map<Key, Value>* other) {
    is_null_ = false;
    map_.swap(*other);
  }

  // Returns a new Map that contains a copy of the contents of this map.  If the
  // values are of a type that is designated move-only, they will be cloned
  // using the Clone() method of the type. Please note that calling this method
  // will fail compilation if the value type cannot be cloned (which usually
  // means that it is a Mojo handle type or a type that contains Mojo handles).
  Map Clone() const {
    Map result;
    result.is_null_ = is_null_;
    Traits::Clone(map_, &result.map_);
    return result.Pass();
  }

  // Indicates whether the contents of this map are equal to those of another
  // Map (including nullness). Keys are compared by the != operator. Values are
  // compared as follows:
  //   - Map, Array, Struct, or StructPtr values are compared by their Equals()
  //     method.
  //   - ScopedHandleBase-derived types are compared by their handles.
  //   - Values of other types are compared by their "==" operator.
  bool Equals(const Map& other) const {
    if (is_null() != other.is_null())
      return false;
    if (size() != other.size())
      return false;
    auto i = cbegin();
    auto j = other.cbegin();
    while (i != cend()) {
      if (i.GetKey() != j.GetKey())
        return false;
      if (!internal::ValueTraits<Value>::Equals(i.GetValue(), j.GetValue()))
        return false;
      ++i;
      ++j;
    }
    return true;
  }

 private:
  // A Map Iterator, templated for mutable and const iterator behaviour.
  // If |IsConstIterator| is true, the iterator behaves like a const-iterator.
  //
  // TODO(vardhan):  Make this adhere to the BidirectionalIterator concept.
  enum class IteratorMutability { kConst, kMutable };
  template <IteratorMutability MutabilityType = IteratorMutability::kMutable>
  class InternalIterator {
    using InternalIteratorType = typename internal::Conditional<
        MutabilityType == IteratorMutability::kConst,
        typename std::map<KeyStorageType, ValueStorageType>::const_iterator,
        typename std::map<KeyStorageType, ValueStorageType>::iterator>::type;

    using ReturnValueType =
        typename internal::Conditional<MutabilityType ==
                                           IteratorMutability::kConst,
                                       ValueConstRefType,
                                       ValueRefType>::type;

   public:
    InternalIterator() : it_() {}
    InternalIterator(InternalIteratorType it) : it_(it) {}

    // The key is always a const reference, but the value is conditional on
    // whether this is a const iterator or not.
    KeyConstRefType GetKey() { return Traits::GetKey(it_); }
    ReturnValueType GetValue() { return Traits::GetValue(it_); }

    InternalIterator& operator++() {
      ++it_;
      return *this;
    }
    InternalIterator<MutabilityType> operator++(int) {
      InternalIterator<MutabilityType> original(*this);
      ++it_;
      return original;
    }
    InternalIterator& operator--() {
      --it_;
      return *this;
    }
    InternalIterator<MutabilityType> operator--(int) {
      InternalIterator<MutabilityType> original(*this);
      --it_;
      return original;
    }
    bool operator!=(const InternalIterator& rhs) const {
      return it_ != rhs.it_;
    }
    bool operator==(const InternalIterator& rhs) const {
      return it_ == rhs.it_;
    }

   private:
    InternalIteratorType it_;
  };

 public:
  using MapIterator = InternalIterator<IteratorMutability::kMutable>;
  using ConstMapIterator = InternalIterator<IteratorMutability::kConst>;

  // Provide read-only and mutable iteration over map members in a way similar
  // to STL collections.
  ConstMapIterator cbegin() const { return ConstMapIterator(map_.cbegin()); }
  ConstMapIterator cend() const { return ConstMapIterator(map_.cend()); }
  MapIterator begin() { return MapIterator(map_.begin()); }
  MapIterator end() { return MapIterator(map_.end()); }

  // Returns the iterator pointing to the entry for |key|, if present, or else
  // returns |cend()| or |end()|, respectively.
  ConstMapIterator find(KeyForwardType key) const {
    return ConstMapIterator(map_.find(key));
  }
  MapIterator find(KeyForwardType key) { return MapIterator(map_.find(key)); }

 private:
  typedef std::map<KeyStorageType, ValueStorageType> Map::*Testable;

 public:
  // The Map may be used in boolean expressions to determine if it is non-null,
  // but is not implicitly convertible to an actual bool value (which would be
  // dangerous).
  operator Testable() const { return is_null_ ? 0 : &Map::map_; }

 private:
  void Take(Map* other) {
    reset();
    Swap(other);
  }

  std::map<KeyStorageType, ValueStorageType> map_;
  bool is_null_;
};

// Copies the contents of an std::map to a new Map, optionally changing the
// types of the keys and values along the way using TypeConverter.
template <typename MojoKey,
          typename MojoValue,
          typename STLKey,
          typename STLValue>
struct TypeConverter<Map<MojoKey, MojoValue>, std::map<STLKey, STLValue>> {
  static Map<MojoKey, MojoValue> Convert(
      const std::map<STLKey, STLValue>& input) {
    Map<MojoKey, MojoValue> result;
    result.mark_non_null();
    for (auto& pair : input) {
      result.insert(TypeConverter<MojoKey, STLKey>::Convert(pair.first),
                    TypeConverter<MojoValue, STLValue>::Convert(pair.second));
    }
    return result.Pass();
  }
};

// Copies the contents of a Map to an std::map, optionally changing the types of
// the keys and values along the way using TypeConverter.
template <typename MojoKey,
          typename MojoValue,
          typename STLKey,
          typename STLValue>
struct TypeConverter<std::map<STLKey, STLValue>, Map<MojoKey, MojoValue>> {
  static std::map<STLKey, STLValue> Convert(
      const Map<MojoKey, MojoValue>& input) {
    std::map<STLKey, STLValue> result;
    if (!input.is_null()) {
      for (auto it = input.cbegin(); it != input.cend(); ++it) {
        result.insert(std::make_pair(
            TypeConverter<STLKey, MojoKey>::Convert(it.GetKey()),
            TypeConverter<STLValue, MojoValue>::Convert(it.GetValue())));
      }
    }
    return result;
  }
};

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_MAP_H_
