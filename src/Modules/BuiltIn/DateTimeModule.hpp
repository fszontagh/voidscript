#ifndef MODULES_DATETIMEMODULE_HPP
#define MODULES_DATETIMEMODULE_HPP

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Modules/BaseModule.hpp"
#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"

namespace Modules {

/**
 * @brief Date/time for VoidScript.
 *
 * Free functions:
 *   current_unix_timestamp()                 -> int (seconds since the epoch)
 *   date([format [, timestamp]])             -> string (strftime; default "%Y-%m-%d %H:%M:%S", now)
 *   date_parse(text, format)                 -> int timestamp (strptime, local time)
 *
 * class DateTime (holds a Unix timestamp, all methods work off it):
 *   new DateTime([timestamp])                // default: now
 *   year/month/day/hour/minute/second()      -> int
 *   dayOfWeek()                              -> int (0=Sunday .. 6=Saturday)
 *   timestamp() / getTimestamp()             -> int;  setTimestamp(int) -> self
 *   addSeconds/addMinutes/addHours/addDays(int)     -> self (mutates in place)
 *   addMonths/addYears(int)                  -> self (calendar-aware via mktime)
 *   format(fmt)                              -> string (strftime)
 *   diff(other)                              -> int (this minus other, in seconds)
 */
class DateTimeModule : public BaseModule {
  public:
    DateTimeModule() {
        setModuleName("DateTime");
        setDescription("Date and time: current_unix_timestamp/date/date_parse, and a DateTime class with "
                       "getters, in-place arithmetic, strftime formatting and diff");
        setBuiltIn(true);
    }

    void registerFunctions() override {
        registerBuiltInFunctions();
        registerDateTimeClass();
    }

  private:
    // --- helpers ---------------------------------------------------------------------

    static std::tm localTm(std::time_t t) {
        return *std::localtime(&t);
    }

    static std::string strftimeFmt(const std::tm & tm, const std::string & fmt) {
        std::ostringstream oss;
        oss << std::put_time(&tm, fmt.c_str());
        return oss.str();
    }

    // Read the timestamp stored on the instance (args[0]).
    long readTs(Symbols::FunctionArguments & args, const char * method) {
        if (args.empty() ||
            (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT)) {
            throw std::runtime_error(std::string("DateTime::") + method + " must be called on a DateTime instance");
        }
        const auto & m  = args[0]->get<Symbols::ObjectMap>();
        auto         it = m.find("__timestamp__");
        if (it == m.end()) {
            throw std::runtime_error("DateTime object missing __timestamp__ property");
        }
        return static_cast<long>(it->second->get<int>());
    }

    // Write the timestamp back onto the instance and return it (for in-place mutators).
    // Copy the handle to a non-const local first: ValuePtr shares the underlying Value, so
    // this mutates the caller's object, and it sidesteps args being a const container.
    Symbols::ValuePtr writeTs(Symbols::FunctionArguments & args, long ts) {
        Symbols::ValuePtr self = args[0];
        self->get<Symbols::ObjectMap>()["__timestamp__"] = Symbols::ValuePtr(static_cast<int>(ts));
        return self;
    }

    // --- free functions --------------------------------------------------------------

    void registerBuiltInFunctions() {
        std::vector<Symbols::FunctionParameterInfo> none = {};
        REGISTER_FUNCTION("current_unix_timestamp", Symbols::Variables::Type::INTEGER, none,
                          "Current Unix timestamp (seconds) as an integer",
                          [this](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (!args.empty()) {
                                  throw std::runtime_error(name() + "::current_unix_timestamp expects no arguments");
                              }
                              auto now = std::chrono::system_clock::now();
                              return static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(
                                  now.time_since_epoch()).count());
                          });

        std::vector<Symbols::FunctionParameterInfo> date_params = {
            { "format", Symbols::Variables::Type::STRING, "strftime format (default \"%Y-%m-%d %H:%M:%S\")", true },
            { "timestamp", Symbols::Variables::Type::INTEGER, "Unix timestamp (default: now)", true }
        };
        REGISTER_FUNCTION("date", Symbols::Variables::Type::STRING, date_params,
                          "Format a timestamp (default now) with strftime placeholders",
                          [this](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              std::string fmt = "%Y-%m-%d %H:%M:%S";
                              if (!args.empty()) {
                                  if (args[0] != Symbols::Variables::Type::STRING) {
                                      throw std::runtime_error(name() + "::date: format must be a string");
                                  }
                                  fmt = args[0]->get<std::string>();
                              }
                              std::time_t t;
                              if (args.size() >= 2 && args[1] == Symbols::Variables::Type::INTEGER) {
                                  t = static_cast<std::time_t>(args[1]->get<int>());
                              } else {
                                  t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                              }
                              return strftimeFmt(localTm(t), fmt);
                          });

