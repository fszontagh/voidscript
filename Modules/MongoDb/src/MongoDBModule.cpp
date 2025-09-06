// MongoDBModule.cpp - Simplified Basic Version
#include "MongoDBModule.hpp"

#include <mongocxx/client.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/collection.hpp>
#include <bsoncxx/document/view.hpp>
#include <bsoncxx/document/value.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/types.hpp>

#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

//=============================================================================
// DatabaseConnection Implementation
//=============================================================================

DatabaseConnection::DatabaseConnection(const ConnectionConfig& config)
    : config_(config), is_connected_(false) {
}

DatabaseConnection::~DatabaseConnection() {
    cleanup();
}

DatabaseConnection::DatabaseConnection(DatabaseConnection&& other) noexcept
    : client_(std::move(other.client_)),
      database_(std::move(other.database_)),
      config_(std::move(other.config_)),
      is_connected_(other.is_connected_) {
    other.is_connected_ = false;
}

DatabaseConnection& DatabaseConnection::operator=(DatabaseConnection&& other) noexcept {
    if (this != &other) {
        cleanup();
        client_ = std::move(other.client_);
        database_ = std::move(other.database_);
        config_ = std::move(other.config_);
        is_connected_ = other.is_connected_;
        other.is_connected_ = false;
    }
    return *this;
}

void DatabaseConnection::cleanup() {
    is_connected_ = false;
}

bool DatabaseConnection::connect() {
    try {
        mongocxx::uri uri_obj(config_.uri);
        client_ = mongocxx::client(uri_obj);
        database_ = client_[config_.database];

        // Simple connection test - ping the database
        auto admin = client_["admin"];
        auto command = bsoncxx::builder::stream::document{} << "ping" << 1 << bsoncxx::builder::stream::finalize;
        auto result = admin.run_command(command.view());

        is_connected_ = true;
        return true;
    } catch (const std::exception& e) {
        is_connected_ = false;
        throw ConnectionException(std::string("Failed to connect: ") + e.what());
    }
}

void DatabaseConnection::disconnect() {
    cleanup();
}

//=============================================================================
// Document Utility Functions
//=============================================================================

bsoncxx::types::bson_value::value Document::convertToBSONValue(const Symbols::ValuePtr& value) {
    switch (value.getType()) {
        case Symbols::Variables::Type::STRING:
            return bsoncxx::types::b_string{value.get<std::string>()};
        case Symbols::Variables::Type::INTEGER:
            return bsoncxx::types::b_int64{static_cast<int64_t>(value.get<int>())};
        case Symbols::Variables::Type::DOUBLE:
            return bsoncxx::types::b_double{value.get<double>()};
        case Symbols::Variables::Type::BOOLEAN:
            return bsoncxx::types::b_bool{value.get<bool>()};
        case Symbols::Variables::Type::NULL_TYPE:
            return bsoncxx::types::b_null{};
        default:
            return bsoncxx::types::b_string{"[UNSUPPORTED_TYPE]"};
    }
}

Symbols::ValuePtr Document::convertFromBSONValue(const bsoncxx::types::bson_value::view& bson_value) {
    switch (bson_value.type()) {
        case bsoncxx::type::k_string:
            return Symbols::ValuePtr(std::string(bson_value.get_string().value));
        case bsoncxx::type::k_int32:
            return Symbols::ValuePtr(static_cast<int>(bson_value.get_int32().value));
        case bsoncxx::type::k_int64:
            return Symbols::ValuePtr(static_cast<int>(bson_value.get_int64().value));
        case bsoncxx::type::k_double:
            return Symbols::ValuePtr(bson_value.get_double().value);
        case bsoncxx::type::k_bool:
            return Symbols::ValuePtr(bson_value.get_bool().value);
        case bsoncxx::type::k_null:
            return Symbols::ValuePtr::null();
        case bsoncxx::type::k_document:
            return Document::toVoidScriptObject(bson_value.get_document().value);
        default:
            return Symbols::ValuePtr(std::string("[UNSUPPORTED_BSON_TYPE]"));
    }
}

