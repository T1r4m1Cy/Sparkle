#pragma once

#include <QWidget>

class QLineEdit;

// A numeric field that changes value by clicking and dragging horizontally.
// Double-click switches to text input mode.
// Hold Ctrl for fine control (10x slower), Shift for coarse (10x faster).
class DragSpinBox : public QWidget {
    Q_OBJECT
public:
    explicit DragSpinBox(QWidget* parent = nullptr);

    void   setValue(double value);
    double value() const { return m_value; }

    void setRange(double min, double max);
    void setDecimals(int decimals);
    void setSensitivity(double pixelsPerUnit);
    void setReadOnly(bool ro);

signals:
    void valueChanged(double value);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void enterEvent(QEnterEvent*) override;
    void leaveEvent(QEvent*) override;
    void focusOutEvent(QFocusEvent*) override;
    QSize sizeHint() const override;

private:
    void start_edit();
    void commit_edit();
    void set_value_clamped(double v);

    double m_value      = 0.0;
    double m_min        = -1e9;
    double m_max        =  1e9;
    double m_sensitivity = 0.1;  // value units per pixel dragged
    int    m_decimals   = 3;
    bool   m_readOnly   = false;
    bool   m_hovered    = false;

    bool   m_dragging        = false;
    QPoint m_dragCenter;       // global pos where drag started; cursor is warped back here each frame
    double m_dragStartValue  = 0.0;

    QLineEdit* m_lineEdit = nullptr;
};
