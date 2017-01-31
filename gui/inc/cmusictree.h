#ifndef CMUSICTREE_H
#define CMUSICTREE_H

#include <QTreeView>

class QFileSystemModel;

class CMusicTree : public QTreeView
{
    Q_OBJECT

public:
    explicit CMusicTree(QWidget* parent = Q_NULLPTR);

    void moveUp();
    void moveDown();
    std::string select();
private slots:
    void handleActivation(const QModelIndex&);
private:
    const QString m_path;
    QFileSystemModel* m_file_system_model;

    void setup();
};

#endif // CMUSICTREE_H
