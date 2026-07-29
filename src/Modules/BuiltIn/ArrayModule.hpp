// ArrayModule.hpp
#ifndef MODULES_ARRAYMODULE_HPP
#define MODULES_ARRAYMODULE_HPP

#include <algorithm>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Interpreter/Interpreter.hpp"
#include "Modules/BaseModule.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"
#include "Symbols/RegistrationMacros.hpp"

namespace Modules {

/**
 * @brief Module providing a sizeof() function for array variables.
 * Usage:
 *   sizeof($array)   -> returns number of elements in the array
 */
class ArrayModule : public BaseModule {
  public:
    ArrayModule() {
        setModuleName("Array");
        setDescription("Provides array and object manipulation functions, including size operations for collections");
        setBuiltIn(true);
    }

    void registerFunctions() override {
        std::vector<Symbols::FunctionParameterInfo> params = {
            { "array", Symbols::Variables::Type::OBJECT, "The array/object to get the size of", false, false }
        };

        REGISTER_FUNCTION("sizeof", Symbols::Variables::Type::INTEGER, params, "Get the size of an array or object",
                          Modules::ArrayModule::SizeOf);

        using T = Symbols::Variables::Type;
        std::vector<Symbols::FunctionParameterInfo> arr1     = { { "array", T::OBJECT, "The array" } };
        std::vector<Symbols::FunctionParameterInfo> arr_fn   = { { "array", T::OBJECT, "The array" },
                                                                 { "callback", T::STRING, "Name of the callback function" } };
        std::vector<Symbols::FunctionParameterInfo> arr_val  = { { "array", T::OBJECT, "The array" },
                                                                 { "value", T::OBJECT, "The value" } };

        REGISTER_FUNCTION("array_map", T::OBJECT, arr_fn,
                          "Apply callback(value) to every element; returns a new array",
                          Modules::ArrayModule::Map);
        REGISTER_FUNCTION("array_filter", T::OBJECT, arr_fn,
                          "Keep elements where callback(value) is truthy; returns a new array",
                          Modules::ArrayModule::Filter);
        REGISTER_FUNCTION("array_reduce", T::OBJECT, arr_fn,
                          "Reduce with callback(carry, value); optional 3rd arg is the initial carry",
                          Modules::ArrayModule::Reduce);
        REGISTER_FUNCTION("array_sort", T::OBJECT, arr1,
                          "Sort ascending (numeric when both numeric, else string); returns a new array",
                          Modules::ArrayModule::Sort);
        REGISTER_FUNCTION("array_usort", T::OBJECT, arr_fn,
                          "Sort using callback(a, b) -> negative/zero/positive; returns a new array",
                          Modules::ArrayModule::Usort);
        REGISTER_FUNCTION("array_keys", T::OBJECT, arr1, "Return the array's keys",
                          Modules::ArrayModule::Keys);
        REGISTER_FUNCTION("array_values", T::OBJECT, arr1, "Return the array's values, reindexed from 0",
                          Modules::ArrayModule::Values);
        REGISTER_FUNCTION("array_reverse", T::OBJECT, arr1, "Return the array reversed",
                          Modules::ArrayModule::Reverse);
        REGISTER_FUNCTION("array_slice", T::OBJECT, arr1,
                          "array_slice(array, offset [, length]) -> sub-array (negative offset counts from end)",
                          Modules::ArrayModule::Slice);
        REGISTER_FUNCTION("array_merge", T::OBJECT, arr1, "Concatenate two or more arrays into one",
                          Modules::ArrayModule::Merge);
        REGISTER_FUNCTION("array_unique", T::OBJECT, arr1, "Return the array with duplicate values removed",
                          Modules::ArrayModule::Unique);
        REGISTER_FUNCTION("array_flip", T::OBJECT, arr1, "Swap keys and values",
                          Modules::ArrayModule::Flip);
        REGISTER_FUNCTION("in_array", T::BOOLEAN, arr_val, "in_array(array, value) -> whether value is present",
                          Modules::ArrayModule::InArray);
        REGISTER_FUNCTION("array_contains", T::BOOLEAN, arr_val, "array_contains(array, value) -> whether value is present",
                          Modules::ArrayModule::InArray);
    }

