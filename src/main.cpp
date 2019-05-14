#include "nlohmann/json.hpp"
#include <fstream>
#include <unordered_set>

template <typename T>
struct BoxedOptional {

    BoxedOptional() = default;

    BoxedOptional(T value): value{new T{value}} {}

    BoxedOptional(BoxedOptional const & optional) {
        copyValueFrom(optional);
    }

    BoxedOptional & operator = (BoxedOptional const & optional) {
        reset();
        copyValueFrom(optional);
        return *this;
    }

    BoxedOptional(BoxedOptional && optional) {
        stealValueFrom(optional);
    }

    BoxedOptional & operator = (BoxedOptional && optional) {
        reset();
        stealValueFrom(optional);
        return *this;
    }

    ~BoxedOptional() {
        reset();
    }

    void reset() {
        if (value) {
            delete value;
            value = nullptr;
        }
    }

    bool has_value() const {
        return value != nullptr;
    }

    explicit operator bool() const {
        return has_value();
    }

    T & operator * () {
        return *value;
    }

    T const & operator * () const {
        return *value;
    }

    T * operator -> () {
        return value;
    }

    T const * operator -> () const {
        return value;
    }

private:
    T * value = nullptr;

    void copyValueFrom(BoxedOptional const & optional) {
        if (optional.value) {
            value = new T{*optional.value};
        }
    }

    void stealValueFrom(BoxedOptional & optional) {
        if (optional.value) {
            value = optional.value;
            optional.value = nullptr;
        }
    }

};

enum class TypeKind {
    Scalar, Object, Interface, Union, Enum, InputObject, List, NonNull
};

enum class Scalar {
    // 32 bit
    Int,
    // Double
    Float,
    // UTF-8
    String,
    Boolean,
    ID
};

struct TypeRef {
    TypeKind kind;
    std::optional<std::string> name;
    // NonNull and List only
    BoxedOptional<TypeRef> ofType;

    TypeRef const & underlyingType() const {
        if (ofType) {
            return ofType->underlyingType();
        }
        return *this;
    }

};

struct InputValue {
    std::string name;
    std::string description;
    TypeRef type;
    // TODO: Default value
};

struct Field {
    std::string name;
    std::string description;
    std::vector<InputValue> args;
    TypeRef type;
    // TODO: Deprecation
};

struct EnumValue {
    std::string name;
    std::string description;
    // TODO: Deprecation
};

struct Type {
    TypeKind kind;
    std::string name;
    std::string description;
    // Object and Interface only
    std::vector<Field> fields;
    // InputObject only
    std::vector<InputValue> inputFields;
    // Object only
    std::vector<TypeRef> interfaces;
    // Enum only
    std::vector<EnumValue> enumValues;
    // Interface and Union only
    std::vector<TypeRef> possibleTypes;
};

enum class Operation {
    Query, Mutation, Subscription
};

struct Schema {

    struct OperationType {
        std::string name;
    };

    std::optional<OperationType> queryType;
    std::optional<OperationType> mutationType;
    std::optional<OperationType> subscriptionType;
    std::vector<Type> types;
    // TODO: Directives
};

using TypeMap = std::unordered_map<std::string, Type>;

namespace nlohmann {
    template <typename T>
    struct adl_serializer<std::optional<T>> {
        static void to_json(json & json, std::optional<T> const & opt) {
            if (opt.has_value()) {
                json = *opt;
            } else {
                json = nullptr;
            }
        }

        static void from_json(const json & json, std::optional<T> & opt) {
            if (json.is_null()) {
                opt.reset();
            } else {
                opt = json.get<T>();
            }
        }
    };

    template <typename T>
    struct adl_serializer<BoxedOptional<T>> {
        static void to_json(json & json, BoxedOptional<T> const & opt) {
            if (opt.has_value()) {
                json = *opt;
            } else {
                json = nullptr;
            }
        }

        static void from_json(const json & json, BoxedOptional<T> & opt) {
            if (json.is_null()) {
                opt.reset();
            } else {
                opt = json.get<T>();
            }
        }
    };
}

using Json = nlohmann::json;

