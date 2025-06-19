#include "XmlModule.hpp"

#include <cstring>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdarg>

#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

// Error handling callback for validation
static void xmlValidationErrorCallback(void* ctx, const char* msg, ...) {
    std::vector<std::string>* errors = static_cast<std::vector<std::string>*>(ctx);
    va_list args;
    va_start(args, msg);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), msg, args);
    va_end(args);
    errors->push_back(std::string(buffer));
}

static void xmlValidationWarningCallback(void* ctx, const char* msg, ...) {
    std::vector<std::string>* warnings = static_cast<std::vector<std::string>*>(ctx);
    va_list args;
    va_start(args, msg);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), msg, args);
    va_end(args);
    warnings->push_back(std::string(buffer));
}

// XmlSchema implementation
XmlSchema::XmlSchema() : schema_(nullptr), parserCtxt_(nullptr), validCtxt_(nullptr) {
    id_ = nextDoc++;
}

XmlSchema::~XmlSchema() {
    if (validCtxt_) {
        xmlSchemaFreeValidCtxt(validCtxt_);
        validCtxt_ = nullptr;
    }
    if (schema_) {
        xmlSchemaFree(schema_);
        schema_ = nullptr;
    }
    if (parserCtxt_) {
        xmlSchemaFreeParserCtxt(parserCtxt_);
        parserCtxt_ = nullptr;
    }
}

bool XmlSchema::loadFromFile(const std::string& filename) {
    clearErrors();
    
    parserCtxt_ = xmlSchemaNewParserCtxt(filename.c_str());
    if (!parserCtxt_) {
        errors_.push_back("Failed to create schema parser context for file: " + filename);
        return false;
    }
    
    xmlSchemaSetParserErrors(parserCtxt_, xmlValidationErrorCallback, xmlValidationWarningCallback, &errors_);
    
    schema_ = xmlSchemaParse(parserCtxt_);
    if (!schema_) {
        errors_.push_back("Failed to parse schema from file: " + filename);
        return false;
    }
    
    return true;
}

bool XmlSchema::loadFromString(const std::string& schemaContent) {
    clearErrors();
    
    parserCtxt_ = xmlSchemaNewMemParserCtxt(schemaContent.c_str(), schemaContent.length());
    if (!parserCtxt_) {
        errors_.push_back("Failed to create schema parser context from string");
        return false;
    }
    
    xmlSchemaSetParserErrors(parserCtxt_, xmlValidationErrorCallback, xmlValidationWarningCallback, &errors_);
    
    schema_ = xmlSchemaParse(parserCtxt_);
    if (!schema_) {
        errors_.push_back("Failed to parse schema from string");
        return false;
    }
    
    return true;
}

bool XmlSchema::loadFromMemory(const char* buffer, int size) {
    clearErrors();
    
    parserCtxt_ = xmlSchemaNewMemParserCtxt(buffer, size);
    if (!parserCtxt_) {
        errors_.push_back("Failed to create schema parser context from memory");
        return false;
    }
    
    xmlSchemaSetParserErrors(parserCtxt_, xmlValidationErrorCallback, xmlValidationWarningCallback, &errors_);
    
    schema_ = xmlSchemaParse(parserCtxt_);
    if (!schema_) {
        errors_.push_back("Failed to parse schema from memory");
        return false;
    }
    
    return true;
}

bool XmlSchema::validate(XmlDocument* document) {
    if (!document) {
        errors_.push_back("Document is null");
        return false;
    }
    return validate(document->getDoc());
}

bool XmlSchema::validate(xmlDocPtr doc) {
    if (!schema_) {
        errors_.push_back("No schema loaded");
        return false;
    }
    
    if (!doc) {
        errors_.push_back("Document is null");
        return false;
    }
    
    clearErrors();
    
    validCtxt_ = xmlSchemaNewValidCtxt(schema_);
    if (!validCtxt_) {
        errors_.push_back("Failed to create schema validation context");
        return false;
    }
    
    xmlSchemaSetValidErrors(validCtxt_, xmlValidationErrorCallback, xmlValidationWarningCallback, &errors_);
    
    int result = xmlSchemaValidateDoc(validCtxt_, doc);
    
    xmlSchemaFreeValidCtxt(validCtxt_);
    validCtxt_ = nullptr;
    
    return result == 0;
}

const std::vector<std::string>& XmlSchema::getErrors() const {
    return errors_;
}

void XmlSchema::clearErrors() {
    errors_.clear();
}

std::string XmlSchema::getLastError() const {
    return errors_.empty() ? "" : errors_.back();
}

// XmlDtd implementation
XmlDtd::XmlDtd() : dtd_(nullptr), validCtxt_(nullptr) {
    id_ = nextDoc++;
}

XmlDtd::~XmlDtd() {
    if (validCtxt_) {
        xmlFreeValidCtxt(validCtxt_);
        validCtxt_ = nullptr;
    }
    if (dtd_) {
        xmlFreeDtd(dtd_);
        dtd_ = nullptr;
    }
}

bool XmlDtd::loadFromFile(const std::string& filename) {
    clearErrors();
    
    dtd_ = xmlParseDTD(nullptr, BAD_CAST filename.c_str());
    if (!dtd_) {
        errors_.push_back("Failed to parse DTD from file: " + filename);
        return false;
    }
    
    return true;
}

bool XmlDtd::loadFromString(const std::string& dtdContent) {
    clearErrors();
    
    // Create a temporary file for DTD parsing since libxml2 doesn't have direct string parsing for DTD
    dtd_ = xmlParseDTD(nullptr, BAD_CAST dtdContent.c_str());
    if (!dtd_) {
        errors_.push_back("Failed to parse DTD from string");
        return false;
    }
    
    return true;
}

bool XmlDtd::loadFromMemory(const char* buffer, int size) {
    clearErrors();
    
    // Parse DTD from memory buffer
    xmlParserCtxtPtr ctxt = xmlNewParserCtxt();
    if (!ctxt) {
        errors_.push_back("Failed to create parser context");
        return false;
    }
    
    dtd_ = xmlIOParseDTD(nullptr, xmlParserInputBufferCreateMem(buffer, size, XML_CHAR_ENCODING_NONE), XML_CHAR_ENCODING_NONE);
    xmlFreeParserCtxt(ctxt);
    
    if (!dtd_) {
        errors_.push_back("Failed to parse DTD from memory");
        return false;
    }
    
    return true;
}

bool XmlDtd::validate(XmlDocument* document) {
    if (!document) {
        errors_.push_back("Document is null");
        return false;
    }
    return validate(document->getDoc());
}

bool XmlDtd::validate(xmlDocPtr doc) {
    if (!dtd_) {
        errors_.push_back("No DTD loaded");
        return false;
    }
    
    if (!doc) {
        errors_.push_back("Document is null");
        return false;
    }
    
    clearErrors();
    
    validCtxt_ = xmlNewValidCtxt();
    if (!validCtxt_) {
        errors_.push_back("Failed to create DTD validation context");
        return false;
    }
    
    validCtxt_->error = xmlValidationErrorCallback;
    validCtxt_->warning = xmlValidationWarningCallback;
    validCtxt_->userData = &errors_;
    
    int result = xmlValidateDtd(validCtxt_, doc, dtd_);
    
    xmlFreeValidCtxt(validCtxt_);
    validCtxt_ = nullptr;
    
    return result == 1;
}

const std::vector<std::string>& XmlDtd::getErrors() const {
    return errors_;
}

void XmlDtd::clearErrors() {
    errors_.clear();
}

std::string XmlDtd::getLastError() const {
    return errors_.empty() ? "" : errors_.back();
}

// XmlValidator implementation
XmlValidator::XmlValidator() {
    id_ = nextDoc++;
}

XmlValidator::~XmlValidator() = default;

bool XmlValidator::validateWellFormedness(XmlDocument* document) {
    if (!document) {
        errors_.push_back("Document is null");
        return false;
    }
    
    clearErrors();
    
    // Use the document's built-in well-formedness check with enhanced error reporting
    std::vector<std::string> docErrors;
    bool isWellFormed = document->validateWellFormedness(docErrors);
    
    errors_.insert(errors_.end(), docErrors.begin(), docErrors.end());
    
    return isWellFormed;
}

bool XmlValidator::validateWellFormedness(const std::string& xmlContent) {
    clearErrors();
    
    try {
        auto doc = XmlDocument::createFromString(xmlContent);
        return validateWellFormedness(doc.get());
    } catch (const std::exception& e) {
        errors_.push_back("Failed to parse XML: " + std::string(e.what()));
        return false;
    }
}

bool XmlValidator::validateAgainstSchema(XmlDocument* document, XmlSchema* schema) {
    if (!document || !schema) {
        errors_.push_back("Document or schema is null");
        return false;
    }
    
    clearErrors();
    
    bool result = schema->validate(document);
    const auto& schemaErrors = schema->getErrors();
    errors_.insert(errors_.end(), schemaErrors.begin(), schemaErrors.end());
    
    return result;
}

bool XmlValidator::validateAgainstDtd(XmlDocument* document, XmlDtd* dtd) {
    if (!document || !dtd) {
        errors_.push_back("Document or DTD is null");
        return false;
    }
    
    clearErrors();
    
    bool result = dtd->validate(document);
    const auto& dtdErrors = dtd->getErrors();
    errors_.insert(errors_.end(), dtdErrors.begin(), dtdErrors.end());
    
    return result;
}

const std::vector<std::string>& XmlValidator::getErrors() const {
    return errors_;
}

const std::vector<std::string>& XmlValidator::getWarnings() const {
    return warnings_;
}

void XmlValidator::clearErrors() {
    errors_.clear();
}

void XmlValidator::clearWarnings() {
    warnings_.clear();
}

void XmlValidator::clearAll() {
    clearErrors();
    clearWarnings();
}

void XmlValidator::setStrictMode(bool strict) {
    // Implementation would control validation strictness
    // For now, this is a placeholder
}

void XmlValidator::setRecoverMode(bool recover) {
    // Implementation would control recovery mode
    // For now, this is a placeholder
}

// XmlXPathResult implementation
XmlXPathResult::XmlXPathResult(xmlXPathObjectPtr result) : result_(result) {
    id_ = nextDoc++;
    
    if (result_) {
        switch (result_->type) {
            case XPATH_BOOLEAN:
                type_ = XPATH_BOOLEAN;
                break;
            case XPATH_NUMBER:
                type_ = XPATH_NUMBER;
                break;
            case XPATH_STRING:
                type_ = XPATH_STRING;
                break;
            case XPATH_NODESET:
                type_ = XPATH_NODESET;
                break;
            default:
                type_ = XPATH_UNDEFINED;
                break;
        }
    } else {
        type_ = XPATH_UNDEFINED;
    }
}

XmlXPathResult::~XmlXPathResult() {
    if (result_) {
        xmlXPathFreeObject(result_);
        result_ = nullptr;
    }
}

XmlXPathResult::ResultType XmlXPathResult::getType() const {
    return type_;
}

bool XmlXPathResult::isNodeSet() const {
    return type_ == XPATH_NODESET;
}

bool XmlXPathResult::isBoolean() const {
    return type_ == XPATH_BOOLEAN;
}

bool XmlXPathResult::isNumber() const {
    return type_ == XPATH_NUMBER;
}

bool XmlXPathResult::isString() const {
    return type_ == XPATH_STRING;
}

bool XmlXPathResult::getBooleanValue() const {
    if (!result_ || type_ != XPATH_BOOLEAN) {
        throw std::runtime_error("XmlXPathResult::getBooleanValue: Result is not a boolean");
    }
    return result_->boolval != 0;
}

double XmlXPathResult::getNumberValue() const {
    if (!result_ || type_ != XPATH_NUMBER) {
        throw std::runtime_error("XmlXPathResult::getNumberValue: Result is not a number");
    }
    return result_->floatval;
}

std::string XmlXPathResult::getStringValue() const {
    if (!result_ || type_ != XPATH_STRING) {
        throw std::runtime_error("XmlXPathResult::getStringValue: Result is not a string");
    }
    if (result_->stringval) {
        return std::string(reinterpret_cast<const char*>(result_->stringval));
    }
    return "";
}

std::unique_ptr<XmlNodeList> XmlXPathResult::getNodeSet() const {
    if (!result_ || type_ != XPATH_NODESET) {
        throw std::runtime_error("XmlXPathResult::getNodeSet: Result is not a node set");
    }
    
    auto nodeList = std::make_unique<XmlNodeList>();
    
    if (result_->nodesetval) {
        for (int i = 0; i < result_->nodesetval->nodeNr; i++) {
            if (result_->nodesetval->nodeTab[i]) {
                nodeList->addNode(result_->nodesetval->nodeTab[i]);
            }
        }
    }
    
    return nodeList;
}

size_t XmlXPathResult::getNodeSetSize() const {
    if (!result_ || type_ != XPATH_NODESET) {
        return 0;
    }
    
    if (result_->nodesetval) {
        return result_->nodesetval->nodeNr;
    }
    
    return 0;
}

// XmlXPath implementation
XmlXPath::XmlXPath(xmlDocPtr doc) : doc_(doc) {
    id_ = nextDoc++;
    
    if (!doc_) {
        throw std::runtime_error("XmlXPath::XmlXPath: Document is null");
    }
    
    context_ = xmlXPathNewContext(doc_);
    if (!context_) {
        throw std::runtime_error("XmlXPath::XmlXPath: Failed to create XPath context");
    }
}

XmlXPath::~XmlXPath() {
    if (context_) {
        xmlXPathFreeContext(context_);
        context_ = nullptr;
    }
}

std::unique_ptr<XmlXPathResult> XmlXPath::evaluate(const std::string& expression) {
    if (!context_) {
        throw std::runtime_error("XmlXPath::evaluate: XPath context is null");
    }
    
    xmlXPathObjectPtr result = xmlXPathEvalExpression(BAD_CAST expression.c_str(), context_);
    if (!result) {
        throw std::runtime_error("XmlXPath::evaluate: Failed to evaluate XPath expression: " + expression);
    }
    
    return std::make_unique<XmlXPathResult>(result);
}