    // --- helpers ---------------------------------------------------------------------

    // Arrays are ObjectMaps keyed by the decimal index, but the map orders keys
    // lexicographically ("10" < "2"), so the elements must be read back by numeric index.
    static std::vector<Symbols::ValuePtr> toVector(const Symbols::ValuePtr & arr, const char * fn) {
        if (arr->getType() != Symbols::Variables::Type::OBJECT &&
            arr->getType() != Symbols::Variables::Type::CLASS) {
            throw std::runtime_error(std::string(fn) + ": expects an array");
        }
        const auto &                   map = arr->get<Symbols::ObjectMap>();
        std::vector<Symbols::ValuePtr> out;
        for (size_t i = 0;; ++i) {
            auto it = map.find(std::to_string(i));
            if (it == map.end()) {
                break;
            }
            out.push_back(it->second);
        }
        return out;
    }

    // (key, value) pairs in a sensible order: numeric-indexed entries first, by index
    // (so a >10-element array is not reordered "10" < "2"), then any remaining
    // associative entries in the map's own order. Works for arrays and objects alike.
    static std::vector<std::pair<std::string, Symbols::ValuePtr>> orderedEntries(const Symbols::ObjectMap & map) {
        std::vector<std::pair<std::string, Symbols::ValuePtr>> out;
        std::set<std::string>                                  used;
        for (size_t i = 0;; ++i) {
            const std::string key = std::to_string(i);
            auto              it  = map.find(key);
            if (it == map.end()) {
                break;
            }
            out.emplace_back(key, it->second);
            used.insert(key);
        }
        for (const auto & kv : map) {
            if (used.find(kv.first) == used.end()) {
                out.emplace_back(kv.first, kv.second);
            }
        }
        return out;
    }

    static Symbols::ValuePtr fromVector(const std::vector<Symbols::ValuePtr> & v) {
        Symbols::ObjectMap m;
        for (size_t i = 0; i < v.size(); ++i) {
            m[std::to_string(i)] = v[i];
        }
        return Symbols::ValuePtr(m);
    }

    // Ascending compare: numeric when both are numeric, otherwise by string form.
    static int compareValues(const Symbols::ValuePtr & a, const Symbols::ValuePtr & b) {
        const auto numeric = [](const Symbols::ValuePtr & v, double & out) {
            switch (v->getType()) {
                case Symbols::Variables::Type::INTEGER: out = static_cast<double>(v->get<int>());   return true;
                case Symbols::Variables::Type::FLOAT:   out = static_cast<double>(v->get<float>()); return true;
                case Symbols::Variables::Type::DOUBLE:  out = v->get<double>();                     return true;
                default:                                                                            return false;
            }
        };
        double na = 0, nb = 0;
        if (numeric(a, na) && numeric(b, nb)) {
            return (na < nb) ? -1 : (na > nb) ? 1 : 0;
        }
        const std::string sa = a->toString();
        const std::string sb = b->toString();
        return sa.compare(sb) < 0 ? -1 : sa.compare(sb) > 0 ? 1 : 0;
    }

    // --- array functions -------------------------------------------------------------

    static Symbols::ValuePtr Map(Symbols::FunctionArguments & args) {
        if (args.size() != 2 || args[1]->getType() != Symbols::Variables::Type::STRING) {
            throw std::runtime_error("array_map expects (array, string callback)");
        }
        const std::string              cb = args[1]->get<std::string>();
        std::vector<Symbols::ValuePtr> in = toVector(args[0], "array_map");
        std::vector<Symbols::ValuePtr> out;
        out.reserve(in.size());
        for (auto & el : in) {
            out.push_back(Interpreter::Interpreter::callUserFunction(cb, { el }));
        }
        return fromVector(out);
    }