Symbols::ValuePtr Document::toVoidScriptObject(const bsoncxx::document::view& bson_view) {
    Symbols::ObjectMap result;
    for (const auto& element : bson_view) {
        std::string key = static_cast<std::string>(element.key());
        auto value = convertFromBSONValue(element.get_value());
        result[key] = std::move(value);
    }
    return Symbols::ValuePtr(result);
}

bsoncxx::document::value Document::fromVoidScriptObject(const Symbols::ValuePtr& obj) {
    bsoncxx::builder::stream::document builder;
    if (obj.getType() == Symbols::Variables::Type::OBJECT) {
        Symbols::ObjectMap objMap = obj;
        for (const auto& [key, value] : objMap) {
            builder << key << convertToBSONValue(value);
        }
    }
    return builder << bsoncxx::builder::stream::finalize;
}

//=============================================================================
// MongoDBModule Implementation
//=============================================================================

MongoDBModule::MongoDBModule() : connection_(nullptr) {
    setModuleName("MongoDB");
    setDescription("Provides basic MongoDB database connectivity and operations using the official C++ driver");
    initializeModule();
}

MongoDBModule::~MongoDBModule() {
    cleanupConnections();
}

void MongoDBModule::registerFunctions() {
    // Register MongoDB class
    REGISTER_CLASS("MongoDB");

    // Constructor
    std::vector<Symbols::FunctionParameterInfo> no_params;
    REGISTER_METHOD("MongoDB", "__construct", no_params,
        [this](FunctionArguments& args) -> Symbols::ValuePtr {
            if (args.size() != 1) {
                throw std::runtime_error("MongoDB::__construct expects no parameters");
            }
            return args[0];
        },
        Symbols::Variables::Type::CLASS,
        "Create new MongoDB instance");

    // Connection methods
    std::vector<Symbols::FunctionParameterInfo> connect_params = {
        {"uri", Symbols::Variables::Type::STRING, "MongoDB connection URI", false},
        {"database", Symbols::Variables::Type::STRING, "Database name", false}
    };
    REGISTER_METHOD("MongoDB", "connect", connect_params, [this](FunctionArguments& args) { return connect(args); }, Symbols::Variables::Type::CLASS, "Connect to MongoDB");

    // Basic operations
    std::vector<Symbols::FunctionParameterInfo> insert_params = {
        {"collection", Symbols::Variables::Type::STRING, "Collection name", false},
        {"document", Symbols::Variables::Type::OBJECT, "Document to insert", false}
    };
    REGISTER_METHOD("MongoDB", "insertOne", insert_params, [this](FunctionArguments& args) { return insertOne(args); }, Symbols::Variables::Type::OBJECT, "Insert single document");

    std::vector<Symbols::FunctionParameterInfo> find_params = {
        {"collection", Symbols::Variables::Type::STRING, "Collection name", false},
        {"filter", Symbols::Variables::Type::OBJECT, "Query filter", true}
    };
    REGISTER_METHOD("MongoDB", "findOne", find_params, [this](FunctionArguments& args) { return findOne(args); }, Symbols::Variables::Type::OBJECT, "Find single document");
}

Symbols::ValuePtr MongoDBModule::connect(FunctionArguments& args) {
    try {
        if (args.size() < 3) {
            throw ConnectionException("connect expects (this, uri, database)");
        }

        const auto& uri_str = args[1].get<std::string>();
        const auto& db_str = args[2].get<std::string>();

        ConnectionConfig config;
        config.uri = uri_str;
        config.database = db_str;

        {
            std::lock_guard<std::mutex> lock(module_mutex_);
            connection_ = std::make_unique<DatabaseConnection>(config);
            connection_->connect();
        }

        // Store connection info in object
        Symbols::ObjectMap objMap = args[0];
        objMap["__connected__"] = Symbols::ValuePtr(true);
        objMap["__database__"] = Symbols::ValuePtr(db_str);

        return Symbols::ValuePtr(objMap);

    } catch (const MongoDBException& e) {
        throw;
    } catch (const std::exception& e) {
        throw ConnectionException(std::string("Connection failed: ") + e.what());
    }
}

