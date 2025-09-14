#ifndef MODULES_DATETIMEMODULE_HPP
#define MODULES_DATETIMEMODULE_HPP

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "Modules/BaseModule.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"
#include "Symbols/RegistrationMacros.hpp"

namespace Modules {

/**
 * @brief DateTime module providing date/time functionality for VoidScript
 *
 * Provides built-in functions:
 * - current_unix_timestamp() -> returns current Unix timestamp as integer
 * - date() -> returns formatted date/time string in current timezone
 *
 * And DateTime class with methods:
 * - Constructor: new DateTime() (creates object with current datetime)
 * - day(), month(), year(), hour(), minute(), second() -> return integers
 * - addDays(int), addMonths(int), addYears(int), addHours(int), addMinutes(int), addSeconds(int)
 * - format(string) -> format datetime using C-style format placeholders
 */
class DateTimeModule : public BaseModule {
  private:
    // External state management like Imagick module
    static std::unordered_map<std::string, int> object_to_timestamp_map_;
  public:
    DateTimeModule() {
        setModuleName("DateTime");
        setDescription("Provides comprehensive date and time functionality including current timestamp retrieval, date formatting, and a DateTime class with arithmetic operations and custom formatting support");
        setBuiltIn(true);
    }

    void registerFunctions() override {
        // Register built-in functions
        registerBuiltInFunctions();
        
        // Register DateTime class and its methods
        registerDateTimeClass();
    }

  private:
    void registerBuiltInFunctions() {
        // current_unix_timestamp() function
        std::vector<Symbols::FunctionParameterInfo> params = {};
        REGISTER_FUNCTION("current_unix_timestamp", Symbols::Variables::Type::INTEGER, params,
                          "Get current Unix timestamp as integer",
                          [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                              if (!args.empty()) {
                                  throw std::runtime_error(name() + "::current_unix_timestamp expects no arguments");
                              }
                              auto now = std::chrono::system_clock::now();
                              auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                                  now.time_since_epoch()).count();
                              return static_cast<int>(timestamp);
                          });

        // date() function
        REGISTER_FUNCTION("date", Symbols::Variables::Type::STRING, params,
                          "Get current date/time as formatted string",
                          [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                              if (!args.empty()) {
                                  throw std::runtime_error(name() + "::date expects no arguments");
                              }
                              auto now = std::chrono::system_clock::now();
                              auto time_t = std::chrono::system_clock::to_time_t(now);
                              auto tm = *std::localtime(&time_t);
                              
                              std::ostringstream oss;
                              oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
                              return oss.str();
                          });
    }

    void registerDateTimeClass() {
        // Register DateTime class
        REGISTER_CLASS("DateTime");

        // Note: We don't register __timestamp__ as a property to avoid default initialization
        // It will be managed manually in the constructor and methods

        // Constructor - new DateTime()
        std::vector<Symbols::FunctionParameterInfo> constructor_params = {};
        REGISTER_METHOD("DateTime", "__construct", constructor_params,
                        [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                            if (args.size() != 1) {
                                throw std::runtime_error("DateTime::__construct expects no parameters, got: " + std::to_string(args.size() - 1));
                            }
                            
                            if (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT) {
                                throw std::runtime_error("DateTime::__construct must be called on DateTime instance");
                            }
                            
                            // Get the current timestamp
                            auto now = std::chrono::system_clock::now();
                            auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                                now.time_since_epoch()).count();
                            
                            // Store timestamp using object identity (like Imagick module)
                            std::string objectId = args[0].toString();
                            object_to_timestamp_map_[objectId] = static_cast<int>(timestamp);
                            
                            // Return the original object
                            return args[0];
                        },
                        Symbols::Variables::Type::CLASS,
                        "Create new DateTime object with current date/time");

        // day() method
        std::vector<Symbols::FunctionParameterInfo> no_params = {};
        REGISTER_METHOD("DateTime", "day", no_params,
                        [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                            auto objMap = this->getObjectMap(args, "day");
                            auto it = objMap.find("__timestamp__");
                            if (it == objMap.end()) {
                                throw std::runtime_error("DateTime object missing __timestamp__ property");
                            }
                            int timestamp = it->second->get<int>();
                            
                            auto time_t = static_cast<std::time_t>(timestamp);
                            auto tm = *std::localtime(&time_t);
                            return tm.tm_mday;
                        },
                        Symbols::Variables::Type::INTEGER,
                        "Get day of month (1-31)");

        // month() method
        REGISTER_METHOD("DateTime", "month", no_params,
                        [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                            auto objMap = this->getObjectMap(args, "month");
                            auto it = objMap.find("__timestamp__");
                            if (it == objMap.end()) {
                                throw std::runtime_error("DateTime object missing __timestamp__ property");
                            }
                            int timestamp = it->second->get<int>();
                            
                            auto time_t = static_cast<std::time_t>(timestamp);
                            auto tm = *std::localtime(&time_t);
                            return tm.tm_mon + 1; // tm_mon is 0-based
                        },
                        Symbols::Variables::Type::INTEGER,
                        "Get month (1-12)");

