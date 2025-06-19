#ifndef XML_MODULE_HPP
#define XML_MODULE_HPP

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlsave.h>
#include <libxml/valid.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlschemas.h>
#include <libxml/relaxng.h>
#include <libxml/xmlIO.h>
#include <libxml/encoding.h>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <memory>

#include "Modules/BaseModule.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/RegistrationMacros.hpp"

namespace Modules {

// Forward declarations
class XmlDocument;
class XmlNode;
class XmlNodeList;
class XmlXPath;
class XmlXPathResult;
class XmlSchema;
class XmlDtd;
class XmlValidator;

// Resource management for libxml2 objects
inline static std::unordered_map<int, xmlDocPtr>  docHolder;
inline static std::unordered_map<int, xmlNodePtr> nodeHolder;
inline static std::unordered_map<int, std::unique_ptr<XmlDocument>> documentHolder;
inline static std::unordered_map<int, std::unique_ptr<XmlNode>> nodeObjectHolder;
inline static std::unordered_map<int, std::unique_ptr<XmlNodeList>> nodeListHolder;
inline static std::unordered_map<int, std::unique_ptr<XmlXPath>> xpathHolder;
inline static std::unordered_map<int, std::unique_ptr<XmlXPathResult>> xpathResultHolder;
inline static std::unordered_map<int, std::unique_ptr<XmlSchema>> schemaHolder;
inline static std::unordered_map<int, std::unique_ptr<XmlDtd>> dtdHolder;
inline static std::unordered_map<int, std::unique_ptr<XmlValidator>> validatorHolder;
static int nextDoc = 1;

/**
 * @brief RAII wrapper for XML documents with enhanced functionality
 */
class XmlDocument {
  private:
    xmlDocPtr doc_;
    int id_;
    bool ownsDocument_;

  public:
    XmlDocument(xmlDocPtr doc, bool takeOwnership = true);
    ~XmlDocument();

    // Document creation methods
    static std::unique_ptr<XmlDocument> createDocument(const std::string& version = "1.0", const std::string& encoding = "UTF-8");
    static std::unique_ptr<XmlDocument> createFromString(const std::string& xmlString, const std::string& baseUrl = "");
    static std::unique_ptr<XmlDocument> createFromFile(const std::string& filename);

    // Document serialization methods
    std::string toString(const std::string& encoding = "UTF-8", bool formatOutput = true) const;
    bool saveToFile(const std::string& filename, const std::string& encoding = "UTF-8", bool formatOutput = true) const;
    std::string saveToMemory(const std::string& encoding = "UTF-8", bool formatOutput = true) const;

    // Document properties and validation
    std::string getVersion() const;
    std::string getEncoding() const;
    bool isWellFormed() const;

    // Node access
    std::unique_ptr<XmlNode> getRootElement() const;
    std::unique_ptr<XmlNode> createElement(const std::string& name) const;
    std::unique_ptr<XmlNode> createTextNode(const std::string& content) const;

    // XPath support
    std::unique_ptr<XmlXPathResult> xpath(const std::string& expression) const;
    std::unique_ptr<XmlNodeList> selectNodes(const std::string& expression) const;
    std::unique_ptr<XmlNode> selectSingleNode(const std::string& expression) const;

    // Validation methods
    bool validateAgainstSchema(XmlSchema* schema) const;
    bool validateAgainstDtd(XmlDtd* dtd) const;
    bool validateWellFormedness(std::vector<std::string>& errors) const;

    // Encoding management
    void setEncoding(const std::string& encoding);
    std::string getDocumentEncoding() const;
    
    // Output formatting
    void setFormatOutput(bool format);
    bool getFormatOutput() const;
    std::string formatDocument(bool pretty = true, int indent = 2) const;

    // Advanced node creation
    std::unique_ptr<XmlNode> createComment(const std::string& text) const;
    std::unique_ptr<XmlNode> createProcessingInstruction(const std::string& target, const std::string& data) const;
    std::unique_ptr<XmlNode> createCDataSection(const std::string& content) const;

    // Namespace management
    void registerNamespacePrefix(const std::string& prefix, const std::string& uri);
    std::string resolveNamespaceUri(const std::string& prefix) const;
    std::string resolveNamespacePrefix(const std::string& uri) const;

    // Internal access
    xmlDocPtr getDoc() const { return doc_; }
    int getId() const { return id_; }
};

/**
 * @brief Enhanced XML node wrapper with full DOM manipulation
 */
class XmlNode {
  private:
    xmlNodePtr node_;
    int id_;
    bool ownsNode_;