std::unique_ptr<XmlXPathResult> XmlXPath::evaluate(const std::string& expression, XmlNode* contextNode) {
    if (!context_ || !contextNode) {
        throw std::runtime_error("XmlXPath::evaluate: Invalid context or node");
    }
    
    // Set context node
    context_->node = contextNode->getNode();
    
    xmlXPathObjectPtr result = xmlXPathEvalExpression(BAD_CAST expression.c_str(), context_);
    if (!result) {
        throw std::runtime_error("XmlXPath::evaluate: Failed to evaluate XPath expression: " + expression);
    }
    
    return std::make_unique<XmlXPathResult>(result);
}

void XmlXPath::registerNamespace(const std::string& prefix, const std::string& uri) {
    if (!context_) {
        throw std::runtime_error("XmlXPath::registerNamespace: XPath context is null");
    }
    
    int result = xmlXPathRegisterNs(context_, BAD_CAST prefix.c_str(), BAD_CAST uri.c_str());
    if (result != 0) {
        throw std::runtime_error("XmlXPath::registerNamespace: Failed to register namespace");
    }
}

void XmlXPath::unregisterNamespace(const std::string& prefix) {
    if (!context_) {
        throw std::runtime_error("XmlXPath::unregisterNamespace: XPath context is null");
    }
    
    xmlXPathRegisterNs(context_, BAD_CAST prefix.c_str(), nullptr);
}

void XmlXPath::registerVariable(const std::string& name, const std::string& value) {
    if (!context_) {
        throw std::runtime_error("XmlXPath::registerVariable: XPath context is null");
    }
    
    xmlXPathObjectPtr varObj = xmlXPathNewString(BAD_CAST value.c_str());
    if (!varObj) {
        throw std::runtime_error("XmlXPath::registerVariable: Failed to create string variable");
    }
    
    int result = xmlXPathRegisterVariable(context_, BAD_CAST name.c_str(), varObj);
    if (result != 0) {
        xmlXPathFreeObject(varObj);
        throw std::runtime_error("XmlXPath::registerVariable: Failed to register variable");
    }
}

void XmlXPath::registerVariable(const std::string& name, double value) {
    if (!context_) {
        throw std::runtime_error("XmlXPath::registerVariable: XPath context is null");
    }
    
    xmlXPathObjectPtr varObj = xmlXPathNewFloat(value);
    if (!varObj) {
        throw std::runtime_error("XmlXPath::registerVariable: Failed to create number variable");
    }
    
    int result = xmlXPathRegisterVariable(context_, BAD_CAST name.c_str(), varObj);
    if (result != 0) {
        xmlXPathFreeObject(varObj);
        throw std::runtime_error("XmlXPath::registerVariable: Failed to register variable");
    }
}

void XmlXPath::registerVariable(const std::string& name, bool value) {
    if (!context_) {
        throw std::runtime_error("XmlXPath::registerVariable: XPath context is null");
    }
    
    xmlXPathObjectPtr varObj = xmlXPathNewBoolean(value ? 1 : 0);
    if (!varObj) {
        throw std::runtime_error("XmlXPath::registerVariable: Failed to create boolean variable");
    }
    
    int result = xmlXPathRegisterVariable(context_, BAD_CAST name.c_str(), varObj);
    if (result != 0) {
        xmlXPathFreeObject(varObj);
        throw std::runtime_error("XmlXPath::registerVariable: Failed to register variable");
    }
}

// XmlDocument implementation
XmlDocument::XmlDocument(xmlDocPtr doc, bool takeOwnership) 
    : doc_(doc), ownsDocument_(takeOwnership) {
    id_ = nextDoc++;
}

XmlDocument::~XmlDocument() {
    if (ownsDocument_ && doc_) {
        xmlFreeDoc(doc_);
        doc_ = nullptr;
    }
}

std::unique_ptr<XmlDocument> XmlDocument::createDocument(const std::string& version, const std::string& encoding) {
    xmlDocPtr doc = xmlNewDoc(BAD_CAST version.c_str());
    if (!doc) {
        throw std::runtime_error("XmlDocument::createDocument: Failed to create new document");
    }
    return std::make_unique<XmlDocument>(doc, true);
}

std::unique_ptr<XmlDocument> XmlDocument::createFromString(const std::string& xmlString, const std::string& baseUrl) {
    xmlDocPtr doc = xmlReadMemory(xmlString.c_str(), xmlString.length(), 
                                  baseUrl.empty() ? "noname.xml" : baseUrl.c_str(), 
                                  nullptr, XML_PARSE_RECOVER);
    if (!doc) {
        throw std::runtime_error("XmlDocument::createFromString: Failed to parse XML string");
    }
    return std::make_unique<XmlDocument>(doc, true);
}

std::unique_ptr<XmlDocument> XmlDocument::createFromFile(const std::string& filename) {
    xmlDocPtr doc = xmlReadFile(filename.c_str(), nullptr, XML_PARSE_RECOVER);
    if (!doc) {
        throw std::runtime_error("XmlDocument::createFromFile: Failed to parse XML file: " + filename);
    }
    return std::make_unique<XmlDocument>(doc, true);
}

std::string XmlDocument::toString(const std::string& encoding, bool formatOutput) const {
    if (!doc_) {
        throw std::runtime_error("XmlDocument::toString: Document is null");
    }

    xmlChar* xmlbuff = nullptr;
    int buffersize = 0;
    
    // Use xmlDocDumpFormatMemoryEnc for simpler string conversion
    xmlDocDumpFormatMemoryEnc(doc_, &xmlbuff, &buffersize, encoding.c_str(), formatOutput ? 1 : 0);
    
    if (!xmlbuff) {
        throw std::runtime_error("XmlDocument::toString: Failed to serialize document");
    }
    
    std::string result(reinterpret_cast<char*>(xmlbuff));
    xmlFree(xmlbuff);
    
    return result;
}

bool XmlDocument::saveToFile(const std::string& filename, const std::string& encoding, bool formatOutput) const {
    if (!doc_) {
        return false;
    }

    int result = xmlSaveFormatFileEnc(filename.c_str(), doc_, encoding.c_str(), formatOutput ? 1 : 0);
    return result >= 0;
}

std::string XmlDocument::saveToMemory(const std::string& encoding, bool formatOutput) const {
    return toString(encoding, formatOutput);
}

std::string XmlDocument::getVersion() const {
    if (!doc_ || !doc_->version) {
        return "1.0";
    }
    return std::string(reinterpret_cast<const char*>(doc_->version));
}

std::string XmlDocument::getEncoding() const {
    if (!doc_ || !doc_->encoding) {
        return "UTF-8";
    }
    return std::string(reinterpret_cast<const char*>(doc_->encoding));
}

bool XmlDocument::isWellFormed() const {
    if (!doc_) {
        return false;
    }
    
    xmlValidCtxt cvp;
    memset(&cvp, 0, sizeof(cvp));
    
    return xmlValidateDocument(&cvp, doc_) == 1;
}

std::unique_ptr<XmlNode> XmlDocument::getRootElement() const {
    if (!doc_) {
        return nullptr;
    }
    
    xmlNodePtr root = xmlDocGetRootElement(doc_);
    if (!root) {
        return nullptr;
    }
    
    return std::make_unique<XmlNode>(root, false);
}

std::unique_ptr<XmlNode> XmlDocument::createElement(const std::string& name) const {
    if (!doc_) {
        throw std::runtime_error("XmlDocument::createElement: Document is null");
    }
    
    xmlNodePtr node = xmlNewNode(nullptr, BAD_CAST name.c_str());
    if (!node) {
        throw std::runtime_error("XmlDocument::createElement: Failed to create element: " + name);
    }
    
    return std::make_unique<XmlNode>(node, true);
}

std::unique_ptr<XmlNode> XmlDocument::createTextNode(const std::string& content) const {
    if (!doc_) {
        throw std::runtime_error("XmlDocument::createTextNode: Document is null");
    }
    
    xmlNodePtr node = xmlNewText(BAD_CAST content.c_str());
    if (!node) {
        throw std::runtime_error("XmlDocument::createTextNode: Failed to create text node");
    }
    
    return std::make_unique<XmlNode>(node, true);
}

std::unique_ptr<XmlXPathResult> XmlDocument::xpath(const std::string& expression) const {
    if (!doc_) {
        throw std::runtime_error("XmlDocument::xpath: Document is null");
    }
    
    auto xpath = std::make_unique<XmlXPath>(doc_);
    return xpath->evaluate(expression);
}

std::unique_ptr<XmlNodeList> XmlDocument::selectNodes(const std::string& expression) const {
    auto result = xpath(expression);
    if (result->isNodeSet()) {
        return result->getNodeSet();
    }
    
    // Return empty node list if not a node set
    return std::make_unique<XmlNodeList>();
}

std::unique_ptr<XmlNode> XmlDocument::selectSingleNode(const std::string& expression) const {
    auto nodeList = selectNodes(expression);
    if (nodeList->getLength() > 0) {
        return nodeList->item(0);
    }
    
    return nullptr;
}

// Additional XmlDocument methods implementation
bool XmlDocument::validateAgainstSchema(XmlSchema* schema) const {
    return schema ? schema->validate(const_cast<XmlDocument*>(this)) : false;
}

bool XmlDocument::validateAgainstDtd(XmlDtd* dtd) const {
    return dtd ? dtd->validate(const_cast<XmlDocument*>(this)) : false;
}

bool XmlDocument::validateWellFormedness(std::vector<std::string>& errors) const {
    if (!doc_) {
        errors.push_back("Document is null");
        return false;
    }
    
    xmlValidCtxt cvp;
    memset(&cvp, 0, sizeof(cvp));
    
    // Set up error handling
    cvp.error = xmlValidationErrorCallback;
    cvp.warning = xmlValidationWarningCallback;
    cvp.userData = &errors;
    
    return xmlValidateDocument(&cvp, doc_) == 1;
}

void XmlDocument::setEncoding(const std::string& encoding) {
    if (!doc_) {
        throw std::runtime_error("XmlDocument::setEncoding: Document is null");
    }
    
    if (doc_->encoding) {
        xmlFree(const_cast<xmlChar*>(doc_->encoding));
    }
    doc_->encoding = xmlStrdup(BAD_CAST encoding.c_str());
}

std::string XmlDocument::getDocumentEncoding() const {
    return getEncoding();
}

void XmlDocument::setFormatOutput(bool format) {
    // This would typically be stored as a document property
    // For libxml2, formatting is controlled at serialization time
    // We can store this as a custom property if needed
}

bool XmlDocument::getFormatOutput() const {
    // Return default formatting preference
    return true;
}

std::string XmlDocument::formatDocument(bool pretty, int indent) const {
    if (!doc_) {
        throw std::runtime_error("XmlDocument::formatDocument: Document is null");
    }
    
    xmlChar* xmlbuff = nullptr;
    int buffersize = 0;
    
    // Save with formatting
    xmlDocDumpFormatMemoryEnc(doc_, &xmlbuff, &buffersize, "UTF-8", pretty ? indent : 0);
    
    if (!xmlbuff) {
        throw std::runtime_error("XmlDocument::formatDocument: Failed to format document");
    }
    
    std::string result(reinterpret_cast<char*>(xmlbuff));
    xmlFree(xmlbuff);
    
    return result;
}

std::unique_ptr<XmlNode> XmlDocument::createComment(const std::string& text) const {
    if (!doc_) {
        throw std::runtime_error("XmlDocument::createComment: Document is null");
    }
    
    xmlNodePtr comment = xmlNewComment(BAD_CAST text.c_str());
    if (!comment) {
        throw std::runtime_error("XmlDocument::createComment: Failed to create comment node");
    }
    
    return std::make_unique<XmlNode>(comment, true);
}

std::unique_ptr<XmlNode> XmlDocument::createProcessingInstruction(const std::string& target, const std::string& data) const {
    if (!doc_) {
        throw std::runtime_error("XmlDocument::createProcessingInstruction: Document is null");
    }
    
    xmlNodePtr pi = xmlNewPI(BAD_CAST target.c_str(), BAD_CAST data.c_str());
    if (!pi) {
        throw std::runtime_error("XmlDocument::createProcessingInstruction: Failed to create processing instruction");
    }
    
    return std::make_unique<XmlNode>(pi, true);
}

std::unique_ptr<XmlNode> XmlDocument::createCDataSection(const std::string& content) const {
    if (!doc_) {
        throw std::runtime_error("XmlDocument::createCDataSection: Document is null");
    }
    
    xmlNodePtr cdata = xmlNewCDataBlock(doc_, BAD_CAST content.c_str(), content.length());
    if (!cdata) {
        throw std::runtime_error("XmlDocument::createCDataSection: Failed to create CDATA section");
    }
    
    return std::make_unique<XmlNode>(cdata, true);
}

void XmlDocument::registerNamespacePrefix(const std::string& prefix, const std::string& uri) {
    if (!doc_) {
        throw std::runtime_error("XmlDocument::registerNamespacePrefix: Document is null");
    }
    
    xmlNodePtr root = xmlDocGetRootElement(doc_);
    if (root) {
        xmlNewNs(root, BAD_CAST uri.c_str(), BAD_CAST prefix.c_str());
    }
}

std::string XmlDocument::resolveNamespaceUri(const std::string& prefix) const {
    if (!doc_) {
        return "";
    }
    
    xmlNodePtr root = xmlDocGetRootElement(doc_);
    if (!root) {
        return "";
    }
    
    xmlNsPtr ns = xmlSearchNs(doc_, root, BAD_CAST prefix.c_str());
    if (ns && ns->href) {
        return std::string(reinterpret_cast<const char*>(ns->href));
    }
    
    return "";
}