        // year() method
        REGISTER_METHOD("DateTime", "year", no_params,
                        [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                            // Use external state management like Imagick
                            std::string objectId = args[0].toString();
                            auto it = object_to_timestamp_map_.find(objectId);
                            if (it == object_to_timestamp_map_.end()) {
                                throw std::runtime_error("DateTime::year: object not properly initialized");
                            }
                            int timestamp = it->second;
                            
                            auto time_t = static_cast<std::time_t>(timestamp);
                            auto tm = *std::localtime(&time_t);
                            return tm.tm_year + 1900; // tm_year is years since 1900
                        },
                        Symbols::Variables::Type::INTEGER,
                        "Get year (e.g., 2024)");

        // hour() method
        REGISTER_METHOD("DateTime", "hour", no_params,
                        [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                            auto objMap = this->getObjectMap(args, "hour");
                            auto it = objMap.find("__timestamp__");
                            if (it == objMap.end()) {
                                throw std::runtime_error("DateTime object missing __timestamp__ property");
                            }
                            int timestamp = it->second->get<int>();
                            
                            auto time_t = static_cast<std::time_t>(timestamp);
                            auto tm = *std::localtime(&time_t);
                            return tm.tm_hour;
                        },
                        Symbols::Variables::Type::INTEGER,
                        "Get hour (0-23)");

        // minute() method
        REGISTER_METHOD("DateTime", "minute", no_params,
                        [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                            auto objMap = this->getObjectMap(args, "minute");
                            auto it = objMap.find("__timestamp__");
                            if (it == objMap.end()) {
                                throw std::runtime_error("DateTime object missing __timestamp__ property");
                            }
                            int timestamp = it->second->get<int>();
                            
                            auto time_t = static_cast<std::time_t>(timestamp);
                            auto tm = *std::localtime(&time_t);
                            return tm.tm_min;
                        },
                        Symbols::Variables::Type::INTEGER,
                        "Get minute (0-59)");

        // second() method
        REGISTER_METHOD("DateTime", "second", no_params,
                        [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                            auto objMap = this->getObjectMap(args, "second");
                            auto it = objMap.find("__timestamp__");
                            if (it == objMap.end()) {
                                throw std::runtime_error("DateTime object missing __timestamp__ property");
                            }
                            int timestamp = it->second->get<int>();
                            
                            auto time_t = static_cast<std::time_t>(timestamp);
                            auto tm = *std::localtime(&time_t);
                            return tm.tm_sec;
                        },
                        Symbols::Variables::Type::INTEGER,
                        "Get second (0-59)");

        // addDays(int) method
        std::vector<Symbols::FunctionParameterInfo> int_param = {
            { "days", Symbols::Variables::Type::INTEGER, "Number of days to add", false, false }
        };
        REGISTER_METHOD("DateTime", "addDays", int_param,
                        [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                            if (args.size() != 2 || args[1] != Symbols::Variables::Type::INTEGER) {
                                throw std::runtime_error(name() + "::DateTime::addDays expects one integer argument");
                            }
                            
                            auto objMap = this->getObjectMap(args, "addDays");
                            auto it = objMap.find("__timestamp__");
                            if (it == objMap.end()) {
                                throw std::runtime_error("DateTime object missing __timestamp__ property");
                            }
                            int timestamp = it->second->get<int>();
                            int days = args[1]->get<int>();
                            
                            int new_timestamp = timestamp + (days * 24 * 60 * 60);
                            
                            Symbols::ObjectMap objectMap;
                            objectMap["__class__"] = Symbols::ValuePtr("DateTime");
                            objectMap["__timestamp__"] = Symbols::ValuePtr(new_timestamp);
                            
                            return Symbols::ValuePtr::makeClassInstance(objectMap);
                        },
                        Symbols::Variables::Type::CLASS,
                        "Add specified number of days and return new DateTime object");