  public:
    XmlNode(xmlNodePtr node, bool takeOwnership = false);
    ~XmlNode();

    // Node properties
    std::string getName() const;
    std::string getType() const;
    std::string getContent() const;
    void setContent(const std::string& content);
    void setName(const std::string& name);

    // Navigation
    std::unique_ptr<XmlNode> getParent() const;
    std::unique_ptr<XmlNode> getFirstChild() const;
    std::unique_ptr<XmlNode> getLastChild() const;
    std::unique_ptr<XmlNode> getNextSibling() const;
    std::unique_ptr<XmlNode> getPreviousSibling() const;
    std::unique_ptr<XmlNodeList> getChildren() const;

    // Node manipulation
    std::unique_ptr<XmlNode> appendChild(XmlNode* child);
    std::unique_ptr<XmlNode> insertBefore(XmlNode* newNode, XmlNode* refNode);
    std::unique_ptr<XmlNode> removeChild(XmlNode* child);
    std::unique_ptr<XmlNode> replaceChild(XmlNode* newNode, XmlNode* oldNode);
    std::unique_ptr<XmlNode> cloneNode(bool deep = false) const;

    // Attribute management
    std::string getAttribute(const std::string& name) const;
    void setAttribute(const std::string& name, const std::string& value);
    bool hasAttribute(const std::string& name) const;
    void removeAttribute(const std::string& name);
    Symbols::ObjectMap getAllAttributes() const;

    // Utility methods
    bool hasChildNodes() const;
    std::string getTextContent() const;
    void setTextContent(const std::string& content);

    // XPath support
    std::unique_ptr<XmlNodeList> selectNodes(const std::string& expression) const;
    std::unique_ptr<XmlNode> selectSingleNode(const std::string& expression) const;

    // Internal access
    xmlNodePtr getNode() const { return node_; }
    int getId() const { return id_; }
};

/**
 * @brief Collection management for XML nodes
 */
class XmlNodeList {
  private:
    std::vector<xmlNodePtr> nodes_;
    int id_;

  public:
    XmlNodeList();
    explicit XmlNodeList(xmlNodePtr startNode);
    ~XmlNodeList();

    size_t getLength() const;
    std::unique_ptr<XmlNode> item(size_t index) const;
    void addNode(xmlNodePtr node);
    void clear();

    int getId() const { return id_; }
};

/**
 * @brief XPath result wrapper for handling different result types
 */
class XmlXPathResult {
  public:
    enum ResultType {
        XPATH_UNDEFINED = 0,
        XPATH_NODESET = 1,
        XPATH_BOOLEAN = 2,
        XPATH_NUMBER = 3,
        XPATH_STRING = 4
    };

  private:
    xmlXPathObjectPtr result_;
    int id_;
    ResultType type_;

  public:
    XmlXPathResult(xmlXPathObjectPtr result);
    ~XmlXPathResult();

    // Result type and access
    ResultType getType() const;
    bool isNodeSet() const;
    bool isBoolean() const;
    bool isNumber() const;
    bool isString() const;

    // Value access methods
    bool getBooleanValue() const;
    double getNumberValue() const;
    std::string getStringValue() const;
    std::unique_ptr<XmlNodeList> getNodeSet() const;
    size_t getNodeSetSize() const;

    // Internal access
    xmlXPathObjectPtr getResult() const { return result_; }
    int getId() const { return id_; }
};

/**
 * @brief XPath expression compiler and evaluator
 */
class XmlXPath {
  private:
    xmlXPathContextPtr context_;
    int id_;
    xmlDocPtr doc_;

  public:
    XmlXPath(xmlDocPtr doc);
    ~XmlXPath();

    // XPath compilation and evaluation
    std::unique_ptr<XmlXPathResult> evaluate(const std::string& expression);
    std::unique_ptr<XmlXPathResult> evaluate(const std::string& expression, XmlNode* contextNode);
    
    // Namespace management
    void registerNamespace(const std::string& prefix, const std::string& uri);
    void unregisterNamespace(const std::string& prefix);
    
    // Variable registration (for advanced use)
    void registerVariable(const std::string& name, const std::string& value);
    void registerVariable(const std::string& name, double value);
    void registerVariable(const std::string& name, bool value);

