#pragma once

#include <string>
#include "traits.h"
#include "MetatableRegistry.h"
#include "Value.h"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

/* The purpose of this header is to handle pushing and retrieving
 * primitives from the stack
 */

namespace sel {
namespace detail {

template <typename T>
struct is_primitive {
    static constexpr bool value = false;
};
template <>
struct is_primitive<char> {
    static constexpr bool value = true;
};
template <>
struct is_primitive<int> {
    static constexpr bool value = true;
};
template <>
struct is_primitive<unsigned int> {
    static constexpr bool value = true;
};
template <>
struct is_primitive<long> {
    static constexpr bool value = true;
};
template <>
struct is_primitive<long unsigned int> {
    static constexpr bool value = true;
};
template <>
struct is_primitive<bool> {
    static constexpr bool value = true;
};
template <>
struct is_primitive<lua_Number> {
    static constexpr bool result = true;
};
template <>
struct is_primitive<std::string> {
    static constexpr bool value = true;
};

/* getters */
template <typename T>
inline T* _get(_id<T*>, lua_State *l, const int index) {
    auto t = lua_topointer(l, index);
    return (T*)(t);
}

inline bool _get(_id<bool>, lua_State *l, const int index) {
    return lua_toboolean(l, index) != 0;
}

inline char _get(_id<char>, lua_State *l, const int index) {
    return *lua_tostring(l, index);
}

inline int _get(_id<int>, lua_State *l, const int index) {
    return lua_tointeger(l, index);
}

inline unsigned int _get(_id<unsigned int>, lua_State *l, const int index) {
#if LUA_VERSION_NUM >= 502
    return lua_tounsigned(l, index);
#else
    return static_cast<unsigned>(lua_tointeger(l, index));
#endif
}

inline long _get(_id<long>, lua_State *l, const int index) {
  return lua_tointeger(l, index);
}

inline long unsigned int _get(_id<long unsigned int>, lua_State *l, const int index) {
#if LUA_VERSION_NUM >= 502
    return lua_tounsigned(l, index);
#else
    return static_cast<long unsigned>(lua_tointeger(l, index));
#endif
}

inline lua_Number _get(_id<lua_Number>, lua_State *l, const int index) {
    return lua_tonumber(l, index);
}

inline std::string _get(_id<std::string>, lua_State *l, const int index) {
    size_t size;
    const char *buff = lua_tolstring(l, index, &size);
    return std::string{buff, size};
}

inline Value _get(_id<Value>, lua_State *l, const int index) {
  if (lua_isnil(l, index))     { return Value(); }
  if (lua_isboolean(l, index)) { return Value(_get(_id<bool>{}, l, index)); }
  if (lua_isnumber(l, index))  { return Value(_get(_id<double>{}, l, index)); }
  if (lua_isstring(l, index))  { return Value(_get(_id<std::string>{}, l, index)); }
  if (lua_istable(l, index)) {
    Table t;
    luaL_checktype(l, index, LUA_TTABLE);
    lua_pushnil(l);
    while (lua_next(l, -2)) {
      if (lua_isnumber(l, -2)) {
        int idx = _get(_id<int>{}, l, -2);
        t[idx] = _get(_id<Value>{}, l, -1);
      } else if (lua_isstring(l, -2)) {
        std::string idx = _get(_id<std::string>{}, l, -2);
        t[idx] = _get(_id<Value>{}, l, -1);
      }
      lua_pop(l, 1);
    }
    return t;
  }
  return Value();
}

template <typename T>
inline T* _check_get(_id<T*>, lua_State *l, const int index) {
    return (T *)lua_topointer(l, index);
};

template <typename T>
inline T& _check_get(_id<T&>, lua_State *l, const int index) {
    static_assert(!is_primitive<T>::value,
                  "Reference types must not be primitives.");
    return *(T *)lua_topointer(l, index);
};

inline char _check_get(_id<char>, lua_State *l, const int index) {
    return *luaL_checkstring(l, index);
};

inline int _check_get(_id<int>, lua_State *l, const int index) {
    return luaL_checkint(l, index);
};

inline unsigned int _check_get(_id<unsigned int>, lua_State *l, const int index) {
#if LUA_VERSION_NUM >= 502
    return luaL_checkunsigned(l, index);
#else
    return static_cast<unsigned>(luaL_checkint(l, index));
#endif
}

inline long _check_get(_id<long>, lua_State *l, const int index) {
    return luaL_checklong(l, index);
}

inline long unsigned int _check_get(_id<long unsigned int>, lua_State *l, const int index) {
    return luaL_checklong(l, index);
}

inline lua_Number _check_get(_id<lua_Number>, lua_State *l, const int index) {
    return luaL_checknumber(l, index);
}

inline bool _check_get(_id<bool>, lua_State *l, const int index) {
    return lua_toboolean(l, index) != 0;
}

inline std::string _check_get(_id<std::string>, lua_State *l, const int index) {
    size_t size;
    const char *buff = luaL_checklstring(l, index, &size);
    return std::string{buff, size};
}

inline Value _check_get(_id<Value>, lua_State *l, const int index) {
  return _get(_id<Value>{}, l, index);
}

// Worker type-trait struct to _pop_n
// Popping multiple elements returns a tuple
template <std::size_t S, typename... Ts> // First template argument denotes
                                         // the sizeof(Ts...)
struct _pop_n_impl {
    using type =  std::tuple<Ts...>;

