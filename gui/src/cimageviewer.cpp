#include "cimageviewer.h"

#include "gui.h"

#define DEFAULT_ARTWORK "/home/hugoa/fss_images/default.jpg"

CImageViewer::CImageViewer(QWidget *parent)
    : QGraphicsView(parent), m_sdefault(DEFAULT_ARTWORK),
      m_idefault(m_sdefault)
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_pixmap_item.setPixmap(QPixmap::fromImage(m_idefault.scaled(this->width(), this->height(), Qt::KeepAspectRatio)));
    m_graphics_scene.addItem(&m_pixmap_item);
    setScene(&m_graphics_scene);
}

void CImageViewer::setImage(const SArtwork& image) {
    if (image.m_raw_buffer.empty()) {
        // select default artwork
        m_pixmap_item.setPixmap(QPixmap::fromImage(m_idefault.scaled(this->width(), this->height(), Qt::KeepAspectRatio)));
    } else {
        QPixmap pixmap;
        pixmap.loadFromData(&image.m_raw_buffer[0], image.m_raw_buffer.size());
        m_pixmap_item.setPixmap(pixmap.scaled(this->width(), this->height(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}