    // Internal access
    xmlXPathContextPtr getContext() const { return context_; }
    int getId() const { return id_; }
};

/**
 * @brief XML Schema (XSD) validation wrapper
 */
class XmlSchema {
  private:
    xmlSchemaPtr schema_;
    xmlSchemaParserCtxtPtr parserCtxt_;
    xmlSchemaValidCtxtPtr validCtxt_;
    int id_;
    std::vector<std::string> errors_;

  public:
    XmlSchema();
    ~XmlSchema();

    // Schema loading
    bool loadFromFile(const std::string& filename);
    bool loadFromString(const std::string& schemaContent);
    bool loadFromMemory(const char* buffer, int size);

    // Validation
    bool validate(XmlDocument* document);
    bool validate(xmlDocPtr doc);

    // Error handling
    const std::vector<std::string>& getErrors() const;
    void clearErrors();
    std::string getLastError() const;

    // Internal access
    xmlSchemaPtr getSchema() const { return schema_; }
    int getId() const { return id_; }
};

/**
 * @brief DTD validation wrapper
 */
class XmlDtd {
  private:
    xmlDtdPtr dtd_;
    xmlValidCtxtPtr validCtxt_;
    int id_;
    std::vector<std::string> errors_;

  public:
    XmlDtd();
    ~XmlDtd();

    // DTD loading
    bool loadFromFile(const std::string& filename);
    bool loadFromString(const std::string& dtdContent);
    bool loadFromMemory(const char* buffer, int size);

    // Validation
    bool validate(XmlDocument* document);
    bool validate(xmlDocPtr doc);

    // Error handling
    const std::vector<std::string>& getErrors() const;
    void clearErrors();
    std::string getLastError() const;

    // Internal access
    xmlDtdPtr getDtd() const { return dtd_; }
    int getId() const { return id_; }
};

/**
 * @brief Unified validation interface
 */
class XmlValidator {
  private:
    int id_;
    std::vector<std::string> errors_;
    std::vector<std::string> warnings_;

  public:
    XmlValidator();
    ~XmlValidator();

    // Validation methods
    bool validateWellFormedness(XmlDocument* document);
    bool validateWellFormedness(const std::string& xmlContent);
    bool validateAgainstSchema(XmlDocument* document, XmlSchema* schema);
    bool validateAgainstDtd(XmlDocument* document, XmlDtd* dtd);

    // Error and warning handling
    const std::vector<std::string>& getErrors() const;
    const std::vector<std::string>& getWarnings() const;
    void clearErrors();
    void clearWarnings();
    void clearAll();

    // Validation options
    void setStrictMode(bool strict);
    void setRecoverMode(bool recover);

    // Internal access
    int getId() const { return id_; }
};

/**
 * @brief Enhanced XML Module with comprehensive DOM functionality
 */
class XmlModule : public BaseModule {
  public:
    XmlModule() {
        setModuleName("Xml2");
        this->className = "Xml2";
        setDescription("Comprehensive XML processing module with full DOM manipulation, XPath evaluation, and validation capabilities. "
                      "Supports XML Schema (XSD) and DTD validation, advanced XML features including CDATA sections, comments, "
                      "processing instructions, namespace management, encoding handling, and document formatting. "
                      "Phase 3 implementation with complete validation and advanced XML processing features.");
        LIBXML_TEST_VERSION
        xmlInitParser();
    }

    /**
     * @brief Register this module's symbols including enhanced classes
     */
    void registerFunctions() override;

    ~XmlModule() {
        // Clean up all document holders
        for (auto& doc : docHolder) {
            if (doc.second) {
                xmlFreeDoc(doc.second);
            }
        }
        docHolder.clear();
        nodeHolder.clear();
        documentHolder.clear();
        nodeObjectHolder.clear();
        nodeListHolder.clear();
        xpathHolder.clear();
        xpathResultHolder.clear();
        schemaHolder.clear();
        dtdHolder.clear();
        validatorHolder.clear();
        xmlCleanupParser();
    }

    // Static utility methods
    static std::string xmlElementTypeToString(int type);
    static bool isValidXmlName(const std::string& name);
    static std::string escapeXmlText(const std::string& text);
    static std::string unescapeXmlText(const std::string& text);

  private:
    const std::string objectStoreName = "__xml2_handler_id__";
    std::string className;

    // Legacy methods (backward compatibility)
    Symbols::ValuePtr readFile(FunctionArguments& args);
    Symbols::ValuePtr readMemory(FunctionArguments& args);
    Symbols::ValuePtr GetRootElement(const FunctionArguments& args);
    Symbols::ValuePtr GetNodeAttributes(FunctionArguments& args);