NLOHMANN_JSON_SERIALIZE_ENUM(TypeKind, {
    {TypeKind::Scalar, "SCALAR"},
    {TypeKind::Object, "OBJECT"},
    {TypeKind::Interface, "INTERFACE"},
    {TypeKind::Union, "UNION"},
    {TypeKind::Enum, "ENUM"},
    {TypeKind::InputObject, "INPUT_OBJECT"},
    {TypeKind::List, "LIST"},
    {TypeKind::NonNull, "NON_NULL"}
});

template <typename T>
void get_value_to(Json const & json, char const * key, T & target) {
    json.at(key).get_to(target);
}

template <typename T>
void get_value_to(Json const & json, char const * key, std::optional<T> & target) {
    auto it = json.find(key);
    if (it != json.end()) {
        it->get_to(target);
    } else {
        target.reset();
    }
}

template <typename T>
void get_value_to(Json const & json, char const * key, BoxedOptional<T> & target) {
    auto it = json.find(key);
    if (it != json.end()) {
        it->get_to(target);
    } else {
        target.reset();
    }
}

template <typename T>
void set_value_from(Json & json, char const * key, T const & source) {
    json[key] = source;
}

template <typename T>
void set_value_from(Json & json, char const * key, std::optional<T> const & source) {
    if (source) {
        json[key] = *source;
        return;
    }
    auto it = json.find(key);
    if (it != json.end()) {
        json.erase(it);
    }
}

void from_json(Json const & json, TypeRef & type) {
    get_value_to(json, "kind", type.kind);
    get_value_to(json, "name", type.name);
    get_value_to(json, "ofType", type.ofType);
}

void from_json(Json const & json, InputValue & input) {
    get_value_to(json, "name", input.name);
    get_value_to(json, "description", input.description);
    get_value_to(json, "type", input.type);
}

void from_json(Json const & json, Field & field) {
    get_value_to(json, "name", field.name);
    get_value_to(json, "description", field.description);
    get_value_to(json, "args", field.args);
    get_value_to(json, "type", field.type);
}

void from_json(Json const & json, EnumValue & value) {
    get_value_to(json, "name", value.name);
    get_value_to(json, "description", value.description);
}

void from_json(Json const & json, Type & type) {
    get_value_to(json, "kind", type.kind);
    get_value_to(json, "name", type.name);
    get_value_to(json, "description", type.description);
    get_value_to(json, "fields", type.fields);
    get_value_to(json, "inputFields", type.inputFields);
    get_value_to(json, "interfaces", type.interfaces);
    get_value_to(json, "enumValues", type.enumValues);
    get_value_to(json, "possibleTypes", type.possibleTypes);
}

void from_json(Json const & json, Schema::OperationType & operationType) {
    get_value_to(json, "name", operationType.name);
}

void from_json(Json const & json, Schema & schema) {
    get_value_to(json, "queryType", schema.queryType);
    get_value_to(json, "mutationType", schema.mutationType);
    get_value_to(json, "subscriptionType", schema.subscriptionType);
    get_value_to(json, "types", schema.types);
}