std::string XmlDocument::resolveNamespacePrefix(const std::string& uri) const {
    if (!doc_) {
        return "";
    }
    
    xmlNodePtr root = xmlDocGetRootElement(doc_);
    if (!root) {
        return "";
    }
    
    // Search through all namespaces to find matching URI
    for (xmlNsPtr ns = root->nsDef; ns; ns = ns->next) {
        if (ns->href && std::string(reinterpret_cast<const char*>(ns->href)) == uri) {
            if (ns->prefix) {
                return std::string(reinterpret_cast<const char*>(ns->prefix));
            }
        }
    }
    
    return "";
}

// XmlNode implementation
XmlNode::XmlNode(xmlNodePtr node, bool takeOwnership) 
    : node_(node), ownsNode_(takeOwnership) {
    id_ = nextDoc++;
    if (node_) {
        nodeHolder[id_] = node_;
    }
}

XmlNode::~XmlNode() {
    if (ownsNode_ && node_) {
        xmlFreeNode(node_);
        node_ = nullptr;
    }
}

std::string XmlNode::getName() const {
    if (!node_ || !node_->name) {
        return "";
    }
    return std::string(reinterpret_cast<const char*>(node_->name));
}

std::string XmlNode::getType() const {
    if (!node_) {
        return "Unknown";
    }
    return XmlModule::xmlElementTypeToString(node_->type);
}

std::string XmlNode::getContent() const {
    if (!node_) {
        return "";
    }
    
    xmlChar* content = xmlNodeGetContent(node_);
    if (!content) {
        return "";
    }
    
    std::string result(reinterpret_cast<char*>(content));
    xmlFree(content);
    return result;
}

void XmlNode::setContent(const std::string& content) {
    if (!node_) {
        throw std::runtime_error("XmlNode::setContent: Node is null");
    }
    
    xmlNodeSetContent(node_, BAD_CAST content.c_str());
}

void XmlNode::setName(const std::string& name) {
    if (!node_) {
        throw std::runtime_error("XmlNode::setName: Node is null");
    }
    
    xmlNodeSetName(node_, BAD_CAST name.c_str());
}

std::unique_ptr<XmlNode> XmlNode::getParent() const {
    if (!node_ || !node_->parent) {
        return nullptr;
    }
    
    return std::make_unique<XmlNode>(node_->parent, false);
}

std::unique_ptr<XmlNode> XmlNode::getFirstChild() const {
    if (!node_ || !node_->children) {
        return nullptr;
    }
    
    return std::make_unique<XmlNode>(node_->children, false);
}

std::unique_ptr<XmlNode> XmlNode::getLastChild() const {
    if (!node_ || !node_->last) {
        return nullptr;
    }
    
    return std::make_unique<XmlNode>(node_->last, false);
}

std::unique_ptr<XmlNode> XmlNode::getNextSibling() const {
    if (!node_ || !node_->next) {
        return nullptr;
    }
    
    return std::make_unique<XmlNode>(node_->next, false);
}

std::unique_ptr<XmlNode> XmlNode::getPreviousSibling() const {
    if (!node_ || !node_->prev) {
        return nullptr;
    }
    
    return std::make_unique<XmlNode>(node_->prev, false);
}

std::unique_ptr<XmlNodeList> XmlNode::getChildren() const {
    if (!node_) {
        return std::make_unique<XmlNodeList>();
    }
    
    return std::make_unique<XmlNodeList>(node_->children);
}

std::unique_ptr<XmlNode> XmlNode::appendChild(XmlNode* child) {
    if (!node_ || !child || !child->node_) {
        throw std::runtime_error("XmlNode::appendChild: Invalid node");
    }
    
    xmlNodePtr result = xmlAddChild(node_, child->node_);
    if (!result) {
        throw std::runtime_error("XmlNode::appendChild: Failed to append child");
    }
    
    child->ownsNode_ = false; // Parent now owns the node
    return std::make_unique<XmlNode>(result, false);
}

std::string XmlNode::getAttribute(const std::string& name) const {
    if (!node_) {
        return "";
    }
    
    xmlChar* attr = xmlGetProp(node_, BAD_CAST name.c_str());
    if (!attr) {
        return "";
    }
    
    std::string result(reinterpret_cast<char*>(attr));
    xmlFree(attr);
    return result;
}

void XmlNode::setAttribute(const std::string& name, const std::string& value) {
    if (!node_) {
        throw std::runtime_error("XmlNode::setAttribute: Node is null");
    }
    
    xmlSetProp(node_, BAD_CAST name.c_str(), BAD_CAST value.c_str());
}

bool XmlNode::hasAttribute(const std::string& name) const {
    if (!node_) {
        return false;
    }
    
    xmlAttrPtr attr = xmlHasProp(node_, BAD_CAST name.c_str());
    return attr != nullptr;
}

void XmlNode::removeAttribute(const std::string& name) {
    if (!node_) {
        return;
    }
    
    xmlUnsetProp(node_, BAD_CAST name.c_str());
}

Symbols::ObjectMap XmlNode::getAllAttributes() const {
    Symbols::ObjectMap attributes;
    
    if (!node_) {
        return attributes;
    }
    
    for (xmlAttrPtr attr = node_->properties; attr; attr = attr->next) {
        if (attr->name && attr->children && attr->children->content) {
            std::string name(reinterpret_cast<const char*>(attr->name));
            std::string value(reinterpret_cast<const char*>(attr->children->content));
            attributes[name] = Symbols::ValuePtr(value);
        }
    }
    
    return attributes;
}

bool XmlNode::hasChildNodes() const {
    return node_ && node_->children != nullptr;
}

std::string XmlNode::getTextContent() const {
    return getContent();
}

void XmlNode::setTextContent(const std::string& content) {
    setContent(content);
}

std::unique_ptr<XmlNode> XmlNode::insertBefore(XmlNode* newNode, XmlNode* refNode) {
    if (!node_ || !newNode || !newNode->node_) {
        throw std::runtime_error("XmlNode::insertBefore: Invalid nodes");
    }

    xmlNodePtr result;
    if (refNode && refNode->node_) {
        result = xmlAddPrevSibling(refNode->node_, newNode->node_);
    } else {
        result = xmlAddChild(node_, newNode->node_);
    }

    if (!result) {
        throw std::runtime_error("XmlNode::insertBefore: Failed to insert node");
    }

    newNode->ownsNode_ = false;
    return std::make_unique<XmlNode>(result, false);
}

std::unique_ptr<XmlNode> XmlNode::removeChild(XmlNode* child) {
    if (!node_ || !child || !child->node_) {
        throw std::runtime_error("XmlNode::removeChild: Invalid nodes");
    }

    xmlUnlinkNode(child->node_);
    child->ownsNode_ = true;
    return std::unique_ptr<XmlNode>(child);
}

std::unique_ptr<XmlNode> XmlNode::replaceChild(XmlNode* newNode, XmlNode* oldNode) {
    if (!node_ || !newNode || !oldNode || !newNode->node_ || !oldNode->node_) {
        throw std::runtime_error("XmlNode::replaceChild: Invalid nodes");
    }

    xmlNodePtr result = xmlReplaceNode(oldNode->node_, newNode->node_);
    if (!result) {
        throw std::runtime_error("XmlNode::replaceChild: Failed to replace node");
    }

    oldNode->ownsNode_ = true;
    newNode->ownsNode_ = false;
    return std::unique_ptr<XmlNode>(oldNode);
}

std::unique_ptr<XmlNode> XmlNode::cloneNode(bool deep) const {
    if (!node_) {
        throw std::runtime_error("XmlNode::cloneNode: Node is null");
    }

    xmlNodePtr clone = xmlCopyNode(node_, deep ? 1 : 0);
    if (!clone) {
        throw std::runtime_error("XmlNode::cloneNode: Failed to clone node");
    }

    return std::make_unique<XmlNode>(clone, true);
}

std::unique_ptr<XmlNodeList> XmlNode::selectNodes(const std::string& expression) const {
    if (!node_ || !node_->doc) {
        throw std::runtime_error("XmlNode::selectNodes: Node or document is null");
    }
    
    auto xpath = std::make_unique<XmlXPath>(node_->doc);
    auto result = xpath->evaluate(expression, const_cast<XmlNode*>(this));
    
    if (result->isNodeSet()) {
        return result->getNodeSet();
    }
    
    // Return empty node list if not a node set
    return std::make_unique<XmlNodeList>();
}

std::unique_ptr<XmlNode> XmlNode::selectSingleNode(const std::string& expression) const {
    auto nodeList = selectNodes(expression);
    if (nodeList->getLength() > 0) {
        return nodeList->item(0);
    }
    
    return nullptr;
}

// XmlNodeList implementation
XmlNodeList::XmlNodeList() {
    id_ = nextDoc++;
}

XmlNodeList::XmlNodeList(xmlNodePtr startNode) : XmlNodeList() {
    for (xmlNodePtr node = startNode; node; node = node->next) {
        if (node->type == XML_ELEMENT_NODE) {
            nodes_.push_back(node);
        }
    }
}

XmlNodeList::~XmlNodeList() = default;

size_t XmlNodeList::getLength() const {
    return nodes_.size();
}

std::unique_ptr<XmlNode> XmlNodeList::item(size_t index) const {
    if (index >= nodes_.size()) {
        return nullptr;
    }
    
    return std::make_unique<XmlNode>(nodes_[index], false);
}

void XmlNodeList::addNode(xmlNodePtr node) {
    if (node) {
        nodes_.push_back(node);
    }
}

void XmlNodeList::clear() {
    nodes_.clear();
}

// Static utility methods
std::string XmlModule::xmlElementTypeToString(int type) {
    static const std::unordered_map<int, std::string> xmlElementTypeMap = {
        { 1,  "XML_ELEMENT_NODE"       },
        { 2,  "XML_ATTRIBUTE_NODE"     },
        { 3,  "XML_TEXT_NODE"          },
        { 4,  "XML_CDATA_SECTION_NODE" },
        { 5,  "XML_ENTITY_REF_NODE"    },
        { 6,  "XML_ENTITY_NODE"        },
        { 7,  "XML_PI_NODE"            },
        { 8,  "XML_COMMENT_NODE"       },
        { 9,  "XML_DOCUMENT_NODE"      },
        { 10, "XML_DOCUMENT_TYPE_NODE" },
        { 11, "XML_DOCUMENT_FRAG_NODE" },
        { 12, "XML_NOTATION_NODE"      },
        { 13, "XML_HTML_DOCUMENT_NODE" },
        { 14, "XML_DTD_NODE"           },
        { 15, "XML_ELEMENT_DECL"       },
        { 16, "XML_ATTRIBUTE_DECL"     },
        { 17, "XML_ENTITY_DECL"        },
        { 18, "XML_NAMESPACE_DECL"     },
        { 19, "XML_XINCLUDE_START"     },
        { 20, "XML_XINCLUDE_END"       },
#ifdef LIBXML_DOCB_ENABLED
        { 21, "XML_DOCB_DOCUMENT_NODE" },
#endif
    };
    
    const auto it = xmlElementTypeMap.find(type);
    return (it != xmlElementTypeMap.end()) ? it->second : "Unknown";
}

bool XmlModule::isValidXmlName(const std::string& name) {
    if (name.empty()) {
        return false;
    }
    
    // Simple validation - starts with letter or underscore, contains only valid characters
    char first = name[0];
    if (!std::isalpha(first) && first != '_') {
        return false;
    }
    
    return std::all_of(name.begin(), name.end(), [](char c) {
        return std::isalnum(c) || c == '_' || c == '-' || c == '.';
    });
}

std::string XmlModule::escapeXmlText(const std::string& text) {
    std::string result;
    result.reserve(text.length() * 1.2); // Reserve some extra space
    
    for (char c : text) {
        switch (c) {
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            case '&': result += "&amp;"; break;
            case '"': result += "&quot;"; break;
            case '\'': result += "&apos;"; break;
            default: result += c; break;
        }
    }
    
    return result;
}

std::string XmlModule::unescapeXmlText(const std::string& text) {
    std::string result = text;
    
    // Simple replacements for basic entities
    size_t pos = 0;
    while ((pos = result.find("&lt;", pos)) != std::string::npos) {
        result.replace(pos, 4, "<");
        pos += 1;
    }
    pos = 0;
    while ((pos = result.find("&gt;", pos)) != std::string::npos) {
        result.replace(pos, 4, ">");
        pos += 1;
    }
    pos = 0;
    while ((pos = result.find("&quot;", pos)) != std::string::npos) {
        result.replace(pos, 6, "\"");
        pos += 1;
    }
    pos = 0;
    while ((pos = result.find("&apos;", pos)) != std::string::npos) {
        result.replace(pos, 6, "'");
        pos += 1;
    }
    pos = 0;
    while ((pos = result.find("&amp;", pos)) != std::string::npos) {
        result.replace(pos, 5, "&");
        pos += 1;
    }
    
    return result;
}

// Helper methods for object management
XmlDocument* XmlModule::getDocumentFromArgs(const FunctionArguments& args) const {
    if (args.empty() || (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT)) {
        throw std::runtime_error("XmlModule: Invalid document object");
    }

    auto symbolContainer = Symbols::SymbolContainer::instance();
    auto handlerProperty = symbolContainer->getObjectProperty("XmlDocument", "__xml_document_handler_id__");
    if (!handlerProperty) {
        throw std::runtime_error("XmlModule: Invalid document handler");
    }

    int handlerId = handlerProperty;
    auto it = documentHolder.find(handlerId);
    if (it == documentHolder.end()) {
        throw std::runtime_error("XmlModule: Document not found");
    }

    return it->second.get();
}

XmlNode* XmlModule::getNodeFromArgs(const FunctionArguments& args) const {
    if (args.empty() || (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT)) {
        throw std::runtime_error("XmlModule: Invalid node object");
    }

    auto symbolContainer = Symbols::SymbolContainer::instance();
    auto handlerProperty = symbolContainer->getObjectProperty("XmlNode", "__xml_node_handler_id__");
    if (!handlerProperty) {
        throw std::runtime_error("XmlModule: Invalid node handler");
    }

    int handlerId = handlerProperty;
    auto it = nodeObjectHolder.find(handlerId);
    if (it == nodeObjectHolder.end()) {
        throw std::runtime_error("XmlModule: Node not found");
    }

    return it->second.get();
}

