#include "cmusictree.h"

#include <QFileSystemModel>
#include <QHeaderView>

#include <QDebug>

CMusicTree::CMusicTree(QWidget* parent)
    : QTreeView(parent),
      m_path("/run/media/hugoa/Music/Music/Dubstep/"),
      //m_path("/media/"),
      m_file_system_model(new QFileSystemModel(this))
{
    setup();
    currentIndex();
}

void CMusicTree::setup() {
    m_file_system_model->setRootPath(m_path);
    m_file_system_model->setFilter(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files);
    m_file_system_model->setNameFilters(QStringList() << "*.mp3");
    m_file_system_model->setNameFilterDisables(false);

    setModel(m_file_system_model);
    setRootIndex(m_file_system_model->index(m_path));

    connect(this, &CMusicTree::activated, this, &CMusicTree::handleActivation);

    hideColumn(1);
    hideColumn(2);
    hideColumn(3);
}

void CMusicTree::handleActivation(const QModelIndex& model_index) {
    if (m_file_system_model->isDir(model_index)) {
        if (isExpanded(model_index)) {
            collapse(model_index);
        } else {
            expand(model_index);
        }
    } else {
        QString music_path = m_file_system_model->filePath(model_index);
        qDebug() << music_path;
    }
}

std::string CMusicTree::select() {
    QModelIndex model_index = currentIndex();

    if (m_file_system_model->isDir(model_index)) {
        if (isExpanded(model_index)) {
            collapse(model_index);
        } else {
            expand(model_index);
        }
    } else {
        QString music_path = m_file_system_model->filePath(model_index);
        qDebug() << music_path;
        return music_path.toStdString();
    }
    return "";
}

void CMusicTree::moveUp() {
    QModelIndex model_index = moveCursor(MoveUp, Qt::KeypadModifier);

    qDebug() << m_file_system_model->filePath(model_index);

    setCurrentIndex(model_index);
}

void CMusicTree::moveDown() {
    QModelIndex model_index = moveCursor(MoveDown, Qt::KeypadModifier);

    qDebug() << m_file_system_model->filePath(model_index);

    setCurrentIndex(model_index);
}