        std::vector<Symbols::FunctionParameterInfo> parse_params = {
            { "text", Symbols::Variables::Type::STRING, "The date/time text" },
            { "format", Symbols::Variables::Type::STRING, "strptime format, e.g. \"%Y-%m-%d %H:%M:%S\"" }
        };
        REGISTER_FUNCTION("date_parse", Symbols::Variables::Type::INTEGER, parse_params,
                          "Parse text with a strptime format into a Unix timestamp (local time)",
                          [this](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 2 || args[0] != Symbols::Variables::Type::STRING ||
                                  args[1] != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error(name() + "::date_parse expects (string text, string format)");
                              }
                              const std::string text = args[0]->get<std::string>();
                              const std::string fmt  = args[1]->get<std::string>();
                              std::tm            tm{};
                              tm.tm_isdst = -1;  // let mktime work out DST
                              std::istringstream iss(text);
                              iss >> std::get_time(&tm, fmt.c_str());
                              if (iss.fail()) {
                                  throw std::runtime_error(name() + "::date_parse: '" + text +
                                                           "' does not match format '" + fmt + "'");
                              }
                              return static_cast<int>(std::mktime(&tm));
                          });
    }

    // --- DateTime class --------------------------------------------------------------

    void registerDateTimeClass() {
        REGISTER_CLASS("DateTime");

        std::vector<Symbols::FunctionParameterInfo> none = {};
        std::vector<Symbols::FunctionParameterInfo> ctor_params = {
            { "timestamp", Symbols::Variables::Type::INTEGER, "Unix timestamp (default: now)", true }
        };
        REGISTER_METHOD("DateTime", "__construct", ctor_params,
                        [this](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                            if (args.empty() || (args[0] != Symbols::Variables::Type::CLASS &&
                                                 args[0] != Symbols::Variables::Type::OBJECT)) {
                                throw std::runtime_error("DateTime::__construct must be called on a DateTime instance");
                            }
                            long ts;
                            if (args.size() >= 2 && args[1] == Symbols::Variables::Type::INTEGER) {
                                ts = args[1]->get<int>();
                            } else {
                                ts = static_cast<long>(std::chrono::duration_cast<std::chrono::seconds>(
                                    std::chrono::system_clock::now().time_since_epoch()).count());
                            }
                            return writeTs(args, ts);  // mutates args[0] in place, returns it
                        },
                        Symbols::Variables::Type::CLASS, "Create a DateTime (default: now)");

        // Field getters.
        registerGetter("year",   [](const std::tm & tm) { return tm.tm_year + 1900; });
        registerGetter("month",  [](const std::tm & tm) { return tm.tm_mon + 1; });
        registerGetter("day",    [](const std::tm & tm) { return tm.tm_mday; });
        registerGetter("hour",   [](const std::tm & tm) { return tm.tm_hour; });
        registerGetter("minute", [](const std::tm & tm) { return tm.tm_min; });
        registerGetter("second", [](const std::tm & tm) { return tm.tm_sec; });
        registerGetter("dayOfWeek", [](const std::tm & tm) { return tm.tm_wday; });

        // timestamp() / getTimestamp()
        for (const char * getterName : { "timestamp", "getTimestamp" }) {
            REGISTER_METHOD("DateTime", getterName, none,
                            ([this, getterName](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                                return Symbols::ValuePtr(static_cast<int>(readTs(args, getterName)));
                            }),
                            Symbols::Variables::Type::INTEGER, "The instance's Unix timestamp");
        }

        std::vector<Symbols::FunctionParameterInfo> set_params = {
            { "timestamp", Symbols::Variables::Type::INTEGER, "Unix timestamp" }
        };
        REGISTER_METHOD("DateTime", "setTimestamp", set_params,
                        [this](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                            if (args.size() != 2 || args[1] != Symbols::Variables::Type::INTEGER) {
                                throw std::runtime_error("DateTime::setTimestamp expects one integer argument");
                            }
                            return writeTs(args, args[1]->get<int>());
                        },
                        Symbols::Variables::Type::CLASS, "Set the Unix timestamp; returns the instance");

        // Simple, exact-second arithmetic (in place).
        registerAdder("addSeconds", 1);
        registerAdder("addMinutes", 60);
        registerAdder("addHours",   60 * 60);
        registerAdder("addDays",    24 * 60 * 60);

        // Calendar-aware month/year arithmetic via mktime normalisation.
        registerCalendarAdder("addMonths", true);
        registerCalendarAdder("addYears", false);

        std::vector<Symbols::FunctionParameterInfo> fmt_params = {
            { "format", Symbols::Variables::Type::STRING, "strftime format, e.g. \"%Y-%m-%d %H:%M:%S\"" }
        };
        REGISTER_METHOD("DateTime", "format", fmt_params,
                        [this](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                            if (args.size() != 2 || args[1] != Symbols::Variables::Type::STRING) {
                                throw std::runtime_error("DateTime::format expects one string argument");
                            }
                            std::time_t t = static_cast<std::time_t>(readTs(args, "format"));
                            return strftimeFmt(localTm(t), args[1]->get<std::string>());
                        },
                        Symbols::Variables::Type::STRING, "Format this datetime with strftime placeholders");

        std::vector<Symbols::FunctionParameterInfo> diff_params = {
            { "other", Symbols::Variables::Type::CLASS, "Another DateTime" }
        };
        REGISTER_METHOD("DateTime", "diff", diff_params,
                        [this](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                            if (args.size() != 2 || (args[1] != Symbols::Variables::Type::CLASS &&
                                                     args[1] != Symbols::Variables::Type::OBJECT)) {
                                throw std::runtime_error("DateTime::diff expects another DateTime instance");
                            }
                            const long a  = readTs(args, "diff");
                            const auto & m = args[1]->get<Symbols::ObjectMap>();
                            auto         it = m.find("__timestamp__");
                            if (it == m.end()) {
                                throw std::runtime_error("DateTime::diff: argument is not a DateTime");
                            }
                            return Symbols::ValuePtr(static_cast<int>(a - it->second->get<int>()));
                        },
                        Symbols::Variables::Type::INTEGER, "Seconds between this and another DateTime (this - other)");
    }

    // Register a getter that derives an int field from the broken-out local time.
    template <typename Fn>
    void registerGetter(const char * methodName, Fn field) {
        std::vector<Symbols::FunctionParameterInfo> none = {};
        REGISTER_METHOD("DateTime", methodName, none,
                        ([this, methodName, field](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                            std::time_t t = static_cast<std::time_t>(readTs(args, methodName));
                            return Symbols::ValuePtr(field(localTm(t)));
                        }),
                        Symbols::Variables::Type::INTEGER, "DateTime field");
    }

    // Register an in-place adder that shifts the timestamp by (n * unitSeconds).
    void registerAdder(const char * methodName, long unitSeconds) {
        std::vector<Symbols::FunctionParameterInfo> int_param = {
            { "amount", Symbols::Variables::Type::INTEGER, "Amount to add (may be negative)" }
        };
        REGISTER_METHOD("DateTime", methodName, int_param,
                        ([this, methodName, unitSeconds](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                            if (args.size() != 2 || args[1] != Symbols::Variables::Type::INTEGER) {
                                throw std::runtime_error(std::string("DateTime::") + methodName +
                                                         " expects one integer argument");
                            }
                            const long ts = readTs(args, methodName);
                            return writeTs(args, ts + static_cast<long>(args[1]->get<int>()) * unitSeconds);
                        }),
                        Symbols::Variables::Type::CLASS, "Add to the datetime in place; returns the instance");
    }

    // Register a calendar-aware in-place adder for months or years.
    void registerCalendarAdder(const char * methodName, bool months) {
        std::vector<Symbols::FunctionParameterInfo> int_param = {
            { "amount", Symbols::Variables::Type::INTEGER, "Amount to add (may be negative)" }
        };
        REGISTER_METHOD("DateTime", methodName, int_param,
                        ([this, methodName, months](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                            if (args.size() != 2 || args[1] != Symbols::Variables::Type::INTEGER) {
                                throw std::runtime_error(std::string("DateTime::") + methodName +
                                                         " expects one integer argument");
                            }
                            std::time_t t   = static_cast<std::time_t>(readTs(args, methodName));
                            std::tm     tm  = localTm(t);
                            const int   amt = args[1]->get<int>();
                            if (months) {
                                tm.tm_mon += amt;  // mktime normalises out-of-range months across years
                            } else {
                                tm.tm_year += amt;
                            }
                            tm.tm_isdst = -1;
                            return writeTs(args, static_cast<long>(std::mktime(&tm)));
                        }),
                        Symbols::Variables::Type::CLASS, "Add months/years in place; returns the instance");
    }
};

}  // namespace Modules

#endif  // MODULES_DATETIMEMODULE_HPP
