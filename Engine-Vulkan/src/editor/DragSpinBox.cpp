#include <QApplication>
#include <QCursor>
#include <QEnterEvent>
#include <QFocusEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>

#include <iostream>

#include "DragSpinBox.h"

DragSpinBox::DragSpinBox(QWidget* parent)
    : QWidget(parent)
{
    setMinimumWidth(50);
    setCursor(Qt::SizeHorCursor);
    setFocusPolicy(Qt::StrongFocus);
}

void DragSpinBox::setValue(double value)
{
    set_value_clamped(value);
    update();
}

void DragSpinBox::setRange(double min, double max)
{
    m_min = min;
    m_max = max;
    set_value_clamped(m_value);
}

void DragSpinBox::setDecimals(int decimals)
{
    m_decimals = decimals;
    update();
}

void DragSpinBox::setSensitivity(double pixelsPerUnit)
{
    m_sensitivity = pixelsPerUnit;
}

void DragSpinBox::setReadOnly(bool ro)
{
    m_readOnly = ro;
    setCursor(ro ? Qt::ArrowCursor : Qt::SizeHorCursor);
}

// ── painting ──────────────────────────────────────────────────────────────────

void DragSpinBox::paintEvent(QPaintEvent*)
{
    if (m_lineEdit) return; // line edit is drawn by Qt, skip our paint

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QColor bg = m_dragging ? QColor(60, 100, 160)
              : m_hovered  ? QColor(75, 85,  105)
                           : QColor(55, 60,   75);
    p.setBrush(bg);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 3, 3);

    p.setPen(QColor(200, 210, 230));
    p.drawText(rect(), Qt::AlignCenter,
               QString::number(m_value, 'f', m_decimals));
}

QSize DragSpinBox::sizeHint() const
{
    return { 70, 22 };
}

// ── mouse ─────────────────────────────────────────────────────────────────────

void DragSpinBox::mousePressEvent(QMouseEvent* event)
{
    if (m_readOnly || event->button() != Qt::LeftButton) return;

    m_dragging       = true;
    m_dragCenter     = event->globalPosition().toPoint();
    m_dragStartValue = m_value;
    QApplication::setOverrideCursor(Qt::BlankCursor);
}

void DragSpinBox::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_dragging) return;

    QPoint current = event->globalPosition().toPoint();
    int delta = current.x() - m_dragCenter.x();
    if (delta == 0) return; // synthetic event from the warp below

    double speed = m_sensitivity;
    if (event->modifiers() & Qt::ControlModifier) speed *= 0.1;
    if (event->modifiers() & Qt::ShiftModifier)   speed *= 10.0;

    set_value_clamped(m_value + delta * speed);
    update();

    QCursor::setPos(m_dragCenter); // keep cursor locked in place
}

void DragSpinBox::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton || !m_dragging) return;

    m_dragging = false;
    QApplication::restoreOverrideCursor();
    update();
}

void DragSpinBox::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (m_readOnly || event->button() != Qt::LeftButton) return;
    start_edit();
}

void DragSpinBox::enterEvent(QEnterEvent*)
{
    m_hovered = true;
    update();
}

void DragSpinBox::leaveEvent(QEvent*)
{
    m_hovered = false;
    update();
}

void DragSpinBox::focusOutEvent(QFocusEvent*)
{
    // Focus may have moved to our own child QLineEdit — don't commit in that case.
    if (m_lineEdit && QApplication::focusWidget() == m_lineEdit) return;
    if (m_lineEdit) commit_edit();
}

// ── text editing ──────────────────────────────────────────────────────────────

void DragSpinBox::start_edit()
{
    if (m_lineEdit) return;

    m_lineEdit = new QLineEdit(this);
    m_lineEdit->setGeometry(rect());
    m_lineEdit->setText(QString::number(m_value, 'f', m_decimals));
    m_lineEdit->selectAll();
    m_lineEdit->setFrame(false);
    m_lineEdit->setAlignment(Qt::AlignCenter);
    m_lineEdit->setStyleSheet(
        "background: #2a3a5a; color: #e0e8ff; border-radius: 3px;");
    m_lineEdit->show();
    m_lineEdit->setFocus();

    connect(m_lineEdit, &QLineEdit::returnPressed, this, &DragSpinBox::commit_edit);
    connect(m_lineEdit, &QLineEdit::editingFinished, this, &DragSpinBox::commit_edit);
}

void DragSpinBox::commit_edit()
{
    if (!m_lineEdit) return;

    bool ok = false;
    double v = m_lineEdit->text().toDouble(&ok);
    if (ok)
        set_value_clamped(v);

    m_lineEdit->deleteLater();
    m_lineEdit = nullptr;
    update();
}

// ── internal ──────────────────────────────────────────────────────────────────

void DragSpinBox::set_value_clamped(double v)
{
    double clamped = qBound(m_min, v, m_max);
    if (clamped == m_value) return;
    m_value = clamped;
    emit valueChanged(m_value);
}