std::vector<Type> sortCustomTypesByDependencyOrder(std::vector<Type> const &types) {
    using namespace std;

    struct TypeWithDependencies {
        Type type;
        unordered_set<string> dependencies;
    };

    unordered_map<string, unordered_set<string>> typesToDependents;
    unordered_map<string, TypeWithDependencies> typesToDependencies;

    auto isCustomType = [](TypeKind kind) {
        switch (kind) {
            case TypeKind::Object:
            case TypeKind::Interface:
            case TypeKind::Union:
            case TypeKind::Enum:
            case TypeKind::InputObject:
                return true;

            case TypeKind::Scalar:
            case TypeKind::List:
            case TypeKind::NonNull:
                return false;
        }
    };

    for (auto const & type : types) {
        if (!isCustomType(type.kind)) {
            continue;
        }

        unordered_set<string> dependencies;

        auto addDependency = [&](auto const &dependency) {
            if (dependency.name && isCustomType(dependency.kind)) {
                typesToDependents[*dependency.name].insert(type.name);
                dependencies.insert(*dependency.name);
            }
        };

        for (auto const & field : type.fields) {
            addDependency(field.type.underlyingType());

            for (auto const & arg : field.args) {
                addDependency(arg.type.underlyingType());
            }
        }

        for (auto const & field : type.inputFields) {
            addDependency(field.type.underlyingType());
        }

        for (auto const & possibleType : type.possibleTypes) {
            addDependency(possibleType);
        }

        typesToDependencies[type.name] = {type, move(dependencies)};
    }

    vector<Type> sortedTypes;

    while (!typesToDependencies.empty()) {
        auto const initialCount = typesToDependencies.size();

        vector<string> addedTypeNames;

        for (auto const & pair : typesToDependencies) {
            if (pair.second.dependencies.empty()) {
                sortedTypes.push_back(pair.second.type);
                addedTypeNames.push_back(pair.first);

                for (auto const & dependentName : typesToDependents[pair.first]) {
                    typesToDependencies[dependentName].dependencies.erase(pair.first);
                }
            }
        }

        for (auto const &addedName : addedTypeNames) {
            typesToDependencies.erase(addedName);
        }

        if (typesToDependencies.size() == initialCount) {
            throw runtime_error{"Circular dependencies in schema"};
        }
    }

    return sortedTypes;
}

constexpr size_t spacesPerIndent = 4;
const std::string unknownCaseName = "Unknown";
const std::string cppJsonTypeName = "Json";
const std::string cppIdTypeName = "Id";
const std::string grapqlErrorTypeName = "GraphqlError";

std::string indent(size_t indentation) {
    return std::string(indentation * spacesPerIndent, ' ');
}

std::string generateDescription(std::string const & description, size_t indentation) {
    if (description.empty()) {
        return {};
    }

    std::string generated;

    // Insert a line break before comments
    generated += "\n";

    if (description.find('\n') == std::string::npos) {
        generated += indent(indentation) + "// " + description + "\n";
    } else {
        // Use block comments for multiline strings
        generated += indent(indentation) + "/*\n" + indent(indentation);

        for (auto const character : description) {
            if (character == '\n') {
                generated += "\n" + indent(indentation);
                continue;
            }

            generated += character;
        }

        generated += "\n" + indent(indentation) + "*/\n";
    }

    return generated;
}

std::string screamingSnakeCaseToPascalCase(std::string const & snake) {
    std::string pascal;

    bool isFirstInWord = true;
    for (auto const & character : snake) {
        if (character == '_') {
            isFirstInWord = true;
            continue;
        }

        if (isFirstInWord) {
            pascal += toupper(character);
            isFirstInWord = false;
        } else {
            pascal += tolower(character);
        }
    }

    return pascal;
}

std::string capitalize(std::string string) {
    string.at(0) = toupper(string.at(0));
    return string;
}

std::string uncapitalize(std::string string) {
    string.at(0) = tolower(string.at(0));
    return string;
}

std::string generateEnum(Type const & type, size_t indentation) {
    std::string generated;
    generated += indent(indentation) + "enum class " + type.name + " {\n";

    auto const valueIndentation = indentation + 1;

    for (auto const & value : type.enumValues) {
        generated += generateDescription(value.description, valueIndentation);
        generated += indent(valueIndentation) + screamingSnakeCaseToPascalCase(value.name) + ",\n";
    }

    generated += indent(valueIndentation) + unknownCaseName + " = -1\n";

    generated += indent(indentation) + "};\n\n";

    return generated;
}

std::string generateEnumSerializationFunctions(Type const & type, size_t indentation) {
    std::string generated;

    generated += indent(indentation) + "NLOHMANN_JSON_SERIALIZE_ENUM(" + type.name + ", {\n";

    auto const valueIndentation = indentation + 1;

    generated += indent(valueIndentation) + "{" + type.name + "::" + unknownCaseName + ", nullptr},\n";

    for (auto const & value : type.enumValues) {
        generated += indent(valueIndentation) + "{" + type.name + "::" + screamingSnakeCaseToPascalCase(value.name) + ", \"" + value.name + "\"},\n";
    }

    generated += indent(indentation) + "});\n\n";

    return generated;
}