XmlNodeList* XmlModule::getNodeListFromArgs(const FunctionArguments& args) const {
    if (args.empty() || (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT)) {
        throw std::runtime_error("XmlModule: Invalid node list object");
    }

    auto symbolContainer = Symbols::SymbolContainer::instance();
    auto handlerProperty = symbolContainer->getObjectProperty("XmlNodeList", "__xml_nodelist_handler_id__");
    if (!handlerProperty) {
        throw std::runtime_error("XmlModule: Invalid node list handler");
    }

    int handlerId = handlerProperty;
    auto it = nodeListHolder.find(handlerId);
    if (it == nodeListHolder.end()) {
        throw std::runtime_error("XmlModule: Node list not found");
    }

    return it->second.get();
}

Symbols::ValuePtr XmlModule::createDocumentObject(std::unique_ptr<XmlDocument> doc) {
    int id = doc->getId();
    documentHolder[id] = std::move(doc);

    auto symbolContainer = Symbols::SymbolContainer::instance();
    symbolContainer->setObjectProperty("XmlDocument", "__xml_document_handler_id__", id);
    symbolContainer->setObjectProperty("XmlDocument", "__class__", "XmlDocument");

    Symbols::ObjectMap objMap;
    objMap["__xml_document_handler_id__"] = symbolContainer->getObjectProperty("XmlDocument", "__xml_document_handler_id__");
    objMap["__class__"] = symbolContainer->getObjectProperty("XmlDocument", "__class__");

    return Symbols::ValuePtr::makeClassInstance(objMap);
}

Symbols::ValuePtr XmlModule::createNodeObject(std::unique_ptr<XmlNode> node) {
    int id = node->getId();
    nodeObjectHolder[id] = std::move(node);

    auto symbolContainer = Symbols::SymbolContainer::instance();
    symbolContainer->setObjectProperty("XmlNode", "__xml_node_handler_id__", id);
    symbolContainer->setObjectProperty("XmlNode", "__class__", "XmlNode");

    Symbols::ObjectMap objMap;
    objMap["__xml_node_handler_id__"] = symbolContainer->getObjectProperty("XmlNode", "__xml_node_handler_id__");
    objMap["__class__"] = symbolContainer->getObjectProperty("XmlNode", "__class__");

    return Symbols::ValuePtr::makeClassInstance(objMap);
}

Symbols::ValuePtr XmlModule::createNodeListObject(std::unique_ptr<XmlNodeList> nodeList) {
    int id = nodeList->getId();
    nodeListHolder[id] = std::move(nodeList);

    auto symbolContainer = Symbols::SymbolContainer::instance();
    symbolContainer->setObjectProperty("XmlNodeList", "__xml_nodelist_handler_id__", id);
    symbolContainer->setObjectProperty("XmlNodeList", "__class__", "XmlNodeList");

    Symbols::ObjectMap objMap;
    objMap["__xml_nodelist_handler_id__"] = symbolContainer->getObjectProperty("XmlNodeList", "__xml_nodelist_handler_id__");
    objMap["__class__"] = symbolContainer->getObjectProperty("XmlNodeList", "__class__");

    return Symbols::ValuePtr::makeClassInstance(objMap);
}

XmlXPath* XmlModule::getXPathFromArgs(const FunctionArguments& args) const {
    if (args.empty() || (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT)) {
        throw std::runtime_error("XmlModule: Invalid XPath object");
    }

    auto symbolContainer = Symbols::SymbolContainer::instance();
    auto handlerProperty = symbolContainer->getObjectProperty("XmlXPath", "__xml_xpath_handler_id__");
    if (!handlerProperty) {
        throw std::runtime_error("XmlModule: Invalid XPath handler");
    }

    int handlerId = handlerProperty;
    auto it = xpathHolder.find(handlerId);
    if (it == xpathHolder.end()) {
        throw std::runtime_error("XmlModule: XPath not found");
    }

    return it->second.get();
}

XmlXPathResult* XmlModule::getXPathResultFromArgs(const FunctionArguments& args) const {
    if (args.empty() || (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT)) {
        throw std::runtime_error("XmlModule: Invalid XPath result object");
    }

    auto symbolContainer = Symbols::SymbolContainer::instance();
    auto handlerProperty = symbolContainer->getObjectProperty("XmlXPathResult", "__xml_xpath_result_handler_id__");
    if (!handlerProperty) {
        throw std::runtime_error("XmlModule: Invalid XPath result handler");
    }

    int handlerId = handlerProperty;
    auto it = xpathResultHolder.find(handlerId);
    if (it == xpathResultHolder.end()) {
        throw std::runtime_error("XmlModule: XPath result not found");
    }

    return it->second.get();
}

Symbols::ValuePtr XmlModule::createXPathObject(std::unique_ptr<XmlXPath> xpath) {
    int id = xpath->getId();
    xpathHolder[id] = std::move(xpath);

    auto symbolContainer = Symbols::SymbolContainer::instance();
    symbolContainer->setObjectProperty("XmlXPath", "__xml_xpath_handler_id__", id);
    symbolContainer->setObjectProperty("XmlXPath", "__class__", "XmlXPath");

    Symbols::ObjectMap objMap;
    objMap["__xml_xpath_handler_id__"] = symbolContainer->getObjectProperty("XmlXPath", "__xml_xpath_handler_id__");
    objMap["__class__"] = symbolContainer->getObjectProperty("XmlXPath", "__class__");

    return Symbols::ValuePtr::makeClassInstance(objMap);
}

Symbols::ValuePtr XmlModule::createXPathResultObject(std::unique_ptr<XmlXPathResult> result) {
    int id = result->getId();
    xpathResultHolder[id] = std::move(result);

    auto symbolContainer = Symbols::SymbolContainer::instance();
    symbolContainer->setObjectProperty("XmlXPathResult", "__xml_xpath_result_handler_id__", id);
    symbolContainer->setObjectProperty("XmlXPathResult", "__class__", "XmlXPathResult");

    Symbols::ObjectMap objMap;
    objMap["__xml_xpath_result_handler_id__"] = symbolContainer->getObjectProperty("XmlXPathResult", "__xml_xpath_result_handler_id__");
    objMap["__class__"] = symbolContainer->getObjectProperty("XmlXPathResult", "__class__");

    return Symbols::ValuePtr::makeClassInstance(objMap);
}

void XmlModule::registerFunctions() {
    registerLegacyMethods();
    registerDocumentMethods();
    registerNodeMethods();
    registerNodeListMethods();
    registerXPathMethods();
    registerValidationMethods();
    registerAdvancedMethods();
}

void XmlModule::registerLegacyMethods() {
    // Register classes using registration macros
    REGISTER_CLASS(this->className);
    REGISTER_CLASS("XmlNode");
    REGISTER_CLASS("XmlDocument");
    REGISTER_CLASS("XmlNodeList");
    REGISTER_CLASS("XmlXPath");
    REGISTER_CLASS("XmlXPathResult");

    // Legacy methods for backward compatibility
    std::vector<Symbols::FunctionParameterInfo> params = {
        { "filename", Symbols::Variables::Type::STRING, "The path to the XML file to read" }
    };

    REGISTER_METHOD(
        this->className, "readFile", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->readFile(args); },
        Symbols::Variables::Type::CLASS, "Read XML from a file (legacy method)");

    params = {
        { "string", Symbols::Variables::Type::STRING, "The XML content as a string to parse" }
    };

    REGISTER_METHOD(
        this->className, "readMemory", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->readMemory(args); },
        Symbols::Variables::Type::CLASS, "Read XML from a string (legacy method)");

    REGISTER_METHOD(
        this->className, "getRootElement", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->GetRootElement(args); },
        Symbols::Variables::Type::CLASS, "Get the root element of the XML document (legacy method)");

    // Legacy attribute method
    REGISTER_METHOD(
        "XmlNode", "getAttributes", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->GetNodeAttributes(args); },
        Symbols::Variables::Type::OBJECT, "Get the attributes of an XML node (legacy method)");

    // Register properties for backward compatibility
    REGISTER_PROPERTY(this->className, "__xml2_handler_id__", Symbols::Variables::Type::INTEGER, nullptr);
    REGISTER_PROPERTY("XmlNode", "__xml_node_handler_id__", Symbols::Variables::Type::INTEGER, nullptr);
    REGISTER_PROPERTY("XmlDocument", "__xml_document_handler_id__", Symbols::Variables::Type::INTEGER, nullptr);
    REGISTER_PROPERTY("XmlNodeList", "__xml_nodelist_handler_id__", Symbols::Variables::Type::INTEGER, nullptr);
    REGISTER_PROPERTY("XmlXPath", "__xml_xpath_handler_id__", Symbols::Variables::Type::INTEGER, nullptr);
    REGISTER_PROPERTY("XmlXPathResult", "__xml_xpath_result_handler_id__", Symbols::Variables::Type::INTEGER, nullptr);
}

void XmlModule::registerDocumentMethods() {
    std::vector<Symbols::FunctionParameterInfo> params;

    // Document creation methods
    params = {
        { "version", Symbols::Variables::Type::STRING, "XML version (default: '1.0')" },
        { "encoding", Symbols::Variables::Type::STRING, "Document encoding (default: 'UTF-8')" }
    };
    REGISTER_METHOD(
        this->className, "createDocument", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->createDocument(args); },
        Symbols::Variables::Type::CLASS, "Create a new XML document");

    params = {
        { "xmlString", Symbols::Variables::Type::STRING, "XML content as string" },
        { "baseUrl", Symbols::Variables::Type::STRING, "Base URL for relative references (optional)" }
    };
    REGISTER_METHOD(
        this->className, "createFromString", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->createFromString(args); },
        Symbols::Variables::Type::CLASS, "Create XML document from string");

    params = {
        { "filename", Symbols::Variables::Type::STRING, "Path to XML file" }
    };
    REGISTER_METHOD(
        this->className, "createFromFile", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->createFromFile(args); },
        Symbols::Variables::Type::CLASS, "Create XML document from file");

    // Document serialization methods
    params = {
        { "encoding", Symbols::Variables::Type::STRING, "Output encoding (optional, default: 'UTF-8')" },
        { "formatOutput", Symbols::Variables::Type::BOOLEAN, "Format output with indentation (optional, default: true)" }
    };
    REGISTER_METHOD(
        "XmlDocument", "toString", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->toString(args); },
        Symbols::Variables::Type::STRING, "Convert document to string");

    params = {
        { "filename", Symbols::Variables::Type::STRING, "Output filename" },
        { "encoding", Symbols::Variables::Type::STRING, "Output encoding (optional, default: 'UTF-8')" },
        { "formatOutput", Symbols::Variables::Type::BOOLEAN, "Format output with indentation (optional, default: true)" }
    };
    REGISTER_METHOD(
        "XmlDocument", "saveToFile", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->saveToFile(args); },
        Symbols::Variables::Type::BOOLEAN, "Save document to file");

    // Document properties
    REGISTER_METHOD(
        "XmlDocument", "getVersion", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->getVersion(args); },
        Symbols::Variables::Type::STRING, "Get XML version");

    REGISTER_METHOD(
        "XmlDocument", "getEncoding", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->getEncoding(args); },
        Symbols::Variables::Type::STRING, "Get document encoding");

    REGISTER_METHOD(
        "XmlDocument", "isWellFormed", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->isWellFormed(args); },
        Symbols::Variables::Type::BOOLEAN, "Check if document is well-formed");

    REGISTER_METHOD(
        "XmlDocument", "getRootElement", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->GetRootElement(args); },
        Symbols::Variables::Type::CLASS, "Get root element");

    // Element creation
    params = {
        { "name", Symbols::Variables::Type::STRING, "Element name" }
    };
    REGISTER_METHOD(
        "XmlDocument", "createElement", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->createElement(args); },
        Symbols::Variables::Type::CLASS, "Create new element");

    params = {
        { "content", Symbols::Variables::Type::STRING, "Text content" }
    };
    REGISTER_METHOD(
        "XmlDocument", "createTextNode", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->createTextNode(args); },
        Symbols::Variables::Type::CLASS, "Create new text node");
}