        // addMonths(int) method
        REGISTER_METHOD("DateTime", "addMonths", int_param,
                        [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                            if (args.size() != 2 || args[1] != Symbols::Variables::Type::INTEGER) {
                                throw std::runtime_error(name() + "::DateTime::addMonths expects one integer argument");
                            }
                            
                            auto objMap = this->getObjectMap(args, "addMonths");
                            auto it = objMap.find("__timestamp__");
                            if (it == objMap.end()) {
                                throw std::runtime_error("DateTime object missing __timestamp__ property");
                            }
                            int timestamp = it->second->get<int>();
                            int months = args[1]->get<int>();
                            
                            auto time_t = static_cast<std::time_t>(timestamp);
                            auto tm = *std::localtime(&time_t);
                            
                            tm.tm_mon += months;
                            // Normalize month overflow/underflow
                            while (tm.tm_mon >= 12) {
                                tm.tm_mon -= 12;
                                tm.tm_year++;
                            }
                            while (tm.tm_mon < 0) {
                                tm.tm_mon += 12;
                                tm.tm_year--;
                            }
                            
                            auto new_time_t = std::mktime(&tm);
                            int new_timestamp = static_cast<int>(new_time_t);
                            
                            Symbols::ObjectMap objectMap;
                            objectMap["__class__"] = Symbols::ValuePtr("DateTime");
                            objectMap["__timestamp__"] = Symbols::ValuePtr(new_timestamp);
                            
                            return Symbols::ValuePtr::makeClassInstance(objectMap);
                        },
                        Symbols::Variables::Type::CLASS,
                        "Add specified number of months and return new DateTime object");

        // addYears(int) method
        REGISTER_METHOD("DateTime", "addYears", int_param,
                        [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                            if (args.size() != 2 || args[1] != Symbols::Variables::Type::INTEGER) {
                                throw std::runtime_error(name() + "::DateTime::addYears expects one integer argument");
                            }
                            
                            auto objMap = this->getObjectMap(args, "addYears");
                            auto it = objMap.find("__timestamp__");
                            if (it == objMap.end()) {
                                throw std::runtime_error("DateTime object missing __timestamp__ property");
                            }
                            int timestamp = it->second->get<int>();
                            int years = args[1]->get<int>();
                            
                            auto time_t = static_cast<std::time_t>(timestamp);
                            auto tm = *std::localtime(&time_t);
                            tm.tm_year += years;
                            
                            auto new_time_t = std::mktime(&tm);
                            int new_timestamp = static_cast<int>(new_time_t);
                            
                            Symbols::ObjectMap objectMap;
                            objectMap["__class__"] = Symbols::ValuePtr("DateTime");
                            objectMap["__timestamp__"] = Symbols::ValuePtr(new_timestamp);
                            
                            return Symbols::ValuePtr::makeClassInstance(objectMap);
                        },
                        Symbols::Variables::Type::CLASS,
                        "Add specified number of years and return new DateTime object");

        // addHours(int) method
        REGISTER_METHOD("DateTime", "addHours", int_param,
                        [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                            if (args.size() != 2 || args[1] != Symbols::Variables::Type::INTEGER) {
                                throw std::runtime_error(name() + "::DateTime::addHours expects one integer argument");
                            }
                            
                            auto objMap = this->getObjectMap(args, "addHours");
                            auto it = objMap.find("__timestamp__");
                            if (it == objMap.end()) {
                                throw std::runtime_error("DateTime object missing __timestamp__ property");
                            }
                            int timestamp = it->second->get<int>();
                            int hours = args[1]->get<int>();
                            
                            int new_timestamp = timestamp + (hours * 60 * 60);
                            
                            Symbols::ObjectMap objectMap;
                            objectMap["__class__"] = Symbols::ValuePtr("DateTime");
                            objectMap["__timestamp__"] = Symbols::ValuePtr(new_timestamp);
                            
                            return Symbols::ValuePtr::makeClassInstance(objectMap);
                        },
                        Symbols::Variables::Type::CLASS,
                        "Add specified number of hours and return new DateTime object");

        // addMinutes(int) method
        REGISTER_METHOD("DateTime", "addMinutes", int_param,
                        [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                            if (args.size() != 2 || args[1] != Symbols::Variables::Type::INTEGER) {
                                throw std::runtime_error(name() + "::DateTime::addMinutes expects one integer argument");
                            }
                            
                            auto objMap = this->getObjectMap(args, "addMinutes");
                            auto it = objMap.find("__timestamp__");
                            if (it == objMap.end()) {
                                throw std::runtime_error("DateTime object missing __timestamp__ property");
                            }
                            int timestamp = it->second->get<int>();
                            int minutes = args[1]->get<int>();
                            
                            int new_timestamp = timestamp + (minutes * 60);
                            
                            Symbols::ObjectMap objectMap;
                            objectMap["__class__"] = Symbols::ValuePtr("DateTime");
                            objectMap["__timestamp__"] = Symbols::ValuePtr(new_timestamp);
                            
                            return Symbols::ValuePtr::makeClassInstance(objectMap);
                        },
                        Symbols::Variables::Type::CLASS,
                        "Add specified number of minutes and return new DateTime object");

