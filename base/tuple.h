// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// A Tuple is a generic templatized container, similar in concept to std::pair
// and std::tuple.  The convenient MakeTuple() function takes any number of
// arguments and will construct and return the appropriate Tuple object.  The
// functions DispatchToMethod and DispatchToFunction take a function pointer or
// instance and method pointer, and unpack a tuple into arguments to the call.
//
// Tuple elements are copied by value, and stored in the tuple.  See the unit
// tests for more details of how/when the values are copied.
//
// Example usage:
//   // These two methods of creating a Tuple are identical.
//   Tuple<int, const char*> tuple_a(1, "wee");
//   Tuple<int, const char*> tuple_b = MakeTuple(1, "wee");
//
//   void SomeFunc(int a, const char* b) { }
//   DispatchToFunction(&SomeFunc, tuple_a);  // SomeFunc(1, "wee")
//   DispatchToFunction(
//       &SomeFunc, MakeTuple(10, "foo"));    // SomeFunc(10, "foo")
//
//   struct { void SomeMeth(int a, int b, int c) { } } foo;
//   DispatchToMethod(&foo, &Foo::SomeMeth, MakeTuple(1, 2, 3));
//   // foo->SomeMeth(1, 2, 3);

#ifndef BASE_TUPLE_H__
#define BASE_TUPLE_H__

#include "base/bind_helpers.h"

// Index sequences
//
// Minimal clone of the similarly-named C++14 functionality.

template <size_t...>
struct IndexSequence {};

template <size_t... Ns>
struct MakeIndexSequenceImpl;

template <size_t... Ns>
struct MakeIndexSequenceImpl<0, Ns...> {
  using Type = IndexSequence<Ns...>;
};

template <size_t N, size_t... Ns>
struct MakeIndexSequenceImpl<N, Ns...>
    : MakeIndexSequenceImpl<N - 1, N - 1, Ns...> {};

template <size_t N>
using MakeIndexSequence = typename MakeIndexSequenceImpl<N>::Type;

// Traits ----------------------------------------------------------------------
//
// A simple traits class for tuple arguments.
//
// ValueType: the bare, nonref version of a type (same as the type for nonrefs).
// RefType: the ref version of a type (same as the type for refs).
// ParamType: what type to pass to functions (refs should not be constified).

template <class P>
struct TupleTraits {
  typedef P ValueType;
  typedef P& RefType;
  typedef const P& ParamType;
};

template <class P>
struct TupleTraits<P&> {
  typedef P ValueType;
  typedef P& RefType;
  typedef P& ParamType;
};

// Tuple -----------------------------------------------------------------------
//
// This set of classes is useful for bundling 0 or more heterogeneous data types
// into a single variable.  The advantage of this is that it greatly simplifies
// function objects that need to take an arbitrary number of parameters; see
// RunnableMethod and IPC::MessageWithTuple.
//
// Tuple<> is supplied to act as a 'void' type.  It can be used, for example,
// when dispatching to a function that accepts no arguments (see the
// Dispatchers below).
// Tuple<A> is rarely useful.  One such use is when A is non-const ref that you
// want filled by the dispatchee, and the tuple is merely a container for that
// output (a "tier").  See MakeRefTuple and its usages.

template <typename IxSeq, typename... Ts>
struct TupleBaseImpl;
template <typename... Ts>
using TupleBase = TupleBaseImpl<MakeIndexSequence<sizeof...(Ts)>, Ts...>;
template <size_t N, typename T>
struct TupleLeaf;

template <typename... Ts>
struct Tuple : TupleBase<Ts...> {
  Tuple() : TupleBase<Ts...>() {}
  explicit Tuple(typename TupleTraits<Ts>::ParamType... args)
      : TupleBase<Ts...>(args...) {}
};

// Avoids ambiguity between Tuple's two constructors.
template <>
struct Tuple<> {};

template <size_t... Ns, typename... Ts>
struct TupleBaseImpl<IndexSequence<Ns...>, Ts...> : TupleLeaf<Ns, Ts>... {
  TupleBaseImpl() : TupleLeaf<Ns, Ts>()... {}
  explicit TupleBaseImpl(typename TupleTraits<Ts>::ParamType... args)
      : TupleLeaf<Ns, Ts>(args)... {}
};