    template <std::size_t... N>
    static type worker(lua_State *l,
                       _indices<N...>) {
        return std::make_tuple(_get(_id<Ts>{}, l, N + 1)...);
    }

    static type apply(lua_State *l) {
        auto ret = worker(l, typename _indices_builder<S>::type());
        lua_pop(l, S);
        return ret;
    }
};

// Popping nothing returns void
template <typename... Ts>
struct _pop_n_impl<0, Ts...> {
    using type = void;
    static type apply(lua_State *) {}
};

// Popping one element returns an unboxed value
template <typename T>
struct _pop_n_impl<1, T> {
    using type = T;
    static type apply(lua_State *l) {
        T ret = _get(_id<T>{}, l, -1);
        lua_pop(l, 1);
        return ret;
    }
};

template <typename... T>
typename _pop_n_impl<sizeof...(T), T...>::type _pop_n(lua_State *l) {
    return _pop_n_impl<sizeof...(T), T...>::apply(l);
}

template <std::size_t S, typename... Ts>
struct _pop_n_reset_impl {
    using type =  std::tuple<Ts...>;

    template <std::size_t... N>
    static type worker(lua_State *l,
                       _indices<N...>) {
        return std::make_tuple(_get(_id<Ts>{}, l, N + 1)...);
    }

