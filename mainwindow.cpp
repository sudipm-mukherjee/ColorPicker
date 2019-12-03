#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QLabel>
#include <QListWidget>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QFile>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle(qAppName());

    settings.setObjectName("settings");
    setting_path =  QStandardPaths::writableLocation(QStandardPaths::DataLocation);

    if(!QDir(setting_path).exists()){
        QDir d(setting_path);
        d.mkpath(setting_path);
    }


    colorDialog = new ColorDialog(this,QColor("green"));
    ui->colorWidgetLayout->addWidget(colorDialog);
    ui->saved->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->saved->verticalHeader()->setVisible(false);

    connect(colorDialog,&ColorDialog::currentColorChanged,[=](const QColor color){
        ui->argb->setText(color.name(QColor::HexArgb));
        ui->cmyk->setText(getCMYK(color));
    });
    colorDialog->setCurrentColor(QColor("red"));//TODO load color from last session

    foreach (QPushButton*btn, colorDialog->findChildren<QPushButton*>())    {
        if(btn->text().contains("Pick Screen Color")){
            btn->setIconSize(QSize(18,18));
            btn->setIcon(QIcon(":/dark/picker.png"));
        }
        if(btn->text().contains("Add to Custom Colors")){
            btn->setIconSize(QSize(18,18));
            btn->setIcon(QIcon(":/dark/plus.png"));
            btn->setText("Add to Temporary Colors");
        }
    }

    foreach (QLabel*label, colorDialog->findChildren<QLabel*>())    {
        label->setText(label->text().replace("&",""));
        if(label->text().contains("Custom colors")){
            label->setAlignment(Qt::AlignCenter);
            label->setText("Click \"Add to Temporary Colors\" to save\ncolor for this session only. \nor\n"
                           "Drag a color to save it permanently.");
        }
    }

    foreach (QLineEdit*ledit, colorDialog->findChildren<QLineEdit*>())    {
        qDebug()<<ledit->text();
        if(ledit->text()=="#ff0000"){
            ledit->setMaximumWidth(40000);
        }
    }

    load_saved_colors();
    this->adjustSize();
}

void MainWindow::setStyle(QString fname)
{   QFile styleSheet(fname);
    if (!styleSheet.open(QIODevice::ReadOnly)) {
        qWarning("Unable to open file");
    return; }
    qApp->setStyleSheet(styleSheet.readAll());
    styleSheet.close();
}



MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::load_saved_colors(){
    QFile file(setting_path+"/saved.colors");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
      return;

    QTextStream in(&file);
    while (!in.atEnd()) {
      QString line = in.readLine();
      qDebug()<<line;
      add_to_table(line,false);
    }
    file.close();
}

void MainWindow::add_to_table(const QString colorStr,bool saving){
    QColor color = QColor(colorStr);
    if(color.isValid()){
        int nextRow;
        int row = ui->saved->rowCount();
        if(row==0){
            nextRow = 0;
        }else{
            nextRow = row++;
        }
        if(saving){
            QColor color =colorDialog->currentColor();
            //save color to saved.colors file
            save_color(color);
        }
        //data
        QString html =  color.name();
        QString hsv =   getHSV(color);
        QString hexArgb = color.name(QColor::HexArgb);
        QString rgb = getRGB(color);
        QString cymk = getCMYK(color);

        QStringList columnData;
        columnData<<html<<hexArgb<<hsv<<rgb<<cymk<<"color"<<"delete";

        qDebug()<<columnData;
        //insertRow
        ui->saved->insertRow(nextRow);
        //add column
        for (int i = 0; i < columnData.count(); i++) {

            if(columnData.at(i)=="color"){
                QPushButton *color = new QPushButton(0);
                color->setStyleSheet("background-color:"+html+";border:0px;");
                connect(color,&QPushButton::clicked,[=](){
                    colorDialog->setCurrentColor(QColor(html));
                });
                ui->saved->setCellWidget(nextRow,i,color);
                this->update();
            }else if(columnData.at(i)=="delete"){
                QPushButton *del = new QPushButton("Delete",0);
                del->setIcon(QIcon(":/dark/delete.png"));
                del->setStyleSheet("border:0px;");
                connect(del,&QPushButton::clicked,[=](){
                    ui->saved->removeRow(ui->saved->rowAt(del->y()));//ui.saved.removerow(nextRow);
                });
                ui->saved->setCellWidget(nextRow,i,del);
                ui->saved->viewport()->resize(ui->saved->viewport()->width(),ui->saved->viewport()->height()+1);
                ui->saved->viewport()->resize(ui->saved->viewport()->width(),ui->saved->viewport()->height()-1);
            }else{
                ui->saved->setItem(nextRow, i,
                                    new QTableWidgetItem(columnData.at(i)));
            }
        }
    }else{
        invalidColor();
    }
}

void MainWindow::save_color(const QColor color){
    QFile file(setting_path+"/saved.colors");
    if (!file.open(QIODevice::Append | QIODevice::Text))
      return;
    QTextStream out(&file);
    out << color.name(QColor::HexArgb)<< "\n";
    file.close();
}

void MainWindow::on_save_clicked()
{
    QColor color =colorDialog->currentColor();
    add_to_table(color.name(QColor::HexArgb),true);
}

QString MainWindow::getHSV(const QColor color){
    int h = color.convertTo(QColor::Hsv).hsvHue();
    int s = color.convertTo(QColor::Hsv).hsvSaturation();
    int v = color.convertTo(QColor::Hsv).value();
    return QString::number(h)+" "+QString::number(s)+" "+QString::number(v);
}

QString MainWindow::getRGB(const QColor color){
    int r = color.convertTo(QColor::Rgb).red();
    int g = color.convertTo(QColor::Rgb).green();
    int b = color.convertTo(QColor::Rgb).blue();
    return QString::number(r)+","+QString::number(g)+","+QString::number(b);
}

QString MainWindow::getCMYK(const QColor color){
    int c = color.convertTo(QColor::Cmyk).cyan();
    int m = color.convertTo(QColor::Cmyk).magenta();
    int y = color.convertTo(QColor::Cmyk).yellow();
    int k = color.convertTo(QColor::Cmyk).black();
    return QString::number(c)+","+QString::number(m)+","+QString::number(y)+","+QString::number(k);
}

void MainWindow::on_saved_cellClicked(int row, int column)
{

    if(ui->saved->item(row,column)==nullptr)
        return;
    QString colorname;

    colorname = ui->saved->item(row,column)->text();


    QColor color(colorname);

    int x,y,z,k;
    if(colorname.contains(",")){
        QStringList val = colorname.split(",");
        if(val.count()==4){
            x= val.at(0).toInt();
            y= val.at(1).toInt();
            z= val.at(2).toInt();
            k= val.at(3).toInt();
            color = QColor::fromCmyk(x,y,z,k);
            qDebug()<<color.isValid();
        }else{
            x= val.at(0).toInt();
            y= val.at(1).toInt();
            z= val.at(2).toInt();
            color = QColor::fromRgb(x,y,z);
        }

    }else if(colorname.contains(" ")){
        QStringList val = colorname.split(" ");
        x= val.at(0).toInt();
        y= val.at(1).toInt();
        z= val.at(2).toInt();
        color = QColor::fromHsv(x,y,z);
    }
    if(color.isValid()){
        colorDialog->setCurrentColor(color);
    }else{
        invalidColor();
    }
}

void MainWindow::invalidColor(){
    QMessageBox msgBox;
    msgBox.setText("Selected color code is invalid.");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();
}


void MainWindow::on_actionSettings_triggered()
{

}