Scalar scalarType(std::string const & name) {
    if (name == "Int") {
        return Scalar::Int;
    } else if (name == "Float") {
        return Scalar::Float;
    } else if (name == "String") {
        return Scalar::String;
    } else if (name == "Boolean") {
        return Scalar::Boolean;
    } else if (name == "ID") {
        return Scalar::ID;
    }

    throw std::runtime_error{"Unknown scalar type: " + name};
}

std::string cppScalarName(Scalar scalar) {
    switch (scalar) {
        case Scalar::Int:
            return "int32_t";
        case Scalar::Float:
            return "double";
        case Scalar::String:
            return "std::string";
        case Scalar::ID:
            return cppIdTypeName;
        case Scalar::Boolean:
            return "bool";
    }
}

std::string cppTypeName(TypeRef const & type, bool shouldCheckNullability = true) {
    if (shouldCheckNullability && type.kind != TypeKind::NonNull) {
        return "std::optional<" + cppTypeName(type, false) + ">";
    }

    switch (type.kind) {
        case TypeKind::Object:
        case TypeKind::Interface:
        case TypeKind::Union:
        case TypeKind::Enum:
        case TypeKind::InputObject:
            return type.name.value();

        case TypeKind::Scalar:
            return cppScalarName(scalarType(type.name.value()));

        case TypeKind::List:
            return "std::vector<" + cppTypeName(*type.ofType, false) + ">";

        case TypeKind::NonNull:
            return cppTypeName(*type.ofType, false);
    }
}

std::string graphqlTypeName(TypeRef const & type) {
    switch (type.kind) {
        case TypeKind::Scalar:
        case TypeKind::Object:
        case TypeKind::Union:
        case TypeKind::Interface:
        case TypeKind::Enum:
        case TypeKind::InputObject:
            return type.name.value();

        case TypeKind::List:
            return "[" + graphqlTypeName(*type.ofType) + "]";

        case TypeKind::NonNull:
            return graphqlTypeName(*type.ofType) + "!";
    }
}

template <typename T>
std::string generateField(T const & field, size_t indentation) {
    std::string generated;
    generated += generateDescription(field.description, indentation);
    generated += indent(indentation) + cppTypeName(field.type) + " " + field.name + ";\n";
    return generated;
}

std::string cppVariant(std::vector<TypeRef> const & possibleTypes, std::string const & unknownTypeName) {
    std::string generated = "std::variant<";
    for (auto const & type : possibleTypes) {
        generated += type.name.value() + ", ";
    }
    generated += unknownTypeName + ">";
    return generated;
}

std::string generateDeserializationFunctionDeclaration(std::string const & typeName, size_t indentation) {
    return indent(indentation) + "inline void from_json(" + cppJsonTypeName + " const & json, " + typeName + " & value) {\n";
}

std::string generateInterface(Type const & type, size_t indentation) {
    std::string interface;
    std::string unknownImplementation;

    interface += indent(indentation) + "struct " + type.name + " {\n";
    auto const unknownTypeName = unknownCaseName + type.name;
    unknownImplementation += indent(indentation) + "struct " + unknownTypeName + " {\n";

    auto const fieldIndentation = indentation + 1;

    interface += indent(fieldIndentation) + cppVariant(type.possibleTypes, unknownTypeName) + " implementation;\n\n";

    for (auto const & field : type.fields) {
        auto const typeName = cppTypeName(field.type);
        unknownImplementation += indent(fieldIndentation) + typeName + " " + field.name + ";\n";

        auto const typeNameConstRef = typeName + " const & ";
        interface += generateDescription(field.description, fieldIndentation);
        interface += indent(fieldIndentation) + typeNameConstRef + field.name + "() const {\n";
        interface += indent(fieldIndentation + 1) + "return std::visit([](auto const & implementation) -> "
        + typeNameConstRef + "{\n";
        interface += indent(fieldIndentation + 2) + "return implementation." + field.name + ";\n";
        interface += indent(fieldIndentation + 1) + "}, implementation);\n";
        interface += indent(fieldIndentation) + "}\n\n";
    }

    interface += indent(indentation) + "};\n\n";
    unknownImplementation += indent(indentation) + "};\n\n";

    return unknownImplementation + interface;
}

