#pragma once

#include <cstdint>

// Shared between SparkleRuntime and SparkleEditor

enum class MsgType : uint32_t {
    Ping             = 0,  // editor -> runtime: keepalive
    Pong             = 1,  // runtime -> editor: keepalive response
    FrameReady       = 2,  // reserved for future use
    Resize           = 3,  // editor -> runtime: viewport widget resized
    Shutdown         = 4,  // editor -> runtime: graceful shutdown
    SetViewport      = 5,  // editor -> runtime: HWND of the Qt viewport to render into
    WorldSnapshot    = 6,  // editor -> runtime: get snapshot of the world
    SelectEntity        = 7,  // editor -> runtime: ask for entity components
    EntityComponents    = 8,  // runtime -> editor: send entity components
    SetComponentField   = 9,  // editor -> runtime: update one field on an entity's component
    CreateEntity        = 10, // editor -> runtime: create a new empty entity
    RenameEntity        = 11, // editor -> runtime: rename an entity
    SetParent           = 12, // editor -> runtime: set entity parent (UINT32_MAX = root)
};

enum class FieldType : uint32_t {
    Float  = 0,
    Int32  = 1,
    Bool   = 2,
    Vec3   = 3,
    Vec4   = 4,
    String = 5,
    Color  = 6,  // rgb float[3], displayed as color swatch
};

inline uint32_t field_type_size(FieldType t) {
    switch (t) {
    case FieldType::Float:  return 4;
    case FieldType::Int32:  return 4;
    case FieldType::Bool:   return 4;
    case FieldType::Vec3:   return 12;
    case FieldType::Color:  return 12;
    case FieldType::Vec4:   return 16;
    case FieldType::String: return 64;
    default:                return 0;
    }
}

#pragma pack(push, 1)

struct MsgHeader {
    uint32_t type;        // MsgType
    uint32_t payloadSize; // bytes following this header
};

struct MsgResize {
    uint32_t width;
    uint32_t height;
};

struct MsgFrameReady {
    uint64_t handle;      // reserved
    uint32_t frameIndex;
    uint32_t width;
    uint32_t height;
};

struct MsgSetViewport {
    uint64_t hwnd;    // HWND cast to uint64_t — the Qt viewport widget's native handle
    uint32_t width;
    uint32_t height;
};

struct MsgSelectEntity {
    uint32_t entityId;
};

struct MsgSetComponentField {
    uint32_t entityId;
    char     componentName[64];
    char     fieldName[32];
    uint32_t fieldType;   // FieldType
    uint8_t  value[64];   // raw bytes; String is the largest (64 bytes)
};

struct EntityInfo {
    uint32_t id;
    char     name[64];
    uint32_t parentId; // UINT32_MAX = no parent
};

struct MsgCreateEntity {
    char name[64];
};

struct MsgRenameEntity {
    uint32_t entityId;
    char     name[64];
};

struct MsgSetParent {
    uint32_t entityId;
    uint32_t parentId; // UINT32_MAX = make root
};

#pragma pack(pop)