    static Symbols::ValuePtr Filter(Symbols::FunctionArguments & args) {
        if (args.size() != 2 || args[1]->getType() != Symbols::Variables::Type::STRING) {
            throw std::runtime_error("array_filter expects (array, string callback)");
        }
        const std::string              cb = args[1]->get<std::string>();
        std::vector<Symbols::ValuePtr> in = toVector(args[0], "array_filter");
        std::vector<Symbols::ValuePtr> out;
        for (auto & el : in) {
            if (Interpreter::Interpreter::callUserFunction(cb, { el }).toBool()) {
                out.push_back(el);
            }
        }
        return fromVector(out);
    }

    static Symbols::ValuePtr Reduce(Symbols::FunctionArguments & args) {
        if (args.size() < 2 || args.size() > 3 || args[1]->getType() != Symbols::Variables::Type::STRING) {
            throw std::runtime_error("array_reduce expects (array, string callback [, initial])");
        }
        const std::string              cb   = args[1]->get<std::string>();
        std::vector<Symbols::ValuePtr> in   = toVector(args[0], "array_reduce");
        Symbols::ValuePtr              carry = (args.size() == 3) ? args[2] : Symbols::ValuePtr::null();
        for (auto & el : in) {
            carry = Interpreter::Interpreter::callUserFunction(cb, { carry, el });
        }
        return carry;
    }

    static Symbols::ValuePtr Sort(Symbols::FunctionArguments & args) {
        if (args.size() != 1) {
            throw std::runtime_error("array_sort expects (array)");
        }
        std::vector<Symbols::ValuePtr> v = toVector(args[0], "array_sort");
        std::stable_sort(v.begin(), v.end(),
                         [](const Symbols::ValuePtr & a, const Symbols::ValuePtr & b) { return compareValues(a, b) < 0; });
        return fromVector(v);
    }

    static Symbols::ValuePtr Usort(Symbols::FunctionArguments & args) {
        if (args.size() != 2 || args[1]->getType() != Symbols::Variables::Type::STRING) {
            throw std::runtime_error("array_usort expects (array, string comparator)");
        }
        const std::string              cb = args[1]->get<std::string>();
        std::vector<Symbols::ValuePtr> v  = toVector(args[0], "array_usort");
        std::stable_sort(v.begin(), v.end(), [&cb](const Symbols::ValuePtr & a, const Symbols::ValuePtr & b) {
            return Interpreter::Interpreter::callUserFunction(cb, { a, b })->get<int>() < 0;
        });
        return fromVector(v);
    }

    static Symbols::ValuePtr Keys(Symbols::FunctionArguments & args) {
        if (args.size() != 1 || (args[0]->getType() != Symbols::Variables::Type::OBJECT &&
                                 args[0]->getType() != Symbols::Variables::Type::CLASS)) {
            throw std::runtime_error("array_keys expects (array)");
        }
        std::vector<Symbols::ValuePtr> keys;
        for (const auto & kv : orderedEntries(args[0]->get<Symbols::ObjectMap>())) {
            keys.push_back(Symbols::ValuePtr(kv.first));
        }
        return fromVector(keys);
    }

    static Symbols::ValuePtr Values(Symbols::FunctionArguments & args) {
        if (args.size() != 1 || (args[0]->getType() != Symbols::Variables::Type::OBJECT &&
                                 args[0]->getType() != Symbols::Variables::Type::CLASS)) {
            throw std::runtime_error("array_values expects (array)");
        }
        std::vector<Symbols::ValuePtr> vals;
        for (const auto & kv : orderedEntries(args[0]->get<Symbols::ObjectMap>())) {
            vals.push_back(kv.second);
        }
        return fromVector(vals);
    }

    static Symbols::ValuePtr Reverse(Symbols::FunctionArguments & args) {
        if (args.size() != 1) {
            throw std::runtime_error("array_reverse expects (array)");
        }
        std::vector<Symbols::ValuePtr> v = toVector(args[0], "array_reverse");
        std::reverse(v.begin(), v.end());
        return fromVector(v);
    }