    static type apply(lua_State *l) {
        auto ret = worker(l, typename _indices_builder<S>::type());
        lua_settop(l, 0);
        return ret;
    }
};

// Popping nothing returns void
template <typename... Ts>
struct _pop_n_reset_impl<0, Ts...> {
    using type = void;
    static type apply(lua_State *l) {
        lua_settop(l, 0);
    }
};

// Popping one element returns an unboxed value
template <typename T>
struct _pop_n_reset_impl<1, T> {
    using type = T;
    static type apply(lua_State *l) {
        T ret = _get(_id<T>{}, l, -1);
        lua_settop(l, 0);
        return ret;
    }
};

template <typename... T>
typename _pop_n_reset_impl<sizeof...(T), T...>::type
_pop_n_reset(lua_State *l) {
    return _pop_n_reset_impl<sizeof...(T), T...>::apply(l);
}

template <typename T>
T _pop(_id<T> t, lua_State *l) {
    T ret =  _get(t, l, -1);
    lua_pop(l, 1);
    return ret;
}

/* Setters */

inline void _push(lua_State *l) {}

template <typename T>
inline void _push(lua_State *l, MetatableRegistry &m, T* t) {
	if(t == nullptr) {
		lua_pushnil(l);
	}
	else {
		lua_pushlightuserdata(l, t);
		if (const std::string* metatable = m.Find(typeid(T))) {
			luaL_setmetatable(l, metatable->c_str());
		}
	}
}

template <typename T>
inline void _push(lua_State *l, MetatableRegistry &m, T& t) {
    lua_pushlightuserdata(l, &t);
    if (const std::string* metatable = m.Find(typeid(T))) {
        luaL_setmetatable(l, metatable->c_str());
    }
}

inline void _push(lua_State *l, MetatableRegistry &, bool b) {
    lua_pushboolean(l, b);
}

inline void _push(lua_State *l, MetatableRegistry &, char c) {
    lua_pushlstring(l, &c, 1);
}

inline void _push(lua_State *l, MetatableRegistry &, int i) {
    lua_pushinteger(l, i);
}

inline void _push(lua_State *l, MetatableRegistry &, unsigned int u) {
#if LUA_VERSION_NUM >= 502
    lua_pushunsigned(l, u);
#else
    lua_pushinteger(l, static_cast<int>(u));
#endif
}

inline void _push(lua_State *l, MetatableRegistry &, long i) {
    lua_pushinteger(l, i);
}

inline void _push(lua_State *l, MetatableRegistry &, long unsigned int u) {
#if LUA_VERSION_NUM >= 502
    lua_pushunsigned(l, u);
#else
    lua_pushinteger(l, static_cast<long>(u));
#endif
}

inline void _push(lua_State *l, MetatableRegistry &, lua_Number f) {
    lua_pushnumber(l, f);
}

inline void _push(lua_State *l, MetatableRegistry &, const std::string &s) {
    lua_pushlstring(l, s.c_str(), s.size());
}

inline void _push(lua_State *l, MetatableRegistry &, const char *s) {
    lua_pushstring(l, s);
}

template <typename T>
inline void _push(lua_State *l, T* t) {
	if(t == nullptr) {
		lua_pushnil(l);
	}
	else {
		lua_pushlightuserdata(l, t);
	}
}

template <typename T>
inline void _push(lua_State *l, T& t) {
    lua_pushlightuserdata(l, &t);
}

inline void _push(lua_State *l, bool b) {
    lua_pushboolean(l, b);
}

inline void _push(lua_State *l, char c) {
    lua_pushlstring(l, &c, 1);
}

inline void _push(lua_State *l, int i) {
    lua_pushinteger(l, i);
}

inline void _push(lua_State *l, unsigned int u) {
#if LUA_VERSION_NUM >= 502
    lua_pushunsigned(l, u);
#else
    lua_pushinteger(l, static_cast<int>(u));
#endif
}

inline void _push(lua_State *l, long i) {
    lua_pushinteger(l, i);
}

inline void _push(lua_State *l, long unsigned int u) {
    lua_pushinteger(l, u);
}

inline void _push(lua_State *l, lua_Number f) {
    lua_pushnumber(l, f);
}

inline void _push(lua_State *l, const std::string &s) {
    lua_pushlstring(l, s.c_str(), s.size());
}

inline void _push(lua_State *l, const char *s) {
    lua_pushstring(l, s);
}

inline void _push(lua_State *l, Value value) {
    if (value.IsBool())   { _push(l, value.bool_value());   }
    if (value.IsChar())   { _push(l, value.char_value());   }
    if (value.IsInt())    { _push(l, value.int_value());    }
    if (value.IsLong())   { _push(l, value.long_value());   }
    if (value.IsDouble()) { _push(l, value.double_value()); }
    if (value.IsString()) { _push(l, value.string_value()); }
    if (value.IsTable()) {
        auto im = value.table_value().int_map();
        auto sm = value.table_value().str_map();
        lua_createtable(l, im.size(), sm.size());
        for (auto item : im) {
            if (!item.second.IsNil()) {
              _push(l, item.second);
              lua_rawseti(l, -2, item.first);
            }
        }
        for (auto item : sm) {
            if (!item.second.IsNil()) {
                _push(l, item.second);
                lua_setfield(l, -2, item.first.data());
            }
        }
    }
}

inline void _push(lua_State *l, MetatableRegistry &, Value&& value) {
  _push(l, std::forward<Value>(value));
}

template <typename T>
inline void _set(lua_State *l, T &&value, const int index) {
    _push(l, std::forward<T>(value));
    lua_replace(l, index);
}

inline void _push_n(lua_State *, MetatableRegistry &) {}

template <typename T, typename... Rest>
inline void _push_n(lua_State *l, MetatableRegistry &m, T value, Rest... rest) {
    _push(l, m, std::forward<T>(value));
    _push_n(l, m, rest...);
}

template <typename... T, std::size_t... N>
inline void _push_dispatcher(lua_State *l,
                             MetatableRegistry &m,
                             const std::tuple<T...> &values,
                             _indices<N...>) {
    _push_n(l, m, std::get<N>(values)...);
}

inline void _push(lua_State *l, MetatableRegistry &, std::tuple<>) {}

template <typename... T>
inline void _push(lua_State *l, MetatableRegistry &m, const std::tuple<T...> &values) {
    constexpr int num_values = sizeof...(T);
    _push_dispatcher(l, m, values,
                     typename _indices_builder<num_values>::type());
}

inline void _push_n(lua_State *) {}

template <typename T, typename... Rest>
inline void _push_n(lua_State *l, T value, Rest... rest) {
    _push(l, std::forward<T>(value));
    _push_n(l, rest...);
}

template <typename... T, std::size_t... N>
inline void _push_dispatcher(lua_State *l,
                             const std::tuple<T...> &values,
                             _indices<N...>) {
    _push_n(l, std::get<N>(values)...);
}

inline void _push(lua_State *l, std::tuple<>) {}

template <typename... T>
inline void _push(lua_State *l, const std::tuple<T...> &values) {
    constexpr int num_values = sizeof...(T);
    _push_dispatcher(l, values,
                     typename _indices_builder<num_values>::type());
}
}
}
