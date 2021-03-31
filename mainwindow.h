#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "djvu.h"
#include <QListWidget>
#include <QtSql>
#include <QTableView>

namespace Ui {
class MainWindow;
}


class Model : public QSqlQueryModel
{
    Q_OBJECT
public:
    // Перечисляем все роли, которые будут использоваться в TableView
    enum Roles {
        DateRole = Qt::UserRole + 1,    // дата
        TimeRole,                       // время
        RandomRole,                     // псевдослучаное число
        MessageRole                     // сообщение
    };
    explicit Model(QObject *parent = 0) : QSqlQueryModel(parent)
    {
    }
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const  {
        int columnId = role - Qt::UserRole - 1;
        QModelIndex modelIndex = this->index(index.row(), columnId);
        return QSqlQueryModel::data(modelIndex, Qt::DisplayRole);
    }
protected:
    QHash<int, QByteArray> roleNames() const
    {
        QHash<int, QByteArray> roles;
        roles[DateRole] = "date";
        roles[TimeRole] = "time";
        roles[RandomRole] = "random";
        roles[MessageRole] = "message";
        return roles;
    }
};


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


private:
    Ui::MainWindow *ui;
    DJVU *document;
    QString currentBookPath;
    QSqlDatabase *sdb;
    QTableView *tbvCat;
    QSqlQueryModel* modelCat;
public slots:
    void next();
    void bookCoverClicked(QListWidgetItem *item);
    void copy();
    void addCat();
    void setCat();
};

#endif // MAINWINDOW_H
