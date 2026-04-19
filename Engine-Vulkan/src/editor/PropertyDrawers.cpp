#include <QCheckBox>
#include <QColorDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <cstring>

#include "DragSpinBox.h"

#include "PropertyDrawers.h"

QWidget* create_field_widget(FieldType type, const QByteArray& value)
{
    const float*    f = reinterpret_cast<const float*>(value.constData());
    const uint32_t* u = reinterpret_cast<const uint32_t*>(value.constData());

    switch (type) {
    case FieldType::Float: {
        auto* spin = new DragSpinBox();
        spin->setValue(static_cast<double>(f[0]));
        return spin;
    }
    case FieldType::Vec3:
    case FieldType::Vec4: {
        int count = (type == FieldType::Vec3) ? 3 : 4;
        auto* container = new QWidget();
        auto* layout    = new QHBoxLayout(container);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(2);
        for (int i = 0; i < count; ++i) {
            auto* spin = new DragSpinBox();
            spin->setValue(static_cast<double>(f[i]));
            layout->addWidget(spin);
        }
        return container;
    }
    case FieldType::Color: {
        auto* btn   = new QPushButton();
        QColor color = QColor::fromRgbF(f[0], f[1], f[2]);
        btn->setProperty("currentColor", color);
        btn->setStyleSheet(QString("background-color: grey; border: none;").arg(color.name()));
        btn->setFixedHeight(22);
        return btn;
    }
    case FieldType::Bool: {
        auto* cb = new QCheckBox();
        cb->setChecked(u[0] != 0);
        return cb;
    }
    case FieldType::String: {
        return new QLineEdit(QString::fromUtf8(
            value.constData(), static_cast<int>(qstrnlen(value.constData(), 64))));
    }
    default:
        return new QLabel("?");
    }
}

void connect_field_changes(QWidget* widget, FieldType type,
                           std::function<void(QByteArray)> onChange)
{
    switch (type) {
    case FieldType::Float: {
        auto* spin = qobject_cast<DragSpinBox*>(widget);
        if (!spin) return;
        QObject::connect(spin, &DragSpinBox::valueChanged, [spin, onChange](double) {
            QByteArray bytes(4, 0);
            float v = static_cast<float>(spin->value());
            memcpy(bytes.data(), &v, 4);
            onChange(bytes);
        });
        break;
    }
    case FieldType::Vec3:
    case FieldType::Vec4: {
        int count = (type == FieldType::Vec3) ? 3 : 4;
        auto* layout = qobject_cast<QHBoxLayout*>(widget->layout());
        if (!layout) return;
        for (int i = 0; i < count; ++i) {
            auto* spin = qobject_cast<DragSpinBox*>(layout->itemAt(i)->widget());
            if (!spin) continue;
            QObject::connect(spin, &DragSpinBox::valueChanged,
                             [layout, count, onChange](double) {
                QByteArray bytes(count * 4, 0);
                for (int j = 0; j < count; ++j) {
                    auto* s = qobject_cast<DragSpinBox*>(layout->itemAt(j)->widget());
                    float v = s ? static_cast<float>(s->value()) : 0.0f;
                    memcpy(bytes.data() + j * 4, &v, 4);
                }
                onChange(bytes);
            });
        }
        break;
    }
    case FieldType::String: {
        auto* le = qobject_cast<QLineEdit*>(widget);
        if (!le) return;
        QObject::connect(le, &QLineEdit::editingFinished, [le, onChange]() {
            QByteArray bytes(64, 0);
            auto utf8 = le->text().toUtf8().left(63);
            memcpy(bytes.data(), utf8.constData(), utf8.size());
            onChange(bytes);
        });
        break;
    }
    case FieldType::Bool: {
        auto* cb = qobject_cast<QCheckBox*>(widget);
        if (!cb) return;
        QObject::connect(cb, &QCheckBox::toggled, [onChange](bool checked) {
            QByteArray bytes(4, 0);
            uint32_t v = checked ? 1u : 0u;
            memcpy(bytes.data(), &v, 4);
            onChange(bytes);
        });
        break;
    }
    case FieldType::Color: {
        auto* btn = qobject_cast<QPushButton*>(widget);
        if (!btn) return;
        QObject::connect(btn, &QPushButton::clicked, [btn, onChange]() {
            QColor initial = btn->property("currentColor").value<QColor>();
            QColor c = QColorDialog::getColor(initial, btn, "Pick Color",
                                              QColorDialog::DontUseNativeDialog);
            if (!c.isValid()) return;
            btn->setProperty("currentColor", c);
            btn->setStyleSheet(
                QString("background-color: %1; border: none;").arg(c.name()));
            QByteArray bytes(12, 0);
            float rgb[3] = { (float)c.redF(), (float)c.greenF(), (float)c.blueF() };
            memcpy(bytes.data(), rgb, 12);
            onChange(bytes);
        });
        break;
    }
    default:
        break;
    }
}
