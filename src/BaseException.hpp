#ifndef BASE_EXCEPTION_HPP
#define BASE_EXCEPTION_HPP

#include <string>

class BaseException : public std::exception {
  public:
    BaseException(const std::string & msg, const std::string & context = "") : rawMessage_(msg), context_(context) {
        formattedMessage_ = formatMessage();
    }
    BaseException() = default;
    BaseException(const BaseException&) = default;


    const char * what() const noexcept override { return formattedMessage_.c_str(); }

    virtual std::string formatMessage() const { return "[BaseException] " + context_ + ": " + rawMessage_; }

  protected:
    std::string rawMessage_;
    std::string context_;
    std::string formattedMessage_;
};

#endif  // BASE_EXCEPTION_HPP
