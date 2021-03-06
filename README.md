# CaffQL

`caffql` generates c++ types and GraphQL request and response serialization logic from a GraphQL json schema file.

## Usage

### Command line options
```bash
-s, --schema arg     input json schema file
-o, --output arg     output generated header file
-n, --namespace arg  generated namespace (default: caffql)
-a, --absl           use absl optional and variant instead of std
-h, --help           help
```

### Example
```bash
caffql \
    --schema mygraphqlschema.json \
    --output GeneratedCode.hpp
```

### Obtaining a GraphQL json schema file
Make an [introspection query](IntrospectionQuery.graphql) to your graphql endpoint and use the resulting json response as the `schema` parameter to `caffql`.

## Generated Code
`caffql` generates a c++ header file with types necessary to perform queries.
### Requirements
* c++17 for `std::optional` and `std::variant`  
  **or** c++11 and [Abseil](https://abseil.io/) for `absl::optional` and `absl::variant`
* [nlohmann/json](https://github.com/nlohmann/json) for request and response serialization

### Operations
`caffql` will generate request and response functions for each field of the input schema's operation types (`query`, `subscription`, and `mutation`). Currently only a single field can be queried at once. 

All subfields and nested types of that field will be included in the query, i.e. there is no way to query a subset of a model. The benefits to this approach are that you don't have to handwrite any queries and the generated request and response functions are kept simple, while the drawback is that you can't omit any unwanted data.

### Types

| GraphQL Type    | Generated C++ Type                                         |
|-----------------|------------------------------------------------------------|
| Int             | int32_t                                                    |
| Float           | double                                                     |
| String          | std::string                                                |
| ID              | Id (std::string typealias)                                 |
| Boolean         | bool                                                       |
| Enum            | enum class                                                 |
| Object          | struct                                                     |
| Interface       | struct containing std::variant of possible implementations |
| Union           | variant of possible types                             |
| InputObject     | struct                                                     |
| List            | std::vector                                                |
| Nullable fields | optional                                              |

#### Type considerations and forwards compatibility
##### Enums
For forwards compatibility, a special `Unknown` case is generated. Enum values that the client is unaware of will be deserialized to the `Unknown` case.

##### Interfaces
Interfaces are generated as `struct`s with an `implementation` member that is a `std::variant` of the possible implementations of the interface. For each field of the interface, a member function is generated that visits the `implementation` and returns the field.

An alternative approach would be to use a pure abstract class for the interface and have the implementations inherit from it. However, that approach prevents using the interface type as a simple value type (which would require implementing something like a polymorphic value-copying smart pointer).

For forwards compatibility, a special `Unknown<InterfaceName>` type is generated that contains the fields of the interface. Interface implementations that the client is unaware of will deserialize to the `Unknown<InterfaceName>` type.

##### Unions
For forwards compatibility, the generated `std::variant` adds `std::monostate` as a possible type to handle unknown types that the client is unaware of.