Symbols::ValuePtr MongoDBModule::findOne(FunctionArguments& args) {
    try {
        if (args.size() < 2) {
            throw QueryException("findOne expects (this, collectionName, [filter])");
        }

        std::lock_guard<std::mutex> lock(module_mutex_);
        if (!isConnected()) {
            throw ConnectionException("No active database connection");
        }

        const std::string& collection_name = args[1].get<std::string>();
        auto collection = connection_->getDatabase()[collection_name];

        // Parse filter (optional)
        bsoncxx::document::value filter_doc = bsoncxx::builder::stream::document{} << bsoncxx::builder::stream::finalize;
        if (args.size() > 2 && args[2].getType() == Symbols::Variables::Type::OBJECT) {
            Symbols::ObjectMap filter_map = args[2].get<Symbols::ObjectMap>();
            filter_doc = convertToBSONDocument(filter_map);
        }

        auto result = collection.find_one(filter_doc.view());

        if (result) {
            return convertFromBSONDocument(*result);
        } else {
            return Symbols::ValuePtr::null();
        }

    } catch (const MongoDBException& e) {
        throw;
    } catch (const std::exception& e) {
        throw QueryException(std::string("Find one operation failed: ") + e.what());
    }
}

Symbols::ValuePtr MongoDBModule::insertOne(FunctionArguments& args) {
    try {
        if (args.size() < 3) {
            throw QueryException("insertOne expects (this, collectionName, document)");
        }

        std::lock_guard<std::mutex> lock(module_mutex_);
        if (!isConnected()) {
            throw ConnectionException("No active database connection");
        }

        const std::string& collection_name = args[1].get<std::string>();
        auto collection = connection_->getDatabase()[collection_name];

        if (args[2].getType() != Symbols::Variables::Type::OBJECT) {
            throw QueryException("Document must be an object");
        }

        Symbols::ObjectMap document_map = args[2].get<Symbols::ObjectMap>();
        bsoncxx::document::value doc = convertToBSONDocument(document_map);

        auto result = collection.insert_one(doc.view());

        if (!result) {
            throw QueryException("Insert one operation failed");
        }

        Symbols::ObjectMap response;
        response["insertedId"] = Symbols::ValuePtr("oid_placeholder");
        response["acknowledged"] = Symbols::ValuePtr(true); // Assume acknowledged

        return Symbols::ValuePtr(response);

    } catch (const MongoDBException& e) {
        throw;
    } catch (const std::exception& e) {
        throw QueryException(std::string("Insert one operation failed: ") + e.what());
    }
}

//=============================================================================
// Private helper methods
//=============================================================================

void MongoDBModule::initializeModule() {
}

void MongoDBModule::cleanupConnections() {
    std::lock_guard<std::mutex> lock(module_mutex_);
    connection_.reset();
}

bool MongoDBModule::isConnected() const {
    return connection_ && connection_->isConnected();
}

bsoncxx::document::value MongoDBModule::convertToBSONDocument(const std::map<std::string, Symbols::ValuePtr>& document) {
    bsoncxx::builder::stream::document builder;
    for (const auto& [key, value] : document) {
        builder << key << Document::convertToBSONValue(value);
    }
    return builder << bsoncxx::builder::stream::finalize;
}

Symbols::ValuePtr MongoDBModule::convertFromBSONDocument(const bsoncxx::document::view& view) {
    return Document::toVoidScriptObject(view);
}

} // namespace Modules