std::string generateUnion(Type const & type, size_t indentation) {
    std::string generated;

    auto const unknownTypeName = unknownCaseName + type.name;
    generated += "using " + unknownTypeName + " = std::monostate;\n\n";
    generated += indent(indentation) + "using " + type.name + " = " + cppVariant(type.possibleTypes, unknownTypeName) + ";\n\n";
    return generated;
}

std::string generateObject(Type const & type, TypeMap const & typeMap, size_t indentation) {
    std::string generated;

    generated += indent(indentation) + "struct " + type.name + " {\n";

    auto const fieldIndentation = indentation + 1;

    for (auto const & field : type.fields) {
        generated += generateField(field, fieldIndentation);
    }

    generated += indent(indentation) + "};\n\n";

    return generated;
}

std::string generateObjectDeserialization(Type const & type, size_t indentation) {
    std::string generated;

    // TODO

    return generated;
}

std::string generateInputObject(Type const & type, size_t indentation) {
    std::string generated;

    generated += indent(indentation) + "struct " + type.name + " {\n";

    auto const fieldIndentation = indentation + 1;

    for (auto const & field : type.inputFields) {
        generated += generateField(field, fieldIndentation);
    }

    generated += indent(indentation) + "};\n\n";

    return generated;
}

template <typename FieldType>
std::string generateFieldSerialization(FieldType const & field, const std::string & fieldPrefix, const std::string & jsonName, size_t indentation) {
    return indent(indentation) + jsonName + "[\"" + field.name + "\"] = " + fieldPrefix + field.name + ";\n";
}

std::string generateInputObjectSerializationFunction(Type const & type, size_t indentation) {
    std::string generated;

    generated += indent(indentation) + "inline void to_json(" + cppJsonTypeName + " & json, " + type.name + " const & object) {\n";

    for (auto const & field : type.inputFields) {
        generated += generateFieldSerialization(field, "object.", "json", indentation + 1);
    }

    generated += indent(indentation) + "}\n\n";

    return generated;
}

std::string operationQueryName(Operation operation) {
    switch (operation) {
        case Operation::Query:
            return "query";
        case Operation::Mutation:
            return "mutation";
        case Operation::Subscription:
            return "subscription";
    }
}

struct QueryVariable {
    std::string name;
    TypeRef type;
};

std::string appendNameToVariablePrefix(std::string const & variablePrefix, std::string const & name) {
    return variablePrefix.empty() ? name : variablePrefix + capitalize(name);
}

std::string generateQueryFields(Type const & type, TypeMap const & typeMap, std::string const & variablePrefix, std::vector<QueryVariable> & variables, size_t indentation);

std::string generateQueryField(Field const & field, TypeMap const & typeMap, std::string const & variablePrefix, std::vector<QueryVariable> & variables, size_t indentation) {
    std::string generated;

    generated += indent(indentation) + field.name;

    if (!field.args.empty()) {
        generated += "(\n";
        for (auto const & arg : field.args) {
            auto name = appendNameToVariablePrefix(variablePrefix, arg.name);
            generated += indent(indentation + 1) + name + ": $" + name + "\n";
            variables.push_back({name, arg.type});
        }
        generated += indent(indentation) + ")";
    }

    auto const & underlyingFieldType = field.type.underlyingType();
    if (underlyingFieldType.kind != TypeKind::Scalar && underlyingFieldType.kind != TypeKind::Enum) {
        generated += " {\n";
        generated += generateQueryFields(typeMap.at(underlyingFieldType.name.value()), typeMap, appendNameToVariablePrefix(variablePrefix, underlyingFieldType.name.value()), variables, indentation + 1);
        generated += indent(indentation) + "}";
    }

    generated += "\n";

    return generated;
}