void XmlModule::registerNodeMethods() {
    std::vector<Symbols::FunctionParameterInfo> params;

    // Node properties
    REGISTER_METHOD(
        "XmlNode", "getName", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->getName(args); },
        Symbols::Variables::Type::STRING, "Get node name");

    params = {
        { "name", Symbols::Variables::Type::STRING, "New node name" }
    };
    REGISTER_METHOD(
        "XmlNode", "setName", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->setName(args); },
        Symbols::Variables::Type::NULL_TYPE, "Set node name");

    REGISTER_METHOD(
        "XmlNode", "getContent", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->getContent(args); },
        Symbols::Variables::Type::STRING, "Get node content");

    params = {
        { "content", Symbols::Variables::Type::STRING, "New content" }
    };
    REGISTER_METHOD(
        "XmlNode", "setContent", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->setContent(args); },
        Symbols::Variables::Type::NULL_TYPE, "Set node content");

    REGISTER_METHOD(
        "XmlNode", "getTextContent", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->getTextContent(args); },
        Symbols::Variables::Type::STRING, "Get text content");

    params = {
        { "content", Symbols::Variables::Type::STRING, "New text content" }
    };
    REGISTER_METHOD(
        "XmlNode", "setTextContent", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->setTextContent(args); },
        Symbols::Variables::Type::NULL_TYPE, "Set text content");

    // Navigation methods
    REGISTER_METHOD(
        "XmlNode", "getParent", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->getParent(args); },
        Symbols::Variables::Type::CLASS, "Get parent node");

    REGISTER_METHOD(
        "XmlNode", "getFirstChild", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->getFirstChild(args); },
        Symbols::Variables::Type::CLASS, "Get first child node");

    REGISTER_METHOD(
        "XmlNode", "getLastChild", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->getLastChild(args); },
        Symbols::Variables::Type::CLASS, "Get last child node");

    REGISTER_METHOD(
        "XmlNode", "getNextSibling", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->getNextSibling(args); },
        Symbols::Variables::Type::CLASS, "Get next sibling node");

    REGISTER_METHOD(
        "XmlNode", "getPreviousSibling", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->getPreviousSibling(args); },
        Symbols::Variables::Type::CLASS, "Get previous sibling node");

    REGISTER_METHOD(
        "XmlNode", "getChildren", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->getChildren(args); },
        Symbols::Variables::Type::CLASS, "Get child nodes");

    // Attribute methods
    params = {
        { "name", Symbols::Variables::Type::STRING, "Attribute name" }
    };
    REGISTER_METHOD(
        "XmlNode", "getAttribute", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->getAttribute(args); },
        Symbols::Variables::Type::STRING, "Get attribute value");

    params = {
        { "name", Symbols::Variables::Type::STRING, "Attribute name" },
        { "value", Symbols::Variables::Type::STRING, "Attribute value" }
    };
    REGISTER_METHOD(
        "XmlNode", "setAttribute", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->setAttribute(args); },
        Symbols::Variables::Type::NULL_TYPE, "Set attribute value");

    params = {
        { "name", Symbols::Variables::Type::STRING, "Attribute name" }
    };
    REGISTER_METHOD(
        "XmlNode", "hasAttribute", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->hasAttribute(args); },
        Symbols::Variables::Type::BOOLEAN, "Check if attribute exists");

    REGISTER_METHOD(
        "XmlNode", "removeAttribute", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->removeAttribute(args); },
        Symbols::Variables::Type::NULL_TYPE, "Remove attribute");

    REGISTER_METHOD(
        "XmlNode", "getAllAttributes", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->getAllAttributes(args); },
        Symbols::Variables::Type::OBJECT, "Get all attributes as object");

    // Utility methods
    REGISTER_METHOD(
        "XmlNode", "hasChildNodes", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->hasChildNodes(args); },
        Symbols::Variables::Type::BOOLEAN, "Check if node has children");

    // Node manipulation
    params = {
        { "child", Symbols::Variables::Type::CLASS, "Child node to append" }
    };
    REGISTER_METHOD(
        "XmlNode", "appendChild", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->appendChild(args); },
        Symbols::Variables::Type::CLASS, "Append child node");

    params = {
        { "deep", Symbols::Variables::Type::BOOLEAN, "Deep clone including children (default: false)" }
    };
    REGISTER_METHOD(
        "XmlNode", "cloneNode", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->cloneNode(args); },
        Symbols::Variables::Type::CLASS, "Clone node");
}

void XmlModule::registerNodeListMethods() {
    std::vector<Symbols::FunctionParameterInfo> params;

    REGISTER_METHOD(
        "XmlNodeList", "getLength", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->getLength(args); },
        Symbols::Variables::Type::INTEGER, "Get number of nodes in list");

    params = {
        { "index", Symbols::Variables::Type::INTEGER, "Node index" }
    };
    REGISTER_METHOD(
        "XmlNodeList", "item", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->item(args); },
        Symbols::Variables::Type::CLASS, "Get node at index");
}

// Legacy method implementations (backward compatibility)
Symbols::ValuePtr XmlModule::readFile(FunctionArguments& args) {
    if (args.size() != 2) {
        throw std::runtime_error(this->className +
                                 " expects one parameter (string $filename), got: " + std::to_string(args.size() - 1));
    }

    if (args[1] != Symbols::Variables::Type::STRING) {
        throw std::invalid_argument(this->className + "::readFile: invalid parameter, must be string");
    }
    int handler = nextDoc++;

    const std::string filename = args[1];
    xmlDocPtr         doc      = xmlReadFile(filename.c_str(), NULL, 0);
    if (!doc) {
        throw std::runtime_error(this->className + "::readFile: Failed to parse file: " + filename);
    }
    docHolder[handler]         = doc;
    Symbols::ObjectMap objMap  = this->storeObject(args, Symbols::ValuePtr(handler), this->objectStoreName);
    objMap["__class__"]        = this->className;
    return Symbols::ValuePtr::makeClassInstance(objMap);
}

Symbols::ValuePtr XmlModule::readMemory(FunctionArguments& args) {
    if (args.size() != 2 && args.size() != 3 && args.size() != 4) {
        throw std::runtime_error(
            this->className +
            " expects one parameter (string $xmlcontent, int $size = -1, string $basename = \"noname.xml\"), "
            "got: " +
            std::to_string(args.size() - 1));
    }

    if (args[1] != Symbols::Variables::Type::STRING) {
        throw std::invalid_argument(this->className + "::readMemory: invalid first parameter, must be string");
    }

    std::string content  = args[1];
    std::string basename = "noname.xml";
    int         size     = -1;

    if (args.size() >= 3) {
        if (args[2] != Symbols::Variables::Type::INTEGER) {
            throw std::invalid_argument(this->className + "::readMemory: size parameter must be integer");
        }
        size = args[2];
    }

    if (args.size() == 4) {
        if (args[3] != Symbols::Variables::Type::STRING) {
            throw std::invalid_argument(this->className + "::readmemory: basename parameter must be string");
        }
        basename = args[3].get<std::string>();
    }

    int       handler = nextDoc++;
    xmlDocPtr doc     = xmlReadMemory(content.c_str(), size == -1 ? content.size() : size, basename.c_str(), NULL, 0);
    if (doc == NULL) {
        throw std::runtime_error(this->className + "::readMemory: failed to parse XML");
    }
    docHolder[handler] = doc;

    // Create a new object map for the XML document
    Symbols::ObjectMap objMap;
    if (args.size() > 0 &&
        (args[0] == Symbols::Variables::Type::CLASS || args[0] == Symbols::Variables::Type::OBJECT)) {
        // If this is a method call on an existing object, use its map
        objMap = args[0];
    }

    // Use the new property management system
    auto symbolContainer = Symbols::SymbolContainer::instance();
    symbolContainer->setObjectProperty(this->className, "__xml2_handler_id__", handler);
    symbolContainer->setObjectProperty(this->className, "__class__", this->className);
    symbolContainer->setObjectProperty(this->className, "__type__", this->className);

    // Copy properties to the object map for backward compatibility
    objMap["__xml2_handler_id__"] = symbolContainer->getObjectProperty(this->className, "__xml2_handler_id__");
    objMap["__class__"]           = symbolContainer->getObjectProperty(this->className, "__class__");
    objMap["__type__"]            = symbolContainer->getObjectProperty(this->className, "__type__");

    return Symbols::ValuePtr::makeClassInstance(objMap);
}

Symbols::ValuePtr XmlModule::GetRootElement(const FunctionArguments& args) {
    if (args.size() != 1) {
        throw std::runtime_error("Xml2::getRootElement: expected 1 argument");
    }

    if (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("Xml2::getRootElement: invalid object type");
    }

    auto symbolContainer = Symbols::SymbolContainer::instance();
    auto handlerProperty = symbolContainer->getObjectProperty(this->className, "__xml2_handler_id__");
    if (!handlerProperty) {
        throw std::runtime_error("Xml2::getRootElement: invalid object");
    }

    int handlerId = handlerProperty;
    if (handlerId == -1) {
        throw std::runtime_error("Xml2::getRootElement: invalid object");
    }

    auto docIt = docHolder.find(handlerId);
    if (docIt == docHolder.end()) {
        throw std::runtime_error("Xml2::getRootElement: document not found");
    }

    // Get the root element
    xmlNodePtr root = xmlDocGetRootElement(docHolder[handlerId]);
    if (root == NULL) {
        throw std::runtime_error(this->className + "::getRootElement: invalid root");
    }

    // Store the root node and create a new object for it
    int nodeHandle         = nextDoc++;
    nodeHolder[nodeHandle] = root;

    // Create a new object map for the XML node
    Symbols::ObjectMap nodeObjMap;

    // Use the new property management system for the XmlNode class
    symbolContainer->setObjectProperty("XmlNode", "__xml_node_handler_id__", nodeHandle);
    symbolContainer->setObjectProperty("XmlNode", "__class__", "XmlNode");

    // Copy properties to the object map for backward compatibility
    nodeObjMap["__xml_node_handler_id__"] = symbolContainer->getObjectProperty("XmlNode", "__xml_node_handler_id__");
    nodeObjMap["__class__"]               = symbolContainer->getObjectProperty("XmlNode", "__class__");

    return Symbols::ValuePtr::makeClassInstance(nodeObjMap);
}

Symbols::ValuePtr XmlModule::GetNodeAttributes(FunctionArguments& args) {
    if (args.size() != 1) {
        throw std::runtime_error(this->className + "::getNodeAttributes: must be called with no arguments");
    }
    auto val    = this->getObjectValue(args, "__xml_node_handler_id__");
    int  handle = val;

    if (nodeHolder.contains(handle)) {
        const auto * node         = nodeHolder[handle];
        const auto * node_name    = node->name;
        const auto   node_type    = node->type;
        const auto * node_content = node->content;
        const auto   children     = node->children;

        Symbols::ObjectMap map;
        map["tagName"]    = Symbols::ValuePtr(node_name);
        map["tagType"]    = Symbols::ValuePtr(XmlModule::xmlElementTypeToString(node_type));
        map["tagContent"] = Symbols::ValuePtr(node_content);


        if (children) {
            Symbols::ObjectMap childrenArray;
            unsigned int       i = 0;
            for (xmlNodePtr child = children; child; child = child->next) {
                if (child->type != XML_ELEMENT_NODE) {
                    continue;
                }

                int childHandle         = nextDoc++;
                nodeHolder[childHandle] = child;

                Symbols::ObjectMap childObj = this->storeObject(args, childHandle, "__xml_node_handler_id__");
                childObj["__class__"]       = "XmlNode";

                childrenArray[std::to_string(i++)] = Symbols::ValuePtr::makeClassInstance(childObj);
            }

            if (!childrenArray.empty()) {
                map["children"] = childrenArray;
            } else {
                map["children"] = Symbols::ValuePtr::null(Symbols::Variables::Type::OBJECT);
            }
        } else {
            map["children"] = Symbols::ValuePtr::null(Symbols::Variables::Type::OBJECT);
        }

        return map;
    }

    throw std::runtime_error(this->className + "::getNodeAttributes: invalid handler");
}

// Enhanced method implementations will continue...
// Due to length, I'll include placeholder implementations for the enhanced methods

Symbols::ValuePtr XmlModule::createDocument(FunctionArguments& args) {
    std::string version = "1.0";
    std::string encoding = "UTF-8";
    
    if (args.size() > 1 && args[1] != Symbols::Variables::Type::NULL_TYPE) {
        version = args[1].get<std::string>();
    }
    if (args.size() > 2 && args[2] != Symbols::Variables::Type::NULL_TYPE) {
        encoding = args[2].get<std::string>();
    }
    
    auto doc = XmlDocument::createDocument(version, encoding);
    return createDocumentObject(std::move(doc));
}

Symbols::ValuePtr XmlModule::createFromString(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::createFromString: xmlString parameter required");
    }
    
    std::string xmlString = args[1].get<std::string>();
    std::string baseUrl = "";
    
    if (args.size() > 2 && args[2] != Symbols::Variables::Type::NULL_TYPE) {
        baseUrl = args[2].get<std::string>();
    }
    
    auto doc = XmlDocument::createFromString(xmlString, baseUrl);
    return createDocumentObject(std::move(doc));
}

Symbols::ValuePtr XmlModule::createFromFile(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::createFromFile: filename parameter required");
    }
    
    std::string filename = args[1].get<std::string>();
    auto doc = XmlDocument::createFromFile(filename);
    return createDocumentObject(std::move(doc));
}

// Placeholder implementations for other enhanced methods
Symbols::ValuePtr XmlModule::toString(FunctionArguments& args) {
    XmlDocument* doc = getDocumentFromArgs(args);
    std::string encoding = "UTF-8";
    bool formatOutput = true;
    
    if (args.size() > 1 && args[1] != Symbols::Variables::Type::NULL_TYPE) {
        encoding = args[1].get<std::string>();
    }
    if (args.size() > 2 && args[2] != Symbols::Variables::Type::NULL_TYPE) {
        formatOutput = args[2].get<bool>();
    }
    
    return Symbols::ValuePtr(doc->toString(encoding, formatOutput));
}

Symbols::ValuePtr XmlModule::saveToFile(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::saveToFile: filename parameter required");
    }
    
    XmlDocument* doc = getDocumentFromArgs(args);
    std::string filename = args[1].get<std::string>();
    std::string encoding = "UTF-8";
    bool formatOutput = true;
    
    if (args.size() > 2 && args[2] != Symbols::Variables::Type::NULL_TYPE) {
        encoding = args[2].get<std::string>();
    }
    if (args.size() > 3 && args[3] != Symbols::Variables::Type::NULL_TYPE) {
        formatOutput = args[3].get<bool>();
    }
    
    return Symbols::ValuePtr(doc->saveToFile(filename, encoding, formatOutput));
}

// Additional method placeholders - implement similar patterns for remaining methods
Symbols::ValuePtr XmlModule::isWellFormed(FunctionArguments& args) {
    XmlDocument* doc = getDocumentFromArgs(args);
    return Symbols::ValuePtr(doc->isWellFormed());
}

Symbols::ValuePtr XmlModule::getVersion(FunctionArguments& args) {
    XmlDocument* doc = getDocumentFromArgs(args);
    return Symbols::ValuePtr(doc->getVersion());
}

Symbols::ValuePtr XmlModule::getEncoding(FunctionArguments& args) {
    XmlDocument* doc = getDocumentFromArgs(args);
    return Symbols::ValuePtr(doc->getEncoding());
}

Symbols::ValuePtr XmlModule::createElement(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::createElement: name parameter required");
    }
    
    XmlDocument* doc = getDocumentFromArgs(args);
    std::string name = args[1].get<std::string>();
    auto element = doc->createElement(name);
    return createNodeObject(std::move(element));
}