        // addSeconds(int) method
        REGISTER_METHOD("DateTime", "addSeconds", int_param,
                        [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                            if (args.size() != 2 || args[1] != Symbols::Variables::Type::INTEGER) {
                                throw std::runtime_error(name() + "::DateTime::addSeconds expects one integer argument");
                            }
                            
                            auto objMap = this->getObjectMap(args, "addSeconds");
                            auto it = objMap.find("__timestamp__");
                            if (it == objMap.end()) {
                                throw std::runtime_error("DateTime object missing __timestamp__ property");
                            }
                            int timestamp = it->second->get<int>();
                            int seconds = args[1]->get<int>();
                            
                            int new_timestamp = timestamp + seconds;
                            
                            Symbols::ObjectMap objectMap;
                            objectMap["__class__"] = Symbols::ValuePtr("DateTime");
                            objectMap["__timestamp__"] = Symbols::ValuePtr(new_timestamp);
                            
                            return Symbols::ValuePtr::makeClassInstance(objectMap);
                        },
                        Symbols::Variables::Type::CLASS,
                        "Add specified number of seconds and return new DateTime object");

        // format(string) method
        std::vector<Symbols::FunctionParameterInfo> format_param = {
            { "format", Symbols::Variables::Type::STRING, "Format string (Y-m-d H:i:s style)", false, false }
        };
        REGISTER_METHOD("DateTime", "format", format_param,
                        [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                            if (args.size() != 2 || args[1] != Symbols::Variables::Type::STRING) {
                                throw std::runtime_error(name() + "::DateTime::format expects one string argument");
                            }
                            
                            auto objMap = this->getObjectMap(args, "format");
                            auto it = objMap.find("__timestamp__");
                            if (it == objMap.end()) {
                                throw std::runtime_error("DateTime object missing __timestamp__ property");
                            }
                            int timestamp = it->second->get<int>();
                            std::string format_str = args[1]->get<std::string>();
                            
                            auto time_t = static_cast<std::time_t>(timestamp);
                            auto tm = *std::localtime(&time_t);
                            
                            return formatDateTime(tm, format_str);
                        },
                        Symbols::Variables::Type::STRING,
                        "Format datetime using C-style format placeholders (Y-m-d H:i:s style)");
    }

    /**
     * @brief Format datetime using C-style format placeholders
     * Supports: Y (4-digit year), m (2-digit month), d (2-digit day),
     *          H (2-digit hour), i (2-digit minute), s (2-digit second)
     */
    std::string formatDateTime(const std::tm& tm, const std::string& format) {
        std::string result = format;
        
        // Year (4 digits)
        if (result.find('Y') != std::string::npos) {
            std::string year = std::to_string(tm.tm_year + 1900);
            size_t pos = 0;
            while ((pos = result.find('Y', pos)) != std::string::npos) {
                result.replace(pos, 1, year);
                pos += year.length();
            }
        }
        
        // Month (2 digits)
        if (result.find('m') != std::string::npos) {
            std::ostringstream oss;
            oss << std::setfill('0') << std::setw(2) << (tm.tm_mon + 1);
            std::string month = oss.str();
            size_t pos = 0;
            while ((pos = result.find('m', pos)) != std::string::npos) {
                result.replace(pos, 1, month);
                pos += month.length();
            }
        }
        
        // Day (2 digits)
        if (result.find('d') != std::string::npos) {
            std::ostringstream oss;
            oss << std::setfill('0') << std::setw(2) << tm.tm_mday;
            std::string day = oss.str();
            size_t pos = 0;
            while ((pos = result.find('d', pos)) != std::string::npos) {
                result.replace(pos, 1, day);
                pos += day.length();
            }
        }
        
        // Hour (2 digits)
        if (result.find('H') != std::string::npos) {
            std::ostringstream oss;
            oss << std::setfill('0') << std::setw(2) << tm.tm_hour;
            std::string hour = oss.str();
            size_t pos = 0;
            while ((pos = result.find('H', pos)) != std::string::npos) {
                result.replace(pos, 1, hour);
                pos += hour.length();
            }
        }
        
        // Minute (2 digits) - using 'i' like PHP
        if (result.find('i') != std::string::npos) {
            std::ostringstream oss;
            oss << std::setfill('0') << std::setw(2) << tm.tm_min;
            std::string minute = oss.str();
            size_t pos = 0;
            while ((pos = result.find('i', pos)) != std::string::npos) {
                result.replace(pos, 1, minute);
                pos += minute.length();
            }
        }
        
        // Second (2 digits)
        if (result.find('s') != std::string::npos) {
            std::ostringstream oss;
            oss << std::setfill('0') << std::setw(2) << tm.tm_sec;
            std::string second = oss.str();
            size_t pos = 0;
            while ((pos = result.find('s', pos)) != std::string::npos) {
                result.replace(pos, 1, second);
                pos += second.length();
            }
        }
        
        return result;
    }
};

} // namespace Modules

// Define the static member
std::unordered_map<std::string, int> Modules::DateTimeModule::object_to_timestamp_map_;

#endif // MODULES_DATETIMEMODULE_HPP