    // Enhanced document methods
    Symbols::ValuePtr createDocument(FunctionArguments& args);
    Symbols::ValuePtr createFromString(FunctionArguments& args);
    Symbols::ValuePtr createFromFile(FunctionArguments& args);

    // Document serialization methods
    Symbols::ValuePtr toString(FunctionArguments& args);
    Symbols::ValuePtr saveToFile(FunctionArguments& args);
    Symbols::ValuePtr saveToMemory(FunctionArguments& args);

    // Document validation methods
    Symbols::ValuePtr isWellFormed(FunctionArguments& args);
    Symbols::ValuePtr getVersion(FunctionArguments& args);
    Symbols::ValuePtr getEncoding(FunctionArguments& args);

    // Node creation methods
    Symbols::ValuePtr createElement(FunctionArguments& args);
    Symbols::ValuePtr createTextNode(FunctionArguments& args);

    // Node navigation methods
    Symbols::ValuePtr getParent(FunctionArguments& args);
    Symbols::ValuePtr getFirstChild(FunctionArguments& args);
    Symbols::ValuePtr getLastChild(FunctionArguments& args);
    Symbols::ValuePtr getNextSibling(FunctionArguments& args);
    Symbols::ValuePtr getPreviousSibling(FunctionArguments& args);
    Symbols::ValuePtr getChildren(FunctionArguments& args);

    // Node manipulation methods
    Symbols::ValuePtr appendChild(FunctionArguments& args);
    Symbols::ValuePtr insertBefore(FunctionArguments& args);
    Symbols::ValuePtr removeChild(FunctionArguments& args);
    Symbols::ValuePtr replaceChild(FunctionArguments& args);
    Symbols::ValuePtr cloneNode(FunctionArguments& args);

    // Node content methods
    Symbols::ValuePtr getName(FunctionArguments& args);
    Symbols::ValuePtr setName(FunctionArguments& args);
    Symbols::ValuePtr getContent(FunctionArguments& args);
    Symbols::ValuePtr setContent(FunctionArguments& args);
    Symbols::ValuePtr getTextContent(FunctionArguments& args);
    Symbols::ValuePtr setTextContent(FunctionArguments& args);

    // Attribute methods
    Symbols::ValuePtr getAttribute(FunctionArguments& args);
    Symbols::ValuePtr setAttribute(FunctionArguments& args);
    Symbols::ValuePtr hasAttribute(FunctionArguments& args);
    Symbols::ValuePtr removeAttribute(FunctionArguments& args);
    Symbols::ValuePtr getAllAttributes(FunctionArguments& args);

    // NodeList methods
    Symbols::ValuePtr getLength(FunctionArguments& args);
    Symbols::ValuePtr item(FunctionArguments& args);

    // Utility methods
    Symbols::ValuePtr hasChildNodes(FunctionArguments& args);

    // XPath methods
    Symbols::ValuePtr xpath(FunctionArguments& args);
    Symbols::ValuePtr selectNodes(FunctionArguments& args);
    Symbols::ValuePtr selectSingleNode(FunctionArguments& args);
    Symbols::ValuePtr createXPath(FunctionArguments& args);
    
    // XPath evaluation methods
    Symbols::ValuePtr xpathEvaluate(FunctionArguments& args);
    Symbols::ValuePtr xpathEvaluateWithContext(FunctionArguments& args);
    
    // XPath namespace methods
    Symbols::ValuePtr xpathRegisterNamespace(FunctionArguments& args);
    Symbols::ValuePtr xpathUnregisterNamespace(FunctionArguments& args);
    
    // XPath variable methods
    Symbols::ValuePtr xpathRegisterVariable(FunctionArguments& args);
    
    // XPath result methods
    Symbols::ValuePtr xpathResultGetType(FunctionArguments& args);
    Symbols::ValuePtr xpathResultGetBooleanValue(FunctionArguments& args);
    Symbols::ValuePtr xpathResultGetNumberValue(FunctionArguments& args);
    Symbols::ValuePtr xpathResultGetStringValue(FunctionArguments& args);
    Symbols::ValuePtr xpathResultGetNodeSet(FunctionArguments& args);
    Symbols::ValuePtr xpathResultGetNodeSetSize(FunctionArguments& args);

    // Validation methods
    Symbols::ValuePtr createSchema(FunctionArguments& args);
    Symbols::ValuePtr createDtd(FunctionArguments& args);
    Symbols::ValuePtr createValidator(FunctionArguments& args);
    
