#pragma once

#include <QByteArray>
#include <cstring>

#include "IPCProtocol.h"
#include "ComponentRegistry.h"

// Builds the binary payload for MsgType::EntityComponents.
//
// Usage:
//   ComponentBuilder b(entityId);
//   b.begin_component("TransformComponent");
//   b.field_vec3("position", x, y, z);
//   b.field_float("fov", fov);
//   QByteArray payload = b.build();
class ComponentBuilder : public FieldWriter {
public:
    explicit ComponentBuilder(uint32_t entityId) : m_entityId(entityId) {}

    void begin_component(const char* typeName) {
        char name[64] = {};
        strncpy(name, typeName, 63);
        m_data.append(reinterpret_cast<const char*>(name), 64);

        m_fieldCountOffset = m_data.size();
        uint32_t fc = 0;
        m_data.append(reinterpret_cast<const char*>(&fc), sizeof(fc));

        m_currentFieldCount = 0;
        m_componentCount++;
    }

    void field_float(const char* name, float v) {
        append_field(name, FieldType::Float, &v, sizeof(v));
    }

    void field_int(const char* name, int32_t v) {
        append_field(name, FieldType::Int32, &v, sizeof(v));
    }

    void field_bool(const char* name, bool v) {
        uint32_t u = v ? 1 : 0;
        append_field(name, FieldType::Bool, &u, sizeof(u));
    }

    void field_vec3(const char* name, float x, float y, float z) {
        float v[3] = { x, y, z };
        append_field(name, FieldType::Vec3, v, sizeof(v));
    }

    void field_vec4(const char* name, float x, float y, float z, float w) {
        float v[4] = { x, y, z, w };
        append_field(name, FieldType::Vec4, v, sizeof(v));
    }

    void field_color(const char* name, float r, float g, float b) {
        float v[3] = { r, g, b };
        append_field(name, FieldType::Color, v, sizeof(v));
    }

    void field_string(const char* name, const char* str) {
        char buf[64] = {};
        strncpy(buf, str, 63);
        append_field(name, FieldType::String, buf, sizeof(buf));
    }

    // FieldWriter implementation
    void write_float (const char* name, float v) override                  { field_float(name, v); }
    void write_vec3  (const char* name, float x, float y, float z) override { field_vec3(name, x, y, z); }
    void write_bool  (const char* name, bool v) override                   { field_bool(name, v); }
    void write_string(const char* name, const char* s) override            { field_string(name, s); }
    void write_color (const char* name, float r, float g, float b) override { field_color(name, r, g, b); }

    QByteArray build() const {
        QByteArray result;
        result.append(reinterpret_cast<const char*>(&m_entityId),       sizeof(m_entityId));
        result.append(reinterpret_cast<const char*>(&m_componentCount), sizeof(m_componentCount));
        result.append(m_data);
        return result;
    }

private:
    void append_field(const char* name, FieldType type, const void* data, uint32_t size) {
        char nameBuf[32] = {};
        strncpy(nameBuf, name, 31);
        m_data.append(reinterpret_cast<const char*>(nameBuf), 32);

        uint32_t t = static_cast<uint32_t>(type);
        m_data.append(reinterpret_cast<const char*>(&t), sizeof(t));
        m_data.append(reinterpret_cast<const char*>(data), size);

        m_currentFieldCount++;
        memcpy(m_data.data() + m_fieldCountOffset,
               &m_currentFieldCount, sizeof(m_currentFieldCount));
    }

    uint32_t  m_entityId;
    QByteArray m_data;
    uint32_t   m_componentCount      = 0;
    uint32_t   m_currentFieldCount   = 0;
    int        m_fieldCountOffset    = 0;
};