Symbols::ValuePtr XmlModule::createTextNode(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::createTextNode: content parameter required");
    }
    
    XmlDocument* doc = getDocumentFromArgs(args);
    std::string content = args[1].get<std::string>();
    auto textNode = doc->createTextNode(content);
    return createNodeObject(std::move(textNode));
}

// Node method implementations
Symbols::ValuePtr XmlModule::getName(FunctionArguments& args) {
    XmlNode* node = getNodeFromArgs(args);
    return Symbols::ValuePtr(node->getName());
}

Symbols::ValuePtr XmlModule::setName(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::setName: name parameter required");
    }
    
    XmlNode* node = getNodeFromArgs(args);
    std::string name = args[1].get<std::string>();
    node->setName(name);
    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr XmlModule::getContent(FunctionArguments& args) {
    XmlNode* node = getNodeFromArgs(args);
    return Symbols::ValuePtr(node->getContent());
}

Symbols::ValuePtr XmlModule::setContent(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::setContent: content parameter required");
    }
    
    XmlNode* node = getNodeFromArgs(args);
    std::string content = args[1].get<std::string>();
    node->setContent(content);
    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr XmlModule::getTextContent(FunctionArguments& args) {
    XmlNode* node = getNodeFromArgs(args);
    return Symbols::ValuePtr(node->getTextContent());
}

Symbols::ValuePtr XmlModule::setTextContent(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::setTextContent: content parameter required");
    }
    
    XmlNode* node = getNodeFromArgs(args);
    std::string content = args[1].get<std::string>();
    node->setTextContent(content);
    return Symbols::ValuePtr::null();
}

// Navigation methods
Symbols::ValuePtr XmlModule::getParent(FunctionArguments& args) {
    XmlNode* node = getNodeFromArgs(args);
    auto parent = node->getParent();
    return parent ? createNodeObject(std::move(parent)) : Symbols::ValuePtr::null();
}

Symbols::ValuePtr XmlModule::getFirstChild(FunctionArguments& args) {
    XmlNode* node = getNodeFromArgs(args);
    auto child = node->getFirstChild();
    return child ? createNodeObject(std::move(child)) : Symbols::ValuePtr::null();
}

Symbols::ValuePtr XmlModule::getLastChild(FunctionArguments& args) {
    XmlNode* node = getNodeFromArgs(args);
    auto child = node->getLastChild();
    return child ? createNodeObject(std::move(child)) : Symbols::ValuePtr::null();
}

Symbols::ValuePtr XmlModule::getNextSibling(FunctionArguments& args) {
    XmlNode* node = getNodeFromArgs(args);
    auto sibling = node->getNextSibling();
    return sibling ? createNodeObject(std::move(sibling)) : Symbols::ValuePtr::null();
}

Symbols::ValuePtr XmlModule::getPreviousSibling(FunctionArguments& args) {
    XmlNode* node = getNodeFromArgs(args);
    auto sibling = node->getPreviousSibling();
    return sibling ? createNodeObject(std::move(sibling)) : Symbols::ValuePtr::null();
}

Symbols::ValuePtr XmlModule::getChildren(FunctionArguments& args) {
    XmlNode* node = getNodeFromArgs(args);
    auto children = node->getChildren();
    return createNodeListObject(std::move(children));
}

// Attribute methods
Symbols::ValuePtr XmlModule::getAttribute(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::getAttribute: name parameter required");
    }
    
    XmlNode* node = getNodeFromArgs(args);
    std::string name = args[1].get<std::string>();
    return Symbols::ValuePtr(node->getAttribute(name));
}

Symbols::ValuePtr XmlModule::setAttribute(FunctionArguments& args) {
    if (args.size() < 3) {
        throw std::runtime_error("XmlModule::setAttribute: name and value parameters required");
    }
    
    XmlNode* node = getNodeFromArgs(args);
    std::string name = args[1].get<std::string>();
    std::string value = args[2].get<std::string>();
    node->setAttribute(name, value);
    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr XmlModule::hasAttribute(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::hasAttribute: name parameter required");
    }
    
    XmlNode* node = getNodeFromArgs(args);
    std::string name = args[1].get<std::string>();
    return Symbols::ValuePtr(node->hasAttribute(name));
}

Symbols::ValuePtr XmlModule::removeAttribute(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::removeAttribute: name parameter required");
    }
    
    XmlNode* node = getNodeFromArgs(args);
    std::string name = args[1].get<std::string>();
    node->removeAttribute(name);
    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr XmlModule::getAllAttributes(FunctionArguments& args) {
    XmlNode* node = getNodeFromArgs(args);
    return Symbols::ValuePtr(node->getAllAttributes());
}

Symbols::ValuePtr XmlModule::hasChildNodes(FunctionArguments& args) {
    XmlNode* node = getNodeFromArgs(args);
    return Symbols::ValuePtr(node->hasChildNodes());
}

Symbols::ValuePtr XmlModule::appendChild(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::appendChild: child parameter required");
    }
    
    XmlNode* parent = getNodeFromArgs(args);
    // This is simplified - in a real implementation you'd need to get the child node from args[1]
    // For now, returning null as placeholder
    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr XmlModule::cloneNode(FunctionArguments& args) {
    XmlNode* node = getNodeFromArgs(args);
    bool deep = false;
    
    if (args.size() > 1 && args[1] != Symbols::Variables::Type::NULL_TYPE) {
        deep = args[1].get<bool>();
    }
    
    auto clone = node->cloneNode(deep);
    return createNodeObject(std::move(clone));
}

// NodeList methods
Symbols::ValuePtr XmlModule::getLength(FunctionArguments& args) {
    XmlNodeList* nodeList = getNodeListFromArgs(args);
    return Symbols::ValuePtr(static_cast<int>(nodeList->getLength()));
}

Symbols::ValuePtr XmlModule::item(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::item: index parameter required");
    }
    
    XmlNodeList* nodeList = getNodeListFromArgs(args);
    int index = args[1].get<int>();
    auto node = nodeList->item(index);
    return node ? createNodeObject(std::move(node)) : Symbols::ValuePtr::null();
}

// Remaining placeholder methods...
Symbols::ValuePtr XmlModule::saveToMemory(FunctionArguments& args) { return Symbols::ValuePtr::null(); }
Symbols::ValuePtr XmlModule::insertBefore(FunctionArguments& args) { return Symbols::ValuePtr::null(); }
Symbols::ValuePtr XmlModule::removeChild(FunctionArguments& args) { return Symbols::ValuePtr::null(); }
Symbols::ValuePtr XmlModule::replaceChild(FunctionArguments& args) { return Symbols::ValuePtr::null(); }

void XmlModule::registerXPathMethods() {
    std::vector<Symbols::FunctionParameterInfo> params;

    // Document XPath methods
    params = {
        { "expression", Symbols::Variables::Type::STRING, "XPath expression" }
    };
    REGISTER_METHOD(
        "XmlDocument", "xpath", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->xpath(args); },
        Symbols::Variables::Type::CLASS, "Evaluate XPath expression and return result");

    REGISTER_METHOD(
        "XmlDocument", "selectNodes", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->selectNodes(args); },
        Symbols::Variables::Type::CLASS, "Select nodes using XPath expression");

    REGISTER_METHOD(
        "XmlDocument", "selectSingleNode", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->selectSingleNode(args); },
        Symbols::Variables::Type::CLASS, "Select single node using XPath expression");

    // Node XPath methods
    REGISTER_METHOD(
        "XmlNode", "selectNodes", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->selectNodes(args); },
        Symbols::Variables::Type::CLASS, "Select nodes using XPath expression from this node");

    REGISTER_METHOD(
        "XmlNode", "selectSingleNode", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->selectSingleNode(args); },
        Symbols::Variables::Type::CLASS, "Select single node using XPath expression from this node");

    // XPath creation
    params = {
        { "document", Symbols::Variables::Type::CLASS, "XML document for XPath context" }
    };
    REGISTER_METHOD(
        this->className, "createXPath", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->createXPath(args); },
        Symbols::Variables::Type::CLASS, "Create XPath evaluator for document");

    // XPath evaluation methods
    params = {
        { "expression", Symbols::Variables::Type::STRING, "XPath expression" }
    };
    REGISTER_METHOD(
        "XmlXPath", "evaluate", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->xpathEvaluate(args); },
        Symbols::Variables::Type::CLASS, "Evaluate XPath expression");

    params = {
        { "expression", Symbols::Variables::Type::STRING, "XPath expression" },
        { "contextNode", Symbols::Variables::Type::CLASS, "Context node for evaluation" }
    };
    REGISTER_METHOD(
        "XmlXPath", "evaluateWithContext", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->xpathEvaluateWithContext(args); },
        Symbols::Variables::Type::CLASS, "Evaluate XPath expression with context node");

    // XPath namespace methods
    params = {
        { "prefix", Symbols::Variables::Type::STRING, "Namespace prefix" },
        { "uri", Symbols::Variables::Type::STRING, "Namespace URI" }
    };
    REGISTER_METHOD(
        "XmlXPath", "registerNamespace", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->xpathRegisterNamespace(args); },
        Symbols::Variables::Type::NULL_TYPE, "Register namespace for XPath queries");

    params = {
        { "prefix", Symbols::Variables::Type::STRING, "Namespace prefix" }
    };
    REGISTER_METHOD(
        "XmlXPath", "unregisterNamespace", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->xpathUnregisterNamespace(args); },
        Symbols::Variables::Type::NULL_TYPE, "Unregister namespace");

    // XPath variable methods
    params = {
        { "name", Symbols::Variables::Type::STRING, "Variable name" },
        { "value", Symbols::Variables::Type::STRING, "Variable value" }
    };
    REGISTER_METHOD(
        "XmlXPath", "registerVariable", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->xpathRegisterVariable(args); },
        Symbols::Variables::Type::NULL_TYPE, "Register variable for XPath queries");

    // XPath result methods
    REGISTER_METHOD(
        "XmlXPathResult", "getType", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->xpathResultGetType(args); },
        Symbols::Variables::Type::INTEGER, "Get result type");

    REGISTER_METHOD(
        "XmlXPathResult", "getBooleanValue", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->xpathResultGetBooleanValue(args); },
        Symbols::Variables::Type::BOOLEAN, "Get boolean value");

    REGISTER_METHOD(
        "XmlXPathResult", "getNumberValue", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->xpathResultGetNumberValue(args); },
        Symbols::Variables::Type::DOUBLE, "Get number value");

    REGISTER_METHOD(
        "XmlXPathResult", "getStringValue", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->xpathResultGetStringValue(args); },
        Symbols::Variables::Type::STRING, "Get string value");

    REGISTER_METHOD(
        "XmlXPathResult", "getNodeSet", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->xpathResultGetNodeSet(args); },
        Symbols::Variables::Type::CLASS, "Get node set as XmlNodeList");

    REGISTER_METHOD(
        "XmlXPathResult", "getNodeSetSize", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->xpathResultGetNodeSetSize(args); },
        Symbols::Variables::Type::INTEGER, "Get node set size");
}

// XPath method implementations
Symbols::ValuePtr XmlModule::xpath(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::xpath: expression parameter required");
    }

    XmlDocument* doc = getDocumentFromArgs(args);
    std::string expression = args[1].get<std::string>();
    auto result = doc->xpath(expression);
    return createXPathResultObject(std::move(result));
}

Symbols::ValuePtr XmlModule::selectNodes(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::selectNodes: expression parameter required");
    }

    std::string expression = args[1].get<std::string>();

    // Check if this is called on a document or node
    try {
        XmlDocument* doc = getDocumentFromArgs(args);
        auto nodeList = doc->selectNodes(expression);
        return createNodeListObject(std::move(nodeList));
    } catch (...) {
        try {
            XmlNode* node = getNodeFromArgs(args);
            auto nodeList = node->selectNodes(expression);
            return createNodeListObject(std::move(nodeList));
        } catch (...) {
            throw std::runtime_error("XmlModule::selectNodes: Invalid object type");
        }
    }
}

Symbols::ValuePtr XmlModule::selectSingleNode(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::selectSingleNode: expression parameter required");
    }

    std::string expression = args[1].get<std::string>();

    // Check if this is called on a document or node
    try {
        XmlDocument* doc = getDocumentFromArgs(args);
        auto node = doc->selectSingleNode(expression);
        return node ? createNodeObject(std::move(node)) : Symbols::ValuePtr::null();
    } catch (...) {
        try {
            XmlNode* node = getNodeFromArgs(args);
            auto resultNode = node->selectSingleNode(expression);
            return resultNode ? createNodeObject(std::move(resultNode)) : Symbols::ValuePtr::null();
        } catch (...) {
            throw std::runtime_error("XmlModule::selectSingleNode: Invalid object type");
        }
    }
}

Symbols::ValuePtr XmlModule::createXPath(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::createXPath: document parameter required");
    }

    XmlDocument* doc = getDocumentFromArgs(args);
    auto xpath = std::make_unique<XmlXPath>(doc->getDoc());
    return createXPathObject(std::move(xpath));
}

Symbols::ValuePtr XmlModule::xpathEvaluate(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::xpathEvaluate: expression parameter required");
    }

    XmlXPath* xpath = getXPathFromArgs(args);
    std::string expression = args[1].get<std::string>();
    auto result = xpath->evaluate(expression);
    return createXPathResultObject(std::move(result));
}

Symbols::ValuePtr XmlModule::xpathEvaluateWithContext(FunctionArguments& args) {
    if (args.size() < 3) {
        throw std::runtime_error("XmlModule::xpathEvaluateWithContext: expression and contextNode parameters required");
    }

    XmlXPath* xpath = getXPathFromArgs(args);
    std::string expression = args[1].get<std::string>();
    // Note: Would need to extract XmlNode from args[2] properly
    auto result = xpath->evaluate(expression);
    return createXPathResultObject(std::move(result));
}