    // Schema methods
    Symbols::ValuePtr schemaLoadFromFile(FunctionArguments& args);
    Symbols::ValuePtr schemaLoadFromString(FunctionArguments& args);
    Symbols::ValuePtr schemaValidate(FunctionArguments& args);
    Symbols::ValuePtr schemaGetErrors(FunctionArguments& args);
    Symbols::ValuePtr schemaClearErrors(FunctionArguments& args);
    
    // DTD methods
    Symbols::ValuePtr dtdLoadFromFile(FunctionArguments& args);
    Symbols::ValuePtr dtdLoadFromString(FunctionArguments& args);
    Symbols::ValuePtr dtdValidate(FunctionArguments& args);
    Symbols::ValuePtr dtdGetErrors(FunctionArguments& args);
    Symbols::ValuePtr dtdClearErrors(FunctionArguments& args);
    
    // Validator methods
    Symbols::ValuePtr validatorValidateWellFormedness(FunctionArguments& args);
    Symbols::ValuePtr validatorValidateAgainstSchema(FunctionArguments& args);
    Symbols::ValuePtr validatorValidateAgainstDtd(FunctionArguments& args);
    Symbols::ValuePtr validatorGetErrors(FunctionArguments& args);
    Symbols::ValuePtr validatorGetWarnings(FunctionArguments& args);
    Symbols::ValuePtr validatorClearAll(FunctionArguments& args);
    
    // Document validation methods
    Symbols::ValuePtr validateAgainstSchema(FunctionArguments& args);
    Symbols::ValuePtr validateAgainstDtd(FunctionArguments& args);
    Symbols::ValuePtr validateWellFormedness(FunctionArguments& args);
    
    // Advanced document methods
    Symbols::ValuePtr setEncoding(FunctionArguments& args);
    Symbols::ValuePtr getDocumentEncoding(FunctionArguments& args);
    Symbols::ValuePtr formatDocument(FunctionArguments& args);
    Symbols::ValuePtr setFormatOutput(FunctionArguments& args);
    Symbols::ValuePtr getFormatOutput(FunctionArguments& args);
    
    // Advanced node creation
    Symbols::ValuePtr createComment(FunctionArguments& args);
    Symbols::ValuePtr createProcessingInstruction(FunctionArguments& args);
    Symbols::ValuePtr createCDataSection(FunctionArguments& args);
    
    // Namespace management
    Symbols::ValuePtr registerNamespacePrefix(FunctionArguments& args);
    Symbols::ValuePtr resolveNamespaceUri(FunctionArguments& args);
    Symbols::ValuePtr resolveNamespacePrefix(FunctionArguments& args);

    // Helper methods for registration
    void registerLegacyMethods();
    void registerDocumentMethods();
    void registerNodeMethods();
    void registerNodeListMethods();
    void registerXPathMethods();
    void registerValidationMethods();
    void registerAdvancedMethods();

    // Helper methods for object management
    XmlDocument* getDocumentFromArgs(const FunctionArguments& args) const;
    XmlNode* getNodeFromArgs(const FunctionArguments& args) const;
    XmlNodeList* getNodeListFromArgs(const FunctionArguments& args) const;
    XmlXPath* getXPathFromArgs(const FunctionArguments& args) const;
    XmlXPathResult* getXPathResultFromArgs(const FunctionArguments& args) const;
    XmlSchema* getSchemaFromArgs(const FunctionArguments& args) const;
    XmlDtd* getDtdFromArgs(const FunctionArguments& args) const;
    XmlValidator* getValidatorFromArgs(const FunctionArguments& args) const;
    Symbols::ValuePtr createDocumentObject(std::unique_ptr<XmlDocument> doc);
    Symbols::ValuePtr createNodeObject(std::unique_ptr<XmlNode> node);
    Symbols::ValuePtr createNodeListObject(std::unique_ptr<XmlNodeList> nodeList);
    Symbols::ValuePtr createXPathObject(std::unique_ptr<XmlXPath> xpath);
    Symbols::ValuePtr createXPathResultObject(std::unique_ptr<XmlXPathResult> result);
    Symbols::ValuePtr createSchemaObject(std::unique_ptr<XmlSchema> schema);
    Symbols::ValuePtr createDtdObject(std::unique_ptr<XmlDtd> dtd);
    Symbols::ValuePtr createValidatorObject(std::unique_ptr<XmlValidator> validator);
};

}  // namespace Modules

#endif  // XML_MODULE_HPP
