#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDir>

#include <QDebug>

template< typename T >
T unpack( const QVariant& var, const T& defVal = T() ) {
    if( var.isValid() && var.canConvert< T >() ) {
        return var.value< T >();
    }
    return defVal;
}

class MyListWidgetItem : public QListWidgetItem
{
public:
    MyListWidgetItem(): id(0), path("") {}

    QString getPath() const {
        return path;
    }
    void setPath(QString path) {
        this->path = path;
    }
    int getId() const {
        return id;
    }
    void setId(int id) {
        this->id = id;
    }
    MyListWidgetItem(QListWidget *parent) :
      QListWidgetItem(parent)
    {
    }

    MyListWidgetItem(QString content, QListWidget *parent) :
      QListWidgetItem(content, parent)
    {
    }
private:
  QString path;
  int id;
};
Q_DECLARE_METATYPE( MyListWidgetItem )

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    sdb = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE"));
    sdb->setDatabaseName("test");
    if (!sdb->open()) {
           //....
    }
    tbvCat = new QTableView();
    modelCat = new QSqlQueryModel();
    modelCat->setQuery("SELECT id, name "
    "FROM Сategories "
    );

    if (modelCat->lastError().isValid()) {
        qDebug() << modelCat->lastError();
    }

    ui->tbvCat->setModel(modelCat);
    ui->tbvCat->hideColumn(0);
    ui->tbvCat->show();

    QDir myDir("/home/andrew/book");
    QStringList filesList = myDir.entryList(QStringList("*.djvu"));
    ui->listWidget->setIconSize(QSize(300, 300));
    MyListWidgetItem* pitem;
    //lwg.setSelectionMode(QAbstractItemView::MultiSelection);
    ui->listWidget->setViewMode(QListView::IconMode);
    QSqlQuery my_query, query;
    my_query.prepare("INSERT INTO Books (Name, Path, Image)"
                                  "VALUES (:name, :path, :img);");
    /*foreach(QString str, filesList) {
        document = new DJVU;
        document->openDocument("/home/andrew/book/"+str);
        QImage *img = document->getImage();
        QByteArray inByteArray;
        QBuffer inBuffer( &inByteArray );
        inBuffer.open( QIODevice::WriteOnly );
        auto p_map = QPixmap::fromImage(*img);
        p_map.save( &inBuffer, "PNG" );

        pitem = new QListWidgetItem("str", ui->listWidget);
        pitem->setIcon(QPixmap::fromImage(*img));
        pitem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable |
        Qt::ItemIsEditable | Qt::ItemIsDragEnabled);

        my_query.bindValue(":name", str);
        my_query.bindValue(":path", "/home/andrew/book/"+str);
        my_query.bindValue(":img", inByteArray);
        //my_query.
        if (!my_query.exec())
        {
             qDebug() << sdb.lastError().text();
        }
    }*/

    if( !query.exec( "SELECT id, Image, Path from Books" ))
           qDebug() << "Error getting image from table:\n" << query.lastError();
    while (query.next()) {
        int id = query.value(0).toInt();
        QByteArray outByteArray = query.value(1).toByteArray();
        QString path = query.value(2).toString();

        QPixmap outPixmap = QPixmap();
        outPixmap.loadFromData( outByteArray );
        pitem = new MyListWidgetItem("str", ui->listWidget);
        pitem->setPath(path);
        pitem->setId(id);
        pitem->setIcon(outPixmap);
        pitem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable |
        Qt::ItemIsEditable | Qt::ItemIsDragEnabled);
    }


    document = new DJVU;
    document->openDocument("test.djvu");
    QImage *img = document->getImage();
    //document->nextPage();
    auto pixmap = QPixmap::fromImage(*img);
    int width = ui->label->width();
    int height = ui->label->height();
    ui->label->setPixmap(pixmap.scaled(width, height, Qt::KeepAspectRatio));

    //label->setPixmap(p.scaled(w,h,Qt::KeepAspectRatio));

    connect(ui->pushButton,SIGNAL(clicked()),this,SLOT(next()));
    connect(ui->pushButton_2, SIGNAL(clicked()), this, SLOT(copy()));
    connect(ui->listWidget, &QListWidget::itemClicked, this, &MainWindow::bookCoverClicked);
    connect(ui->btnAddCat, SIGNAL(clicked()), this, SLOT(addCat()));
    connect(ui->btnSetCat, SIGNAL(clicked()), this, SLOT(setCat()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::next()
{
    ui->label->clear();
    document->nextPage();
    QImage *img = document->getImage();
    auto pixmap = QPixmap::fromImage(*img);

    int width = ui->label->width();
    int height = ui->label->height();
    ui->label->setPixmap(pixmap.scaled(width, height, Qt::KeepAspectRatio));

}

void MainWindow::bookCoverClicked(QListWidgetItem *item)
{
    MyListWidgetItem* pitem = dynamic_cast<MyListWidgetItem*>(item);
    ui->label_2->setText(pitem->getPath());

    document = new DJVU;
    document->openDocument(pitem->getPath());
    currentBookPath = pitem->getPath();
    QImage *img = document->getImage();
    //document->nextPage();
    auto pixmap = QPixmap::fromImage(*img);
    int width = ui->label->width();
    int height = ui->label->height();
    ui->label->setPixmap(pixmap.scaled(width, height, Qt::KeepAspectRatio));
}

void MainWindow::copy()
{
    QFile::copy(currentBookPath, currentBookPath+"copy");
}

void MainWindow::addCat()
{
    QSqlQuery my_query;
    QString category = ui->leCat->text();
    my_query.prepare("INSERT INTO Сategories (name)"
                                  "VALUES (:name);");
    my_query.bindValue(":name", category);
    if (!my_query.exec())
    {
         qDebug() << sdb->lastError().text();
    }
    modelCat->setQuery("SELECT id, name "
    "FROM Сategories ");
}

void MainWindow::setCat()
{
    QSqlQuery my_query;
    my_query.prepare("INSERT INTO СategoriesBook (book_id, categories_id)"
                                  "VALUES (:book_id, categories_id);");
    int b_id;
    int cat_id;
    for(auto item : ui->listWidget->selectedItems())
    {
        QVariant var = QVariant::fromValue( item->data(0) );
        b_id = unpack< MyListWidgetItem >( var ).getId();
    }
    auto indexes = ui->tbvCat->selectionModel()->selectedIndexes();
    for (auto index: indexes){
        modelCat->data(index).toInt();

        //cat_id = selectedRows[i].data().toInt();
        my_query.bindValue(":book_id", cat_id);
        my_query.bindValue(":categories_id", b_id);
        if (!my_query.exec())
        {
             qDebug() << sdb->lastError().text();
        }
    }
}