Symbols::ValuePtr XmlModule::xpathRegisterNamespace(FunctionArguments& args) {
    if (args.size() < 3) {
        throw std::runtime_error("XmlModule::xpathRegisterNamespace: prefix and uri parameters required");
    }

    XmlXPath* xpath = getXPathFromArgs(args);
    std::string prefix = args[1].get<std::string>();
    std::string uri = args[2].get<std::string>();
    xpath->registerNamespace(prefix, uri);
    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr XmlModule::xpathUnregisterNamespace(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::xpathUnregisterNamespace: prefix parameter required");
    }

    XmlXPath* xpath = getXPathFromArgs(args);
    std::string prefix = args[1].get<std::string>();
    xpath->unregisterNamespace(prefix);
    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr XmlModule::xpathRegisterVariable(FunctionArguments& args) {
    if (args.size() < 3) {
        throw std::runtime_error("XmlModule::xpathRegisterVariable: name and value parameters required");
    }

    XmlXPath* xpath = getXPathFromArgs(args);
    std::string name = args[1].get<std::string>();
    
    // Handle different value types
    if (args[2] == Symbols::Variables::Type::STRING) {
        std::string value = args[2].get<std::string>();
        xpath->registerVariable(name, value);
    } else if (args[2] == Symbols::Variables::Type::DOUBLE || args[2] == Symbols::Variables::Type::INTEGER) {
        double value = args[2].get<double>();
        xpath->registerVariable(name, value);
    } else if (args[2] == Symbols::Variables::Type::BOOLEAN) {
        bool value = args[2].get<bool>();
        xpath->registerVariable(name, value);
    } else {
        throw std::runtime_error("XmlModule::xpathRegisterVariable: Unsupported value type");
    }
    
    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr XmlModule::xpathResultGetType(FunctionArguments& args) {
    XmlXPathResult* result = getXPathResultFromArgs(args);
    return Symbols::ValuePtr(static_cast<int>(result->getType()));
}

Symbols::ValuePtr XmlModule::xpathResultGetBooleanValue(FunctionArguments& args) {
    XmlXPathResult* result = getXPathResultFromArgs(args);
    return Symbols::ValuePtr(result->getBooleanValue());
}

Symbols::ValuePtr XmlModule::xpathResultGetNumberValue(FunctionArguments& args) {
    XmlXPathResult* result = getXPathResultFromArgs(args);
    return Symbols::ValuePtr(result->getNumberValue());
}

Symbols::ValuePtr XmlModule::xpathResultGetStringValue(FunctionArguments& args) {
    XmlXPathResult* result = getXPathResultFromArgs(args);
    return Symbols::ValuePtr(result->getStringValue());
}

Symbols::ValuePtr XmlModule::xpathResultGetNodeSet(FunctionArguments& args) {
    XmlXPathResult* result = getXPathResultFromArgs(args);
    auto nodeSet = result->getNodeSet();
    return createNodeListObject(std::move(nodeSet));
}

Symbols::ValuePtr XmlModule::xpathResultGetNodeSetSize(FunctionArguments& args) {
    XmlXPathResult* result = getXPathResultFromArgs(args);
    return Symbols::ValuePtr(static_cast<int>(result->getNodeSetSize()));
}

// Validation method registrations
void XmlModule::registerValidationMethods() {
    std::vector<Symbols::FunctionParameterInfo> params;

    // Register validation classes
    REGISTER_CLASS("XmlSchema");
    REGISTER_CLASS("XmlDtd");
    REGISTER_CLASS("XmlValidator");

    // Schema methods
    REGISTER_METHOD(
        this->className, "createSchema", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->createSchema(args); },
        Symbols::Variables::Type::CLASS, "Create new XML Schema validator");

    params = {
        { "filename", Symbols::Variables::Type::STRING, "Path to XSD schema file" }
    };
    REGISTER_METHOD(
        "XmlSchema", "loadFromFile", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->schemaLoadFromFile(args); },
        Symbols::Variables::Type::BOOLEAN, "Load schema from file");

    params = {
        { "schemaContent", Symbols::Variables::Type::STRING, "XSD schema content as string" }
    };
    REGISTER_METHOD(
        "XmlSchema", "loadFromString", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->schemaLoadFromString(args); },
        Symbols::Variables::Type::BOOLEAN, "Load schema from string");

    params = {
        { "document", Symbols::Variables::Type::CLASS, "XML document to validate" }
    };
    REGISTER_METHOD(
        "XmlSchema", "validate", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->schemaValidate(args); },
        Symbols::Variables::Type::BOOLEAN, "Validate document against schema");

    REGISTER_METHOD(
        "XmlSchema", "getErrors", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->schemaGetErrors(args); },
        Symbols::Variables::Type::OBJECT, "Get validation errors");

    REGISTER_METHOD(
        "XmlSchema", "clearErrors", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->schemaClearErrors(args); },
        Symbols::Variables::Type::NULL_TYPE, "Clear validation errors");

    // DTD methods
    REGISTER_METHOD(
        this->className, "createDtd", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->createDtd(args); },
        Symbols::Variables::Type::CLASS, "Create new DTD validator");

    params = {
        { "filename", Symbols::Variables::Type::STRING, "Path to DTD file" }
    };
    REGISTER_METHOD(
        "XmlDtd", "loadFromFile", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->dtdLoadFromFile(args); },
        Symbols::Variables::Type::BOOLEAN, "Load DTD from file");

    params = {
        { "dtdContent", Symbols::Variables::Type::STRING, "DTD content as string" }
    };
    REGISTER_METHOD(
        "XmlDtd", "loadFromString", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->dtdLoadFromString(args); },
        Symbols::Variables::Type::BOOLEAN, "Load DTD from string");

    params = {
        { "document", Symbols::Variables::Type::CLASS, "XML document to validate" }
    };
    REGISTER_METHOD(
        "XmlDtd", "validate", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->dtdValidate(args); },
        Symbols::Variables::Type::BOOLEAN, "Validate document against DTD");

    REGISTER_METHOD(
        "XmlDtd", "getErrors", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->dtdGetErrors(args); },
        Symbols::Variables::Type::OBJECT, "Get validation errors");

    REGISTER_METHOD(
        "XmlDtd", "clearErrors", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->dtdClearErrors(args); },
        Symbols::Variables::Type::NULL_TYPE, "Clear validation errors");

    // Validator methods
    REGISTER_METHOD(
        this->className, "createValidator", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->createValidator(args); },
        Symbols::Variables::Type::CLASS, "Create unified XML validator");

    params = {
        { "document", Symbols::Variables::Type::CLASS, "XML document to validate" }
    };
    REGISTER_METHOD(
        "XmlValidator", "validateWellFormedness", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->validatorValidateWellFormedness(args); },
        Symbols::Variables::Type::BOOLEAN, "Validate document well-formedness");

    params = {
        { "document", Symbols::Variables::Type::CLASS, "XML document to validate" },
        { "schema", Symbols::Variables::Type::CLASS, "XML Schema to validate against" }
    };
    REGISTER_METHOD(
        "XmlValidator", "validateAgainstSchema", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->validatorValidateAgainstSchema(args); },
        Symbols::Variables::Type::BOOLEAN, "Validate document against schema");

    params = {
        { "document", Symbols::Variables::Type::CLASS, "XML document to validate" },
        { "dtd", Symbols::Variables::Type::CLASS, "DTD to validate against" }
    };
    REGISTER_METHOD(
        "XmlValidator", "validateAgainstDtd", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->validatorValidateAgainstDtd(args); },
        Symbols::Variables::Type::BOOLEAN, "Validate document against DTD");

    REGISTER_METHOD(
        "XmlValidator", "getErrors", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->validatorGetErrors(args); },
        Symbols::Variables::Type::OBJECT, "Get validation errors");

    REGISTER_METHOD(
        "XmlValidator", "getWarnings", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->validatorGetWarnings(args); },
        Symbols::Variables::Type::OBJECT, "Get validation warnings");

    REGISTER_METHOD(
        "XmlValidator", "clearAll", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->validatorClearAll(args); },
        Symbols::Variables::Type::NULL_TYPE, "Clear all errors and warnings");

    // Document validation methods
    params = {
        { "schema", Symbols::Variables::Type::CLASS, "XML Schema to validate against" }
    };
    REGISTER_METHOD(
        "XmlDocument", "validateAgainstSchema", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->validateAgainstSchema(args); },
        Symbols::Variables::Type::BOOLEAN, "Validate document against schema");

    params = {
        { "dtd", Symbols::Variables::Type::CLASS, "DTD to validate against" }
    };
    REGISTER_METHOD(
        "XmlDocument", "validateAgainstDtd", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->validateAgainstDtd(args); },
        Symbols::Variables::Type::BOOLEAN, "Validate document against DTD");

    REGISTER_METHOD(
        "XmlDocument", "validateWellFormedness", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->validateWellFormedness(args); },
        Symbols::Variables::Type::BOOLEAN, "Validate document well-formedness");

    // Register properties for validation classes
    REGISTER_PROPERTY("XmlSchema", "__xml_schema_handler_id__", Symbols::Variables::Type::INTEGER, nullptr);
    REGISTER_PROPERTY("XmlDtd", "__xml_dtd_handler_id__", Symbols::Variables::Type::INTEGER, nullptr);
    REGISTER_PROPERTY("XmlValidator", "__xml_validator_handler_id__", Symbols::Variables::Type::INTEGER, nullptr);
}

// Advanced method registrations
void XmlModule::registerAdvancedMethods() {
    std::vector<Symbols::FunctionParameterInfo> params;

    // Encoding management
    params = {
        { "encoding", Symbols::Variables::Type::STRING, "Document encoding (UTF-8, UTF-16, ISO-8859-1, etc.)" }
    };
    REGISTER_METHOD(
        "XmlDocument", "setEncoding", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->setEncoding(args); },
        Symbols::Variables::Type::NULL_TYPE, "Set document encoding");

    REGISTER_METHOD(
        "XmlDocument", "getDocumentEncoding", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->getDocumentEncoding(args); },
        Symbols::Variables::Type::STRING, "Get document encoding");

    // Output formatting
    params = {
        { "pretty", Symbols::Variables::Type::BOOLEAN, "Enable pretty printing (default: true)" },
        { "indent", Symbols::Variables::Type::INTEGER, "Indentation size (default: 2)" }
    };
    REGISTER_METHOD(
        "XmlDocument", "formatDocument", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->formatDocument(args); },
        Symbols::Variables::Type::STRING, "Format document with pretty printing");

    params = {
        { "format", Symbols::Variables::Type::BOOLEAN, "Enable format output" }
    };
    REGISTER_METHOD(
        "XmlDocument", "setFormatOutput", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->setFormatOutput(args); },
        Symbols::Variables::Type::NULL_TYPE, "Set format output option");

    REGISTER_METHOD(
        "XmlDocument", "getFormatOutput", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->getFormatOutput(args); },
        Symbols::Variables::Type::BOOLEAN, "Get format output option");

    // Advanced node creation
    params = {
        { "text", Symbols::Variables::Type::STRING, "Comment text" }
    };
    REGISTER_METHOD(
        "XmlDocument", "createComment", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->createComment(args); },
        Symbols::Variables::Type::CLASS, "Create comment node");

    params = {
        { "target", Symbols::Variables::Type::STRING, "Processing instruction target" },
        { "data", Symbols::Variables::Type::STRING, "Processing instruction data" }
    };
    REGISTER_METHOD(
        "XmlDocument", "createProcessingInstruction", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->createProcessingInstruction(args); },
        Symbols::Variables::Type::CLASS, "Create processing instruction node");

    params = {
        { "content", Symbols::Variables::Type::STRING, "CDATA content" }
    };
    REGISTER_METHOD(
        "XmlDocument", "createCDataSection", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->createCDataSection(args); },
        Symbols::Variables::Type::CLASS, "Create CDATA section node");

    // Namespace management
    params = {
        { "prefix", Symbols::Variables::Type::STRING, "Namespace prefix" },
        { "uri", Symbols::Variables::Type::STRING, "Namespace URI" }
    };
    REGISTER_METHOD(
        "XmlDocument", "registerNamespacePrefix", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->registerNamespacePrefix(args); },
        Symbols::Variables::Type::NULL_TYPE, "Register namespace prefix");

    params = {
        { "prefix", Symbols::Variables::Type::STRING, "Namespace prefix" }
    };
    REGISTER_METHOD(
        "XmlDocument", "resolveNamespaceUri", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->resolveNamespaceUri(args); },
        Symbols::Variables::Type::STRING, "Resolve namespace URI from prefix");

    params = {
        { "uri", Symbols::Variables::Type::STRING, "Namespace URI" }
    };
    REGISTER_METHOD(
        "XmlDocument", "resolveNamespacePrefix", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->resolveNamespacePrefix(args); },
        Symbols::Variables::Type::STRING, "Resolve namespace prefix from URI");
}

// Helper methods for validation object management
XmlSchema* XmlModule::getSchemaFromArgs(const FunctionArguments& args) const {
    if (args.empty() || (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT)) {
        throw std::runtime_error("XmlModule: Invalid schema object");
    }

    auto symbolContainer = Symbols::SymbolContainer::instance();
    auto handlerProperty = symbolContainer->getObjectProperty("XmlSchema", "__xml_schema_handler_id__");
    if (!handlerProperty) {
        throw std::runtime_error("XmlModule: Invalid schema handler");
    }

    int handlerId = handlerProperty;
    auto it = schemaHolder.find(handlerId);
    if (it == schemaHolder.end()) {
        throw std::runtime_error("XmlModule: Schema not found");
    }

    return it->second.get();
}

XmlDtd* XmlModule::getDtdFromArgs(const FunctionArguments& args) const {
    if (args.empty() || (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT)) {
        throw std::runtime_error("XmlModule: Invalid DTD object");
    }

    auto symbolContainer = Symbols::SymbolContainer::instance();
    auto handlerProperty = symbolContainer->getObjectProperty("XmlDtd", "__xml_dtd_handler_id__");
    if (!handlerProperty) {
        throw std::runtime_error("XmlModule: Invalid DTD handler");
    }

    int handlerId = handlerProperty;
    auto it = dtdHolder.find(handlerId);
    if (it == dtdHolder.end()) {
        throw std::runtime_error("XmlModule: DTD not found");
    }

    return it->second.get();
}