std::string generateQueryFields(Type const & type, TypeMap const & typeMap, std::string const & variablePrefix, std::vector<QueryVariable> & variables, size_t indentation) {
    std::string generated;

    if (!type.possibleTypes.empty()) {
        generated += indent(indentation) + "__typename\n";

        for (auto const & possibleType : type.possibleTypes) {
            generated += indent(indentation) + "...on " + possibleType.name.value() + " {\n";
            generated += generateQueryFields(typeMap.at(possibleType.name.value()), typeMap, appendNameToVariablePrefix(variablePrefix, possibleType.name.value()), variables, indentation + 1);
            generated += indent(indentation) + "}\n";
        }
    } else {
        for (auto const & field : type.fields) {
            generated += generateQueryField(field, typeMap, appendNameToVariablePrefix(variablePrefix, field.name), variables, indentation);
        }
    }

    return generated;
}

struct QueryDocument {
    std::string query;
    std::vector<QueryVariable> variables;
};

QueryDocument generateQueryDocument(Field const & field, Operation operation, TypeMap const & typeMap, size_t indentation) {
    QueryDocument document;
    auto & query = document.query;
    auto & variables = document.variables;

    auto selectionSet = generateQueryField(field, typeMap, "", variables, indentation + 1);

    query += indent(indentation) + operationQueryName(operation) + " " + capitalize(field.name) + "(\n";

    for (auto const & variable : variables) {
        query += indent(indentation + 1) + "$" + variable.name + ": " + graphqlTypeName(variable.type) + "\n";
    }

    query += indent(indentation) + ") {\n";
    query += selectionSet;
    query += indent(indentation) + "}\n";

    return document;
}

std::string generateOperationRequestFunction(Field const & field, Operation operation, TypeMap const & typeMap, size_t indentation) {
    auto const functionIndentation = indentation + 1;
    auto const queryIndentation = functionIndentation + 1;

    auto const document = generateQueryDocument(field, operation, typeMap, queryIndentation);

    std::string generated;
    generated += indent(indentation) + "static " + cppJsonTypeName + " request(";

    for (auto it = document.variables.begin(); it != document.variables.end(); ++it) {
        generated += cppTypeName(it->type) + " " + it->name;

        if (it != document.variables.end() - 1) {
            generated += ", ";
        }
    }

    generated += ") {\n";

    // Use raw string literal for the query.
    generated += indent(functionIndentation) + cppJsonTypeName + " query = R\"(\n" + document.query + indent(functionIndentation) + ")\";\n";
    generated += indent(functionIndentation) + cppJsonTypeName + " variables;\n";

    for (auto const & variable : document.variables) {
        generated += generateFieldSerialization(variable, "", "variables", functionIndentation);
    }

    generated += indent(functionIndentation) + "return {{\"query\", std::move(query)}, {\"variables\", std::move(variables)}};\n";

    generated += indent(indentation) + "}\n\n";

    return generated;
}

std::string generateOperationResponseFunction(Field const & field, size_t indentation) {
    std::string generated;

    auto const dataType = cppTypeName(field.type);
    auto const errorsType = "std::vector<" + grapqlErrorTypeName + ">";
    auto const responseType = "std::variant<" + dataType + ", " + errorsType + ">";

    generated += indent(indentation) + "static " + responseType + " response(" + cppJsonTypeName + " const & json) {\n";

    generated += indent(indentation + 1) + "if (json.find(\"errors\") != json.end()) {\n";
    generated += indent(indentation + 2) + "return " + errorsType + "(json);\n";
    generated += indent(indentation + 1) + "} else {\n";
    generated += indent(indentation + 2) + "return " + dataType + "(json.at(\"" + field.name + "\"));\n";
    generated += indent(indentation + 1) + "}\n";

    generated += indent(indentation) + "}\n\n";

    return generated;
}

std::string generateOperationType(Field const & field, Operation operation, TypeMap const & typeMap, size_t indentation) {
    auto const document = generateQueryDocument(field, operation, typeMap, 0);

    std::string generated;

    generated += indent(indentation) + "struct " + capitalize(field.name) + " {\n\n";

    generated += generateOperationRequestFunction(field, operation, typeMap, indentation + 1);
    generated += generateOperationResponseFunction(field, indentation + 1);

    generated += indent(indentation) + "};\n\n";

    return generated;
}

