#ifndef NAMESPACE_MANAGER_H
#define NAMESPACE_MANAGER_H
#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

class Namespace {
  public:
    Namespace(const std::string & name = "", Namespace * parent = nullptr) : name_(name), parent_(parent) {}

    /**
    * @brief Add a child namespace to the current namespace.
    * @param name Name of the child namespace.
    */
    Namespace * addChild(const std::string & name) {
        auto it = children_.find(name);
        if (it == children_.end()) {
            children_[name] = std::make_unique<Namespace>(name, this);
        }
        return children_[name].get();
    }

    /**
    * @brief Get the child namespace by name.
    * @param name Name of the child namespace.
    * @return The child namespace
    */
    Namespace * getChild(const std::string & name) const {
        auto it = children_.find(name);
        return it != children_.end() ? it->second.get() : nullptr;
    }

    /**
    * @brief Get the child namespace by name.
    * @param name Name of the child namespace.
    * @return The child namespace
    */
    Namespace * getOrCreate(const std::string & fullName) {
        auto        parts   = split(fullName, '.');
        Namespace * current = this;
        for (const auto & part : parts) {
            current = current->addChild(part);
        }
        return current;
    }

    /**
    * @brief Get the parent namespace.
    * @return The parent namespace.
    */
    Namespace * getParent() const { return parent_; }

    std::string toString() const {
        if (!parent_ || name_.empty()) {
            return name_;
        }
        return parent_->toString() + "." + name_;
    }

    void traverse(const std::function<void(const Namespace &)>& visitor) const {
        visitor(*this);
        for (const auto & [_, child] : children_) {
            child->traverse(visitor);
        }
    }

    const std::string & getName() const { return name_; }

  private:
    std::string                                       name_;
    Namespace *                                       parent_;
    std::map<std::string, std::unique_ptr<Namespace>> children_;

    static std::vector<std::string> split(const std::string & str, char delimiter) {
        std::stringstream        ss(str);
        std::string              part;
        std::vector<std::string> result;
        while (std::getline(ss, part, delimiter)) {
            result.push_back(part);
        }
        return result;
    }
};

class NamespaceManager {
  public:
    static NamespaceManager & instance() {
        static NamespaceManager instance_;
        return instance_;
    }

    /**
     * @brief Constructor for NamespaceManager.
     * @param name Name of the namespace.
     * @param parent Parent namespace.
     */
    NamespaceManager() : root_(""), currentNamespace_(&root_) {}

    /**
    * @brief Get or create a namespace by full name.
    * @param fullName Full name of the namespace.
    * @return The namespace object.
    */
    Namespace * getOrCreate(const std::string & fullName) { return root_.getOrCreate(fullName); }

    /**
    * @brief Set the current namespace.
    * @param fullName Full name of the namespace.
    */
    void setCurrent(const std::string & fullName) {
        Namespace * ns = root_.getOrCreate(fullName);
        if (ns) {
            currentNamespace_ = ns;
        } else {
            currentNamespace_ = &root_;  // fallback
        }
    }

    /**
    * @brief Get the current namespace.
    * @return The current namespace object.
    */
    Namespace * getCurrent() const { return currentNamespace_; }

    /**
    * @brief Reset the current namespace.
    */
    void resetCurrent() { currentNamespace_ = &root_; }

    /**
    * @brief Traverse the namespace tree.
    * @param visitor A function to visit each namespace.
    */
    void traverse(const std::function<void(const Namespace &)>& visitor) const { root_.traverse(visitor); }

  private:
    Namespace   root_;
    Namespace * currentNamespace_;
};

#endif  // NAMESPACE_MANAGER_H