template <size_t N, typename T>
struct TupleLeaf {
  TupleLeaf() : x() {}
  explicit TupleLeaf(typename TupleTraits<T>::ParamType x) : x(x) {}

  T& get() { return x; }
  const T& get() const { return x; }

  T x;
};

// For legacy compatibility, we name the first 8 tuple elements "a", "b", ...
// TODO(mdempsky): Update users to use get<N>() (crbug.com/440675).

#define DEFINE_TUPLE_LEAF(N, x)                                        \
  template <typename T>                                                \
  struct TupleLeaf<N, T> {                                             \
    TupleLeaf() : x() {}                                               \
    explicit TupleLeaf(typename TupleTraits<T>::ParamType x) : x(x) {} \
                                                                       \
    T& get() { return x; }                                             \
    const T& get() const { return x; }                                 \
                                                                       \
    T x;                                                               \
  }

DEFINE_TUPLE_LEAF(0, a);
DEFINE_TUPLE_LEAF(1, b);
DEFINE_TUPLE_LEAF(2, c);
DEFINE_TUPLE_LEAF(3, d);
DEFINE_TUPLE_LEAF(4, e);
DEFINE_TUPLE_LEAF(5, f);
DEFINE_TUPLE_LEAF(6, g);
DEFINE_TUPLE_LEAF(7, h);

#undef DEFINE_TUPLE_LEAF

// Deprecated compat aliases
// TODO(mdempsky): Update users to just use Tuple instead (crbug.com/440675).

using Tuple0 = Tuple<>;
template <typename A>
using Tuple1 = Tuple<A>;
template <typename A, typename B>
using Tuple2 = Tuple<A, B>;
template <typename A, typename B, typename C>
using Tuple3 = Tuple<A, B, C>;
template <typename A, typename B, typename C, typename D>
using Tuple4 = Tuple<A, B, C, D>;
template <typename A, typename B, typename C, typename D, typename E>
using Tuple5 = Tuple<A, B, C, D, E>;
template <typename A,
          typename B,
          typename C,
          typename D,
          typename E,
          typename F>
using Tuple6 = Tuple<A, B, C, D, E, F>;
template <typename A,
          typename B,
          typename C,
          typename D,
          typename E,
          typename F,
          typename G>
using Tuple7 = Tuple<A, B, C, D, E, F, G>;
template <typename A,
          typename B,
          typename C,
          typename D,
          typename E,
          typename F,
          typename G,
          typename H>
using Tuple8 = Tuple<A, B, C, D, E, F, G, H>;

// Tuple getters --------------------------------------------------------------
//
// Allows accessing an arbitrary tuple element by index.
//
// Example usage:
//   Tuple<int, double> t2;
//   get<0>(t2) = 42;
//   get<1>(t2) = 3.14;

template <size_t I, typename T>
T& get(TupleLeaf<I, T>& leaf) {
  return leaf.get();
}

template <size_t I, typename T>
const T& get(const TupleLeaf<I, T>& leaf) {
  return leaf.get();
}

// Tuple types ----------------------------------------------------------------
//
// Allows for selection of ValueTuple/RefTuple/ParamTuple without needing the
// definitions of class types the tuple takes as parameters.

template <typename T>
struct TupleTypes;

template <typename... Ts>
struct TupleTypes<Tuple<Ts...>> {
  using ValueTuple = Tuple<typename TupleTraits<Ts>::ValueType...>;
  using RefTuple = Tuple<typename TupleTraits<Ts>::RefType...>;
  using ParamTuple = Tuple<typename TupleTraits<Ts>::ParamType...>;
};

// Tuple creators -------------------------------------------------------------
//
// Helper functions for constructing tuples while inferring the template
// argument types.

template <typename... Ts>
inline Tuple<Ts...> MakeTuple(const Ts&... arg) {
  return Tuple<Ts...>(arg...);
}

// The following set of helpers make what Boost refers to as "Tiers" - a tuple
// of references.

