#ifndef INTERPRETER_OPERATION_CONTAINER_HPP
#define INTERPRETER_OPERATION_CONTAINER_HPP

#include <map>
#include <string>
#include <vector>

#include "Interpreter/Operation.hpp"

namespace Operations {

class Container {

  public:
    /**
     * @brief Get the Operations::Container instance.
     * @return The Operations::Container instance.
     */
    static Operations::Container * instance() {
        static Operations::Container instance_;
        return &instance_;
    }

    Container() = default;

    void add(const std::string & ns, Operations::Operation operation) {
        this->_operations[ns].emplace_back(std::make_shared<Operations::Operation>(std::move(operation)));
    }

    /**
    * @brief Returns the first operation in the namespace.
    * @param ns Namespace from which to get the operation.
    * @return The first operation in the namespace.
    */
    std::shared_ptr<Operations::Operation> getFirst(const std::string & ns) {
        auto it = _operations.find(ns);
        if (it != _operations.end()) {
            return it->second.front();
        }
        return nullptr;
    }

    /**
     * @brief Removes the first operation from the namespace.
     * @param ns Namespace from which to remove the operation.
     * @return The removed operation.
     */
    std::shared_ptr<Operations::Operation> pullFirst(const std::string & ns) {
        auto it = _operations.find(ns);
        if (it != _operations.end()) {
            auto operation = it->second.front();
            it->second.erase(it->second.begin());
            return operation;
        }
        return nullptr;
    }

    /**
     * @brief Removes the last operation from the namespace.
     * @param ns Namespace from which to remove the operation.
     * @return The removed operation.
     */
    std::shared_ptr<Operations::Operation> pullLast(const std::string & ns) {
        auto it = _operations.find(ns);
        if (it != _operations.end()) {
            auto operation = it->second.back();
            it->second.pop_back();
            return operation;
        }
        return nullptr;
    }

    /**
    * @brief Returns the last operation in the namespace.
    * @param ns Namespace from which to get the operation.
    * @return The last operation in the namespace.
    */
    std::shared_ptr<Operations::Operation> getLast(const std::string & ns) {
        auto it = _operations.find(ns);
        if (it != _operations.end()) {
            return it->second.back();
        }
        return nullptr;
    }

    /**
     * @brief Returns all operations in the namespace.
     * @param ns Namespace from which to get the operations.
     * @return All operations in the namespace.
     */
    std::vector<std::shared_ptr<Operations::Operation>> getAll(const std::string & ns) {
        auto it = _operations.find(ns);
        if (it != _operations.end()) {
            return it->second;
        }
        return {};
    }

    /**
     * @brief Returns all operations from all namespaces
     * @return All operations in the namespace.
     */
    std::vector<std::shared_ptr<Operations::Operation>> getAll() {
        std::vector<std::shared_ptr<Operations::Operation>> result;
        for (const auto & [_, table] : _operations) {
            result.insert(result.end(), table.begin(), table.end());
        }
        return result;
    }

    auto begin() { return _operations.begin(); }

    auto end() { return _operations.end(); }

    auto begin() const { return _operations.begin(); }

    auto end() const { return _operations.end(); }

    static std::string dump()  {
        std::string result;
        for (const auto & [_, table] : Operations::Container::instance()->_operations) {
            result += "Namespace: " + _ + "\n";
            for (const auto & operation : table) {
                result += "  Operation: " + operation->toString() + "\n";
            }
        }
        return result;
    }

  private:
    std::map<std::string, std::vector<std::shared_ptr<Operations::Operation>>> _operations;
};  // class Container
};  // namespace Operations

#endif  // INTERPRETER_OPERATION_CONTAINER_HPP
