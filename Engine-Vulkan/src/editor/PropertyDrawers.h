#pragma once

#include <QWidget>
#include <functional>
#include "IPCProtocol.h"

// Creates a widget pre-populated with the field's current value.
QWidget* create_field_widget(FieldType type, const QByteArray& value);

// Connects the widget's change signals to onChange(serialized bytes).
void connect_field_changes(QWidget* widget, FieldType type,
                           std::function<void(QByteArray)> onChange);
