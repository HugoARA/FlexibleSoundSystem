#ifndef CIMAGEVIEWER_H
#define CIMAGEVIEWER_H
#pragma once

#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <vector>

struct SArtwork;

class CImageViewer : public QGraphicsView
{
    Q_OBJECT

public:
    explicit CImageViewer(QWidget* parent = Q_NULLPTR);

    void setImage(const SArtwork& image);
private:
    const QString m_sdefault;
    QGraphicsScene m_graphics_scene;
    QGraphicsPixmapItem m_pixmap_item;
    QImage m_idefault;
};

#endif // CIMAGEVIEWER_H