std::string generateOperationTypes(Type const & type, Operation operation, TypeMap const & typeMap, size_t indentation) {
    std::string generated;

    generated += indent(indentation) + "namespace " + type.name + " {\n\n";

    for (auto const & field : type.fields) {
        generated += generateOperationType(field, operation, typeMap, indentation + 1);
    }

    generated += indent(indentation) + "}; // namespace " + type.name + " \n\n";

    return generated;
}

std::string generateGraphqlErrorType(size_t indentation) {
    std::string generated;
    generated += indent(indentation) + "struct " + grapqlErrorTypeName + " {\n";
    generated += indent(indentation + 1) + "std::string message;\n";
    generated += indent(indentation) + "};\n\n";
    return generated;
}

std::string generateGraphqlErrorDeserialization(size_t indentation) {
    std::string generated;

    generated += generateDeserializationFunctionDeclaration(grapqlErrorTypeName, indentation);
    generated += indent(indentation + 1) + "json.at(\"message\").get_to(value.message);\n";
    generated += indent(indentation) + "}\n\n";

    return generated;
}

std::string generateTypes(Schema const & schema, std::string const & generatedNamespace) {
    auto const sortedTypes = sortCustomTypesByDependencyOrder(schema.types);

    TypeMap typeMap;

    for (auto const & type : schema.types) {
        typeMap[type.name] = type;
    }

    std::string source;

    source +=
R"(// This file was automatically generated and should not be edited.

#include <memory>
#include <optional>
#include <variant>
#include <vector>
#include "nlohmann/json.hpp"

// std::optional serialization
namespace nlohmann {
    template <typename T>
    struct adl_serializer<std::optional<T>> {
        static void to_json(json & json, std::optional<T> const & opt) {
            if (opt.has_value()) {
                json = *opt;
            } else {
                json = nullptr;
            }
        }

        static void from_json(const json & json, std::optional<T> & opt) {
            if (json.is_null()) {
                opt.reset();
            } else {
                opt = json.get<T>();
            }
        }
    };
}

)";

    source += "namespace " + generatedNamespace + " {\n\n";

    size_t typeIndentation = 1;

    source += indent(typeIndentation) + "using " + cppJsonTypeName + " = nlohmann::json;\n";
    source += indent(typeIndentation) + "using " + cppIdTypeName + " = std::string;\n\n";

    source += generateGraphqlErrorType(typeIndentation);
    source += generateGraphqlErrorDeserialization(typeIndentation);

    for (auto const & type : sortedTypes) {
        auto isOperationType = [&](std::optional<Schema::OperationType> const & special) {
            return special && special->name == type.name;
        };

        switch (type.kind) {
            case TypeKind::Object:
                if (isOperationType(schema.queryType)) {
                    source += generateOperationTypes(type, Operation::Query, typeMap, typeIndentation);
                } else if (isOperationType(schema.mutationType)) {
                    source += generateOperationTypes(type, Operation::Mutation, typeMap, typeIndentation);
                } else if (isOperationType(schema.subscriptionType)) {
                    source += generateOperationTypes(type, Operation::Subscription, typeMap, typeIndentation);
                } else {
                    source += generateObject(type, typeMap, typeIndentation);
                }
                break;

            case TypeKind::Interface:
                source += generateInterface(type, typeIndentation);
                break;

            case TypeKind::Union:
                source += generateUnion(type, typeIndentation);
                break;

            case TypeKind::Enum:
                source += generateEnum(type, typeIndentation);
                source += generateEnumSerializationFunctions(type, typeIndentation);
                break;

            case TypeKind::InputObject:
                source += generateInputObject(type, typeIndentation);
                source += generateInputObjectSerializationFunction(type, typeIndentation);
                break;

            case TypeKind::Scalar:
            case TypeKind::List:
            case TypeKind::NonNull:
                break;
        }
    }

    source += "} // namespace " + generatedNamespace + "\n";

    return source;
}

int main(int argc, const char * argv[]) {
    if (argc < 3) {
        printf("Please provide an input schema and namespace\n");
        return 0;
    }

    std::ifstream file{argv[1]};
    auto const json = Json::parse(file);
    Schema schema = json.at("data").at("__schema");

    auto const source = generateTypes(schema, argv[2]);
    std::ofstream out{"Generated.hpp"};
    out << source;
    out.close();

    return 0;
}