template <typename... Ts>
inline Tuple<Ts&...> MakeRefTuple(Ts&... arg) {
  return Tuple<Ts&...>(arg...);
}

// Dispatchers ----------------------------------------------------------------
//
// Helper functions that call the given method on an object, with the unpacked
// tuple arguments.  Notice that they all have the same number of arguments,
// so you need only write:
//   DispatchToMethod(object, &Object::method, args);
// This is very useful for templated dispatchers, since they don't need to know
// what type |args| is.

// Non-Static Dispatchers with no out params.

template <typename ObjT, typename Method, typename A>
inline void DispatchToMethod(ObjT* obj, Method method, const A& arg) {
  (obj->*method)(base::internal::UnwrapTraits<A>::Unwrap(arg));
}

template <typename ObjT, typename Method, typename... Ts, size_t... Ns>
inline void DispatchToMethodImpl(ObjT* obj,
                                 Method method,
                                 const Tuple<Ts...>& arg,
                                 IndexSequence<Ns...>) {
  (obj->*method)(base::internal::UnwrapTraits<Ts>::Unwrap(get<Ns>(arg))...);
}

template <typename ObjT, typename Method, typename... Ts>
inline void DispatchToMethod(ObjT* obj,
                             Method method,
                             const Tuple<Ts...>& arg) {
  DispatchToMethodImpl(obj, method, arg, MakeIndexSequence<sizeof...(Ts)>());
}

// Static Dispatchers with no out params.

template <typename Function, typename A>
inline void DispatchToMethod(Function function, const A& arg) {
  (*function)(base::internal::UnwrapTraits<A>::Unwrap(arg));
}

template <typename Function, typename... Ts, size_t... Ns>
inline void DispatchToFunctionImpl(Function function,
                                   const Tuple<Ts...>& arg,
                                   IndexSequence<Ns...>) {
  (*function)(base::internal::UnwrapTraits<Ts>::Unwrap(get<Ns>(arg))...);
}

template <typename Function, typename... Ts>
inline void DispatchToFunction(Function function, const Tuple<Ts...>& arg) {
  DispatchToFunctionImpl(function, arg, MakeIndexSequence<sizeof...(Ts)>());
}

// Dispatchers with out parameters.

template <typename ObjT,
          typename Method,
          typename In,
          typename... OutTs,
          size_t... OutNs>
inline void DispatchToMethodImpl(ObjT* obj,
                                 Method method,
                                 const In& in,
                                 Tuple<OutTs...>* out,
                                 IndexSequence<OutNs...>) {
  (obj->*method)(base::internal::UnwrapTraits<In>::Unwrap(in),
                 &get<OutNs>(*out)...);
}

template <typename ObjT, typename Method, typename In, typename... OutTs>
inline void DispatchToMethod(ObjT* obj,
                             Method method,
                             const In& in,
                             Tuple<OutTs...>* out) {
  DispatchToMethodImpl(obj, method, in, out,
                       MakeIndexSequence<sizeof...(OutTs)>());
}

template <typename ObjT,
          typename Method,
          typename... InTs,
          typename... OutTs,
          size_t... InNs,
          size_t... OutNs>
inline void DispatchToMethodImpl(ObjT* obj,
                                 Method method,
                                 const Tuple<InTs...>& in,
                                 Tuple<OutTs...>* out,
                                 IndexSequence<InNs...>,
                                 IndexSequence<OutNs...>) {
  (obj->*method)(base::internal::UnwrapTraits<InTs>::Unwrap(get<InNs>(in))...,
                 &get<OutNs>(*out)...);
}

template <typename ObjT, typename Method, typename... InTs, typename... OutTs>
inline void DispatchToMethod(ObjT* obj,
                             Method method,
                             const Tuple<InTs...>& in,
                             Tuple<OutTs...>* out) {
  DispatchToMethodImpl(obj, method, in, out,
                       MakeIndexSequence<sizeof...(InTs)>(),
                       MakeIndexSequence<sizeof...(OutTs)>());
}

#endif  // BASE_TUPLE_H__