    static Symbols::ValuePtr Slice(Symbols::FunctionArguments & args) {
        if (args.size() < 2 || args.size() > 3) {
            throw std::runtime_error("array_slice expects (array, offset [, length])");
        }
        std::vector<Symbols::ValuePtr> v = toVector(args[0], "array_slice");
        const int                      n = static_cast<int>(v.size());
        int                            offset = args[1]->get<int>();
        if (offset < 0) {
            offset = std::max(0, n + offset);
        }
        offset = std::min(offset, n);
        int length = (args.size() == 3) ? args[2]->get<int>() : (n - offset);
        if (length < 0) {
            length = std::max(0, (n + length) - offset);
        }
        length = std::min(length, n - offset);
        std::vector<Symbols::ValuePtr> out(v.begin() + offset, v.begin() + offset + length);
        return fromVector(out);
    }

    static Symbols::ValuePtr Merge(Symbols::FunctionArguments & args) {
        if (args.empty()) {
            throw std::runtime_error("array_merge expects at least one array");
        }
        std::vector<Symbols::ValuePtr> out;
        for (size_t i = 0; i < args.size(); ++i) {
            std::vector<Symbols::ValuePtr> v = toVector(args[i], "array_merge");
            out.insert(out.end(), v.begin(), v.end());
        }
        return fromVector(out);
    }

    static Symbols::ValuePtr Unique(Symbols::FunctionArguments & args) {
        if (args.size() != 1) {
            throw std::runtime_error("array_unique expects (array)");
        }
        std::vector<Symbols::ValuePtr> v = toVector(args[0], "array_unique");
        std::vector<Symbols::ValuePtr> out;
        std::vector<std::string>       seen;
        for (auto & el : v) {
            const std::string key = el->toString();
            if (std::find(seen.begin(), seen.end(), key) == seen.end()) {
                seen.push_back(key);
                out.push_back(el);
            }
        }
        return fromVector(out);
    }

    static Symbols::ValuePtr Flip(Symbols::FunctionArguments & args) {
        if (args.size() != 1 || (args[0]->getType() != Symbols::Variables::Type::OBJECT &&
                                 args[0]->getType() != Symbols::Variables::Type::CLASS)) {
            throw std::runtime_error("array_flip expects (array)");
        }
        Symbols::ObjectMap out;
        for (const auto & kv : args[0]->get<Symbols::ObjectMap>()) {
            out[kv.second->toString()] = Symbols::ValuePtr(kv.first);
        }
        return Symbols::ValuePtr(out);
    }

    static Symbols::ValuePtr InArray(Symbols::FunctionArguments & args) {
        if (args.size() != 2) {
            throw std::runtime_error("in_array expects (array, value)");
        }
        const std::string             needle = args[1]->toString();
        for (auto & el : toVector(args[0], "in_array")) {
            if (el->toString() == needle) {
                return Symbols::ValuePtr(true);
            }
        }
        return Symbols::ValuePtr(false);
    }

    static Symbols::ValuePtr SizeOf(Symbols::FunctionArguments & args) {
        if (args.size() != 1) {
            throw std::runtime_error("sizeof expects exactly one argument");
        }
        const auto & val  = args[0];
        auto         type = val->getType();
        switch (type) {
            case Symbols::Variables::Type::OBJECT:
                {
                    const auto & map = val->get<Symbols::ObjectMap>();
                    return static_cast<int>(map.size());
                }
            case Symbols::Variables::Type::STRING:
                {
                    const auto & str = val->get<std::string>();
                    return static_cast<int>(str.size());
                }
            case Symbols::Variables::Type::CLASS:
                {
                    const auto & map = val->get<Symbols::ObjectMap>();
                    return static_cast<int>(map.size());
                }
            case Symbols::Variables::Type::INTEGER:
            case Symbols::Variables::Type::DOUBLE:
            case Symbols::Variables::Type::FLOAT:
            case Symbols::Variables::Type::BOOLEAN:
                {
                    return 1;
                }
            default:
                throw std::runtime_error("sizeof unsupported type: "+Symbols::Variables::TypeToString(val));
        }
    }  // SizeOf
};

}  // namespace Modules

#endif  // MODULES_ARRAYMODULE_HPP
