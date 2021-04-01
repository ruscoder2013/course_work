#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDir>
#include <QFileDialog>
#include <QStringRef>

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
    QString getName() {
        return name;
    }
    void setName(QString name) {
        this->name = name;
    }
    QByteArray getImage() {
        return inByteArray;
    }
    void setImage(QByteArray arr) {
        inByteArray = arr;
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
  QString name;
  QByteArray inByteArray;
};
Q_DECLARE_METATYPE( MyListWidgetItem )

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    sdb = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE"));
        
    ui->listWidget->setViewMode(QListView::IconMode);
    ui->listWidget->setIconSize(QSize(160,160));
        
    ui->label->setScaledContents(true);

    connect(ui->btnNextPage,SIGNAL(clicked()),this,SLOT(next()));
    //connect(ui->pushButton_2, SIGNAL(clicked()), this, SLOT(copy()));
    connect(ui->listWidget, &QListWidget::itemClicked, this, &MainWindow::bookCoverClicked);
    connect(ui->btnAddCat, SIGNAL(clicked()), this, SLOT(addCat()));
    connect(ui->btnSetCat, SIGNAL(clicked()), this, SLOT(setCat()));
    connect(ui->btnSetFolder, SIGNAL(clicked()), this, SLOT(setFolder()));
    connect(ui->btnSaveDb, SIGNAL(clicked()), this, SLOT(saveDb()));
    connect(ui->btnOpenDb, SIGNAL(clicked()), this, SLOT(openDb()));
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
    ui->label->setPixmap(pixmap);
}

void MainWindow::bookCoverClicked(QListWidgetItem *item)
{
    MyListWidgetItem* pitem = dynamic_cast<MyListWidgetItem*>(item);

    document = new DJVU;
    document->openDocument(pitem->getPath());
    currentBookPath = pitem->getPath();
    QImage *img = document->getImage();
    auto pixmap = QPixmap::fromImage(*img);
    ui->label->setPixmap(pixmap);
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

void MainWindow::setFolder()
{

    currentDir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                 "/home",
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);
    QDir myDir(currentDir);
    QStringList filesList = myDir.entryList(QStringList("*.djvu"));
    MyListWidgetItem* pitem;

    foreach(QString str, filesList) {
        document = new DJVU;
        document->openDocument(currentDir + "/" + str);
        QImage *img = document->getImage();
        QByteArray inByteArray;
        QBuffer inBuffer( &inByteArray );
        inBuffer.open( QIODevice::WriteOnly );
        auto p_map = QPixmap::fromImage(*img);
        p_map.save( &inBuffer, "PNG" );
        
        pitem = new MyListWidgetItem(str.mid(0, 10), ui->listWidget);

        pitem->setIcon(QPixmap::fromImage(*img));
        pitem->setPath(currentDir + "/" + str);
        pitem->setName(str);
        pitem->setImage(inByteArray);

        pitem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable |
        Qt::ItemIsEditable | Qt::ItemIsDragEnabled);
    }

}

void MainWindow::saveDb()
{
    QString filename = QFileDialog::getSaveFileName();
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");//not dbConnection
    db.setDatabaseName(filename);
    db.open();
    QSqlQuery query;
    query.exec("CREATE TABLE 'Books' "
               "( `id` INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE, "
               "`Name` TEXT, "
               "`Path` TEXT, "
               "`Image` BLOB, "
               "`Importance` TEXT, "
               "`Interest` TEXT, "
               "`Comment` TEXT, "
               "`ContentPage` INTEGER, "
               "`InterestingPages` TEXT, "
               "`SessionNumber` INTEGER )");

    QSqlQuery my_query;
    my_query.prepare("INSERT INTO Books (Name, Path, Image)"
                                  "VALUES (:name, :path, :img);");

    for (auto item : ui->listWidget->findItems("*", Qt::MatchWildcard)){
        auto my_item = dynamic_cast<MyListWidgetItem*>(item);
         my_query.bindValue(":name", my_item->getName());
         my_query.bindValue(":path", my_item->getPath());
         my_query.bindValue(":img", my_item->getImage());

         if (!my_query.exec())
         {
              qDebug() << db.lastError().text();
         }
    }

}

void MainWindow::openDb()
{
    QSqlQuery query;
    MyListWidgetItem* pitem;

    QString fileName = QFileDialog::getOpenFileName(this,
            tr("Open Address Book"), "");

    sdb->setDatabaseName(fileName);
    if (!sdb->open()) {    }

    if( !query.exec( "SELECT id, Name, Image, Path from Books" ))
              qDebug() << "Error getting image from table:\n" << query.lastError();
   while (query.next()) {
       int id = query.value(0).toInt();
       QString name = query.value(1).toString();
       QByteArray outByteArray = query.value(2).toByteArray();
       QString path = query.value(3).toString();

       QPixmap outPixmap = QPixmap();
       outPixmap.loadFromData( outByteArray );
       pitem = new MyListWidgetItem(name.mid(0, 10), ui->listWidget);
       pitem->setPath(path);
       pitem->setId(id);
       pitem->setIcon(outPixmap);
       pitem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable |
       Qt::ItemIsEditable | Qt::ItemIsDragEnabled);
   }
   sdb->close();
}
