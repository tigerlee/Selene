#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

#include <lua.hpp>

namespace sel {

class LuaValue;
class Table;

class Value {
 public:
  enum class Type : char
  {
    Nil,
    Boolean,
    Char,
    Integer,
    Long,
    Double,
    String,
    Table
  };

  Value();
  Value(bool value);
  Value(char value);
  Value(int value);
  Value(long value);
  Value(double value);
  Value(const char* value);
  Value(std::string value);
  Value(std::map<int, Value> value);
  Value(std::map<std::string, Value> value);
  Value(Table v);
  Value(const Value& v);

  inline bool IsNil()    const { return type() == Type::Nil; }
  inline bool IsBool()   const { return type() == Type::Boolean; }
  inline bool IsChar()   const { return type() == Type::Char;    }
  inline bool IsInt()    const { return type() == Type::Integer; }
  inline bool IsLong()   const { return type() == Type::Long;    }
  inline bool IsDouble() const { return type() == Type::Double;  }
  inline bool IsString() const { return type() == Type::String;  }
  inline bool IsTable()  const { return type() == Type::Table;   }

  inline Type               type()         const;
  inline bool               bool_value()   const;
  inline char               char_value()   const;
  inline int                int_value()    const;
  inline long               long_value()   const;
  inline double             double_value() const;
  inline const std::string& string_value() const;
  inline Table&             table_value()  const;

  Value& operator[](size_t i);
  Value& operator[](const std::string& key);
  Value& operator[](const char* key);

  operator bool()        const;
  operator char()        const;
  operator int()         const;
  operator long()        const;
  operator double()      const;
  operator std::string() const;
  operator Table()       const;

 private:
  std::shared_ptr<LuaValue> _value;
};

class Table {
 public:
  Table() {}
  Table(std::map<int, Value>& m) : _int_idx(m) {}
  Table(std::map<std::string, Value>& m) : _str_idx(m) {}

  std::map<int, Value>& int_map()         { return _int_idx; }
  std::map<std::string, Value>& str_map() { return _str_idx; }
  Value& operator[](size_t i)             { return _int_idx[i]; }
  Value& operator[](const std::string& s) { return _str_idx[s]; }

 private:
  std::map<int, Value> _int_idx;
  std::map<std::string, Value> _str_idx;
};

struct Statics {
  Value nil;
  std::string empty_string;
  Table empty_table;
};

inline Statics& statics() {
  static Statics s;
  return s;
}

class LuaValue {
 public:
  virtual ~LuaValue() {}
  virtual Value::Type        type()                   const = 0;
  virtual bool               bool_value()             const { return '\0'; }
  virtual char               char_value()             const { return '\0'; }
  virtual int                int_value()              const { return 0; }
  virtual long               long_value()             const { return 0; }
  virtual double             double_value()           const { return 0; }
  virtual const std::string& string_value()           const { return statics().empty_string; }
  virtual inline Table&      table_value()                  { return statics().empty_table; }
  virtual inline Value&      operator[](size_t)             { return statics().nil; }
  virtual inline Value&      operator[](const std::string&) { return statics().nil; }
};

template <Value::Type tag, typename T>
class BaseValue : public LuaValue {
 public:
  explicit BaseValue(const T &value) : _value(value) {}
  explicit BaseValue(const T &&value) : _value(std::move(value)) {}
  virtual ~BaseValue() {}

  virtual Value::Type type() const override {
    return tag;
  }

 protected:
  T _value;
};

class NilValue : public BaseValue<Value::Type::Nil, std::nullptr_t> {
 public:
  explicit NilValue() : BaseValue(nullptr) {}
};

class BoolValue : public BaseValue<Value::Type::Boolean, bool> {
 public:
  explicit BoolValue(bool value) : BaseValue(value) {}
  virtual inline bool bool_value() const override { return _value; }
};

class CharValue : public BaseValue<Value::Type::Char, char> {
 public:
  explicit CharValue(char value) : BaseValue(value) {}
  virtual inline char char_value() const override { return _value; }
};

class IntValue : public BaseValue<Value::Type::Integer, int> {
 public:
  explicit IntValue(int value) : BaseValue(value) {}
  virtual inline int int_value() const override { return _value; }
};

class LongValue : public BaseValue<Value::Type::Long, long> {
 public:
  explicit LongValue(long value) : BaseValue(value) {}
  virtual inline long long_value() const override { return _value; }
};

class DoubleValue : public BaseValue<Value::Type::Double, double> {
 public:
  explicit DoubleValue(double value) : BaseValue(value) {}
  virtual inline int    int_value()    const override { return static_cast<int>(_value); }
  virtual inline long   long_value()   const override { return static_cast<long>(_value); }
  virtual inline double double_value() const override { return _value; }
};

class StringValue : public BaseValue<Value::Type::String, std::string> {
 public:
  explicit StringValue(std::string value) : BaseValue(value) {}
  virtual inline char               char_value()   const override { return _value[0]; }
  virtual inline const std::string& string_value() const override { return _value; }
};

class TableValue : public BaseValue<Value::Type::Table, Table> {
 public:
  explicit TableValue(Table value) : BaseValue(value) {}
  virtual inline Table& table_value() override { return this->_value; }
  virtual inline Value& operator[](size_t i) { return this->_value[i]; }
  virtual inline Value& operator[](const std::string& s) { return this->_value[s]; }
};

inline Value::Value()                                   : _value(std::make_shared<NilValue>())         {}
inline Value::Value(bool value)                         : _value(std::make_shared<BoolValue>(value))   {}
inline Value::Value(char value)                         : _value(std::make_shared<CharValue>(value))   {}
inline Value::Value(int value)                          : _value(std::make_shared<IntValue>(value))    {}
inline Value::Value(long value)                         : _value(std::make_shared<LongValue>(value))   {}
inline Value::Value(double value)                       : _value(std::make_shared<DoubleValue>(value)) {}
inline Value::Value(const char* value)                  : _value(std::make_shared<StringValue>(std::string(value))) {}
inline Value::Value(std::string value)                  : _value(std::make_shared<StringValue>(value)) {}
inline Value::Value(std::map<int, Value> value)         : _value(std::make_shared<TableValue>(value))  {}
inline Value::Value(std::map<std::string, Value> value) : _value(std::make_shared<TableValue>(value))  {}
inline Value::Value(Table value)                        : _value(std::make_shared<TableValue>(value))  {}
inline Value::Value(const Value& v)                     : _value(v._value) {}

inline Value::Type        Value::type()                     const { return _value->type();         }
inline bool               Value::bool_value()               const { return _value->bool_value();   }
inline char               Value::char_value()               const { return _value->char_value();   }
inline int                Value::int_value()                const { return _value->int_value();    }
inline long               Value::long_value()               const { return _value->long_value();   }
inline double             Value::double_value()             const { return _value->double_value(); }
inline const std::string& Value::string_value()             const { return _value->string_value(); }
inline Table&             Value::table_value()              const { return _value->table_value();  }
inline Value&             Value::operator[](size_t i)             { return (*_value)[i]; }
inline Value&             Value::operator[](const std::string& s) { return (*_value)[s]; }
inline Value&             Value::operator[](const char* s)        { return (*_value)[std::string(s)]; }

inline Value::operator bool()        const { return bool_value();   }
inline Value::operator char()        const { return char_value();   }
inline Value::operator int()         const { return int_value();    }
inline Value::operator long()        const { return long_value();   }
inline Value::operator double()      const { return double_value(); }
inline Value::operator std::string() const { return string_value(); }
inline Value::operator Table()       const { return table_value();  }

}