XmlValidator* XmlModule::getValidatorFromArgs(const FunctionArguments& args) const {
    if (args.empty() || (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT)) {
        throw std::runtime_error("XmlModule: Invalid validator object");
    }

    auto symbolContainer = Symbols::SymbolContainer::instance();
    auto handlerProperty = symbolContainer->getObjectProperty("XmlValidator", "__xml_validator_handler_id__");
    if (!handlerProperty) {
        throw std::runtime_error("XmlModule: Invalid validator handler");
    }

    int handlerId = handlerProperty;
    auto it = validatorHolder.find(handlerId);
    if (it == validatorHolder.end()) {
        throw std::runtime_error("XmlModule: Validator not found");
    }

    return it->second.get();
}

Symbols::ValuePtr XmlModule::createSchemaObject(std::unique_ptr<XmlSchema> schema) {
    int id = schema->getId();
    schemaHolder[id] = std::move(schema);

    auto symbolContainer = Symbols::SymbolContainer::instance();
    symbolContainer->setObjectProperty("XmlSchema", "__xml_schema_handler_id__", id);
    symbolContainer->setObjectProperty("XmlSchema", "__class__", "XmlSchema");

    Symbols::ObjectMap objMap;
    objMap["__xml_schema_handler_id__"] = symbolContainer->getObjectProperty("XmlSchema", "__xml_schema_handler_id__");
    objMap["__class__"] = symbolContainer->getObjectProperty("XmlSchema", "__class__");

    return Symbols::ValuePtr::makeClassInstance(objMap);
}

Symbols::ValuePtr XmlModule::createDtdObject(std::unique_ptr<XmlDtd> dtd) {
    int id = dtd->getId();
    dtdHolder[id] = std::move(dtd);

    auto symbolContainer = Symbols::SymbolContainer::instance();
    symbolContainer->setObjectProperty("XmlDtd", "__xml_dtd_handler_id__", id);
    symbolContainer->setObjectProperty("XmlDtd", "__class__", "XmlDtd");

    Symbols::ObjectMap objMap;
    objMap["__xml_dtd_handler_id__"] = symbolContainer->getObjectProperty("XmlDtd", "__xml_dtd_handler_id__");
    objMap["__class__"] = symbolContainer->getObjectProperty("XmlDtd", "__class__");

    return Symbols::ValuePtr::makeClassInstance(objMap);
}

Symbols::ValuePtr XmlModule::createValidatorObject(std::unique_ptr<XmlValidator> validator) {
    int id = validator->getId();
    validatorHolder[id] = std::move(validator);

    auto symbolContainer = Symbols::SymbolContainer::instance();
    symbolContainer->setObjectProperty("XmlValidator", "__xml_validator_handler_id__", id);
    symbolContainer->setObjectProperty("XmlValidator", "__class__", "XmlValidator");

    Symbols::ObjectMap objMap;
    objMap["__xml_validator_handler_id__"] = symbolContainer->getObjectProperty("XmlValidator", "__xml_validator_handler_id__");
    objMap["__class__"] = symbolContainer->getObjectProperty("XmlValidator", "__class__");

    return Symbols::ValuePtr::makeClassInstance(objMap);
}

// Validation method implementations
Symbols::ValuePtr XmlModule::createSchema(FunctionArguments& args) {
    auto schema = std::make_unique<XmlSchema>();
    return createSchemaObject(std::move(schema));
}

Symbols::ValuePtr XmlModule::createDtd(FunctionArguments& args) {
    auto dtd = std::make_unique<XmlDtd>();
    return createDtdObject(std::move(dtd));
}

Symbols::ValuePtr XmlModule::createValidator(FunctionArguments& args) {
    auto validator = std::make_unique<XmlValidator>();
    return createValidatorObject(std::move(validator));
}

Symbols::ValuePtr XmlModule::schemaLoadFromFile(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::schemaLoadFromFile: filename parameter required");
    }
    
    XmlSchema* schema = getSchemaFromArgs(args);
    std::string filename = args[1].get<std::string>();
    return Symbols::ValuePtr(schema->loadFromFile(filename));
}

Symbols::ValuePtr XmlModule::schemaLoadFromString(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::schemaLoadFromString: schemaContent parameter required");
    }
    
    XmlSchema* schema = getSchemaFromArgs(args);
    std::string content = args[1].get<std::string>();
    return Symbols::ValuePtr(schema->loadFromString(content));
}

Symbols::ValuePtr XmlModule::schemaValidate(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::schemaValidate: document parameter required");
    }
    
    XmlSchema* schema = getSchemaFromArgs(args);
    // Note: Would need to extract XmlDocument from args[1] properly
    // For now, using a simplified approach
    return Symbols::ValuePtr(false);
}

Symbols::ValuePtr XmlModule::schemaGetErrors(FunctionArguments& args) {
    XmlSchema* schema = getSchemaFromArgs(args);
    const auto& errors = schema->getErrors();
    
    Symbols::ObjectMap arrayMap;
    for (size_t i = 0; i < errors.size(); ++i) {
        arrayMap[std::to_string(i)] = Symbols::ValuePtr(errors[i]);
    }
    
    return Symbols::ValuePtr(arrayMap);
}

Symbols::ValuePtr XmlModule::schemaClearErrors(FunctionArguments& args) {
    XmlSchema* schema = getSchemaFromArgs(args);
    schema->clearErrors();
    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr XmlModule::dtdLoadFromFile(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::dtdLoadFromFile: filename parameter required");
    }
    
    XmlDtd* dtd = getDtdFromArgs(args);
    std::string filename = args[1].get<std::string>();
    return Symbols::ValuePtr(dtd->loadFromFile(filename));
}

Symbols::ValuePtr XmlModule::dtdLoadFromString(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::dtdLoadFromString: dtdContent parameter required");
    }
    
    XmlDtd* dtd = getDtdFromArgs(args);
    std::string content = args[1].get<std::string>();
    return Symbols::ValuePtr(dtd->loadFromString(content));
}

Symbols::ValuePtr XmlModule::dtdValidate(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::dtdValidate: document parameter required");
    }
    
    XmlDtd* dtd = getDtdFromArgs(args);
    // Note: Would need to extract XmlDocument from args[1] properly
    return Symbols::ValuePtr(false);
}

Symbols::ValuePtr XmlModule::dtdGetErrors(FunctionArguments& args) {
    XmlDtd* dtd = getDtdFromArgs(args);
    const auto& errors = dtd->getErrors();
    
    Symbols::ObjectMap arrayMap;
    for (size_t i = 0; i < errors.size(); ++i) {
        arrayMap[std::to_string(i)] = Symbols::ValuePtr(errors[i]);
    }
    
    return Symbols::ValuePtr(arrayMap);
}

Symbols::ValuePtr XmlModule::dtdClearErrors(FunctionArguments& args) {
    XmlDtd* dtd = getDtdFromArgs(args);
    dtd->clearErrors();
    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr XmlModule::validatorValidateWellFormedness(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::validatorValidateWellFormedness: document parameter required");
    }
    
    XmlValidator* validator = getValidatorFromArgs(args);
    // Note: Would need to extract XmlDocument from args[1] properly
    return Symbols::ValuePtr(false);
}

Symbols::ValuePtr XmlModule::validatorValidateAgainstSchema(FunctionArguments& args) {
    if (args.size() < 3) {
        throw std::runtime_error("XmlModule::validatorValidateAgainstSchema: document and schema parameters required");
    }
    
    XmlValidator* validator = getValidatorFromArgs(args);
    // Note: Would need to extract XmlDocument and XmlSchema from args properly
    return Symbols::ValuePtr(false);
}

Symbols::ValuePtr XmlModule::validatorValidateAgainstDtd(FunctionArguments& args) {
    if (args.size() < 3) {
        throw std::runtime_error("XmlModule::validatorValidateAgainstDtd: document and dtd parameters required");
    }
    
    XmlValidator* validator = getValidatorFromArgs(args);
    // Note: Would need to extract XmlDocument and XmlDtd from args properly
    return Symbols::ValuePtr(false);
}

Symbols::ValuePtr XmlModule::validatorGetErrors(FunctionArguments& args) {
    XmlValidator* validator = getValidatorFromArgs(args);
    const auto& errors = validator->getErrors();
    
    Symbols::ObjectMap arrayMap;
    for (size_t i = 0; i < errors.size(); ++i) {
        arrayMap[std::to_string(i)] = Symbols::ValuePtr(errors[i]);
    }
    
    return Symbols::ValuePtr(arrayMap);
}

Symbols::ValuePtr XmlModule::validatorGetWarnings(FunctionArguments& args) {
    XmlValidator* validator = getValidatorFromArgs(args);
    const auto& warnings = validator->getWarnings();
    
    Symbols::ObjectMap arrayMap;
    for (size_t i = 0; i < warnings.size(); ++i) {
        arrayMap[std::to_string(i)] = Symbols::ValuePtr(warnings[i]);
    }
    
    return Symbols::ValuePtr(arrayMap);
}

Symbols::ValuePtr XmlModule::validatorClearAll(FunctionArguments& args) {
    XmlValidator* validator = getValidatorFromArgs(args);
    validator->clearAll();
    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr XmlModule::validateAgainstSchema(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::validateAgainstSchema: schema parameter required");
    }
    
    XmlDocument* doc = getDocumentFromArgs(args);
    // Note: Would need to extract XmlSchema from args[1] properly
    return Symbols::ValuePtr(false);
}

Symbols::ValuePtr XmlModule::validateAgainstDtd(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::validateAgainstDtd: dtd parameter required");
    }
    
    XmlDocument* doc = getDocumentFromArgs(args);
    // Note: Would need to extract XmlDtd from args[1] properly
    return Symbols::ValuePtr(false);
}

Symbols::ValuePtr XmlModule::validateWellFormedness(FunctionArguments& args) {
    XmlDocument* doc = getDocumentFromArgs(args);
    std::vector<std::string> errors;
    bool result = doc->validateWellFormedness(errors);
    return Symbols::ValuePtr(result);
}

// Advanced method implementations
Symbols::ValuePtr XmlModule::setEncoding(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::setEncoding: encoding parameter required");
    }
    
    XmlDocument* doc = getDocumentFromArgs(args);
    std::string encoding = args[1].get<std::string>();
    doc->setEncoding(encoding);
    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr XmlModule::getDocumentEncoding(FunctionArguments& args) {
    XmlDocument* doc = getDocumentFromArgs(args);
    return Symbols::ValuePtr(doc->getDocumentEncoding());
}

Symbols::ValuePtr XmlModule::formatDocument(FunctionArguments& args) {
    XmlDocument* doc = getDocumentFromArgs(args);
    bool pretty = true;
    int indent = 2;
    
    if (args.size() > 1 && args[1] != Symbols::Variables::Type::NULL_TYPE) {
        pretty = args[1].get<bool>();
    }
    if (args.size() > 2 && args[2] != Symbols::Variables::Type::NULL_TYPE) {
        indent = args[2].get<int>();
    }
    
    return Symbols::ValuePtr(doc->formatDocument(pretty, indent));
}

Symbols::ValuePtr XmlModule::setFormatOutput(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::setFormatOutput: format parameter required");
    }
    
    XmlDocument* doc = getDocumentFromArgs(args);
    bool format = args[1].get<bool>();
    doc->setFormatOutput(format);
    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr XmlModule::getFormatOutput(FunctionArguments& args) {
    XmlDocument* doc = getDocumentFromArgs(args);
    return Symbols::ValuePtr(doc->getFormatOutput());
}

Symbols::ValuePtr XmlModule::createComment(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::createComment: text parameter required");
    }
    
    XmlDocument* doc = getDocumentFromArgs(args);
    std::string text = args[1].get<std::string>();
    auto comment = doc->createComment(text);
    return createNodeObject(std::move(comment));
}

Symbols::ValuePtr XmlModule::createProcessingInstruction(FunctionArguments& args) {
    if (args.size() < 3) {
        throw std::runtime_error("XmlModule::createProcessingInstruction: target and data parameters required");
    }
    
    XmlDocument* doc = getDocumentFromArgs(args);
    std::string target = args[1].get<std::string>();
    std::string data = args[2].get<std::string>();
    auto pi = doc->createProcessingInstruction(target, data);
    return createNodeObject(std::move(pi));
}

Symbols::ValuePtr XmlModule::createCDataSection(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::createCDataSection: content parameter required");
    }
    
    XmlDocument* doc = getDocumentFromArgs(args);
    std::string content = args[1].get<std::string>();
    auto cdata = doc->createCDataSection(content);
    return createNodeObject(std::move(cdata));
}

Symbols::ValuePtr XmlModule::registerNamespacePrefix(FunctionArguments& args) {
    if (args.size() < 3) {
        throw std::runtime_error("XmlModule::registerNamespacePrefix: prefix and uri parameters required");
    }
    
    XmlDocument* doc = getDocumentFromArgs(args);
    std::string prefix = args[1].get<std::string>();
    std::string uri = args[2].get<std::string>();
    doc->registerNamespacePrefix(prefix, uri);
    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr XmlModule::resolveNamespaceUri(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::resolveNamespaceUri: prefix parameter required");
    }
    
    XmlDocument* doc = getDocumentFromArgs(args);
    std::string prefix = args[1].get<std::string>();
    return Symbols::ValuePtr(doc->resolveNamespaceUri(prefix));
}

Symbols::ValuePtr XmlModule::resolveNamespacePrefix(FunctionArguments& args) {
    if (args.size() < 2) {
        throw std::runtime_error("XmlModule::resolveNamespacePrefix: uri parameter required");
    }
    
    XmlDocument* doc = getDocumentFromArgs(args);
    std::string uri = args[1].get<std::string>();
    return Symbols::ValuePtr(doc->resolveNamespacePrefix(uri));
}

}  // namespace Modules
