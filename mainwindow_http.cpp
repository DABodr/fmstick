/*
 * Part of FmStick, see LICENSE.txt.
 * Modified 2026-11-25 by Aurel: port to Qt6.
 */
#include "mainwindow.h"
#include "querier.h"
#include "ui_mainwindow.h"

#include "fmtx.h"
#include "logging.h"

#include "requestmapper.h"
#include "httplistener.h"

/******************************************************************
 * HTTP server staff
 ******************************************************************/
void MainWindow::updateHTTPListenerAccess()
{
    http_admin_login=ui->lineEdit_sw_remote_login->text();
    http_admin_pwd=ui->lineEdit_sw_remote_pwd->text();
}

bool MainWindow::checkHTTPListenerPort()
{
    bool ret=false;
    int port = ui->spinBox_sw_remote_port->value();
    QTcpServer *test_server = new QTcpServer();
    test_server->listen(QHostAddress::Any, port);
    if (!test_server->isListening()) {
        //qCritical("HttpListener: Cannot bind on port %i: %s",port,qPrintable(errorString()));
        qDebug("HttpListener: Cannot bind on port %i",port);
        ui->spinBox_sw_remote_port->setStyleSheet("background-color: rgb(255,127,127);");
        ui->textBrowser_sw_remote_uri->setHtml("<font color=red>this port is not avaliable</font>");
    } else {
        ui->spinBox_sw_remote_port->setStyleSheet("background-color: rgb(127,255,127);");
        qDebug("HttpListener: Listening on port %i",port);
        ui->textBrowser_sw_remote_uri->setHtml("<font color=red>this port is avaliable</font>");
        ret=true;
        //ui->label_check_conn->setText("");
    }
    test_server->close();
    test_server->~QTcpServer();
    return ret;
}

void MainWindow::updateHTTPListener()
{
    if(listener) {
        listener->close();
        listener->~HttpListener();
        listener=NULL;
    }

    if(ui->checkBox_sw_remote->isChecked()) {
        ui->frame_sw_remoteaccesss->setVisible(true);
        if(checkHTTPListenerPort()) {
            QSettings* web_cfg=new QSettings();
            web_cfg->setValue("port",ui->spinBox_sw_remote_port->value());
            /*web_cfg->setValue("minThreads", "1");
            web_cfg->setValue("maxThreads", "10");
            web_cfg->setValue("cleanupInterval", "1000");
            web_cfg->setValue("readTimeout", "60000");
            web_cfg->setValue("maxRequestSize", "16000");
            web_cfg->setValue("maxMultiPartSize", "1000000");*/
            RequestMapper *req_mapper = new RequestMapper();
            req_mapper->mw_ptr = this;
            listener=new HttpListener(web_cfg,req_mapper,this);

            /* getting IP-address */
            QString IP_address = "127.0.0.1";
            // QNetworkSession was removed in Qt6: take the first non-loopback IPv4 address
            const QList<QHostAddress> laddr = QNetworkInterface::allAddresses();
            for (const QHostAddress &address : laddr)
            {
                if(address.protocol() == QAbstractSocket::IPv4Protocol && !address.isLoopback()) {
                    IP_address = address.toString();
                    break;
                }
            }


            ui->textBrowser_sw_remote_uri->setHtml(
                        QString("Web-UI link: <a href=\"http://%1:%2/\" target=\"_blank\">http://%1:%2/</a>")
                        .arg(IP_address)
                        .arg(ui->spinBox_sw_remote_port->value())
                        );
            //ui->textBrowser_sw_remote_uri->setReadOnly();
            connect( req_mapper, SIGNAL(updateItem(QObject *, QString *)),
                     this, SLOT(updateItemFromHTTPListener(QObject *, QString *)) );
        }
    } else {
        ui->frame_sw_remoteaccesss->setVisible(false);
    }
}

bool MainWindow::updateItemFromHTTPListener(QObject *obj, QString *value)
{
    QString objTypeName = obj->metaObject()->className();
    if(!value) value = new QString("");
    qDebug() << "[MAIN] object class name:"  << objTypeName << ", name: " << obj->objectName() << "value:" << *value;

    if(objTypeName == "QPlainTextEdit") {
        QPlainTextEdit *item = (QPlainTextEdit *) obj;
        if(item->isReadOnly()){
            qDebug() << "[MAIN] trying to set read-only item " << objTypeName << " with name " << obj->objectName();
            return false;
        }

        if(!item->isEnabled()){
            qDebug() << "[MAIN] trying to set disabled item " << objTypeName << " with name " << obj->objectName();
            return false;
        }
        QTextDocument doc;
        doc.setHtml( QString(*value) );
        item->setPlainText(doc.toPlainText());
    }
    else if(objTypeName == "QLineEdit") {
        QLineEdit *item = (QLineEdit *) obj;
        if(item->isReadOnly()){
            qDebug() << "[MAIN] trying to set read-only item " << objTypeName << " with name " << obj->objectName();
            return false;
        }

        if(!item->isEnabled()){
            qDebug() << "[MAIN] trying to set disabled item " << objTypeName << " with name " << obj->objectName();
            return false;
        }

        item->setText(QString(*value));
    }
    else if(objTypeName == "QCheckBox") {
        QCheckBox *item = (QCheckBox *) obj;
        if(!item->isEnabled()){
            qDebug() << "[MAIN] trying to set disabled item " << objTypeName << " with name " << obj->objectName();
            return false;
        }

        item->setChecked(QString(*value) == "" ? false : true);
    }
    else if(objTypeName == "QRadioButton") {
        QRadioButton *item = (QRadioButton *) obj;
        if(!item->isEnabled()){
            qDebug() << "[MAIN] trying to set disabled item " << objTypeName << " with name " << obj->objectName();
            return false;
        }

        item->setChecked(QString(*value) == "" ? false : true);
    }
    else if(objTypeName == "QSpinBox") {
        QSpinBox *item = (QSpinBox *) obj;
        if(item->isReadOnly()){
            qDebug() << "[MAIN] trying to set read-only item " << objTypeName << " with name " << obj->objectName();
            return false;
        }

        if(!item->isEnabled()){
            qDebug() << "[MAIN] trying to set disabled item " << objTypeName << " with name " << obj->objectName();
            return false;
        }

        item->setValue(QString(*value).toInt());
    }
    else if(objTypeName == "QDoubleSpinBox") {
        QDoubleSpinBox *item = (QDoubleSpinBox *) obj;
        if(item->isReadOnly()){
            qDebug() << "[MAIN] trying to set read-only item " << objTypeName << " with name " << obj->objectName();
            return false;
        }

        if(!item->isEnabled()){
            qDebug() << "[MAIN] trying to set disabled item " << objTypeName << " with name " << obj->objectName();
            return false;
        }

        item->setValue(QString(*value).toDouble());
    }
    else if(objTypeName == "QComboBox") {
        QComboBox *item = (QComboBox *) obj;
        if(!item->isEnabled()){
            qDebug() << "[MAIN] trying to set disabled item " << objTypeName << " with name " << obj->objectName();
            return false;
        }

        item->setCurrentIndex(QString(*value).toInt());
    }
    else if(objTypeName == "QPushButton") {
        QPushButton *item = (QPushButton *) obj;
        if(!item->isEnabled()){
            qDebug() << "[MAIN] trying to set disabled item " << objTypeName << " with name " << obj->objectName();
            return false;
        }
        if(item->objectName() == "pushButton_sw_send_custom_rds")
            SetRDSCustomGroup(value);
        else
            item->click();
    }
    else if(objTypeName == "QAction") {
        QAction *item = (QAction *) obj;
        if(!item->isEnabled()){
            qDebug() << "[MAIN] trying to set disabled item " << objTypeName << " with name " << obj->objectName();
            return false;
        }

        /* save params for http access */
        if(item->objectName() == "actionReset_to_Default") {
            bool http_serve = ui->checkBox_sw_remote->isChecked();
            int http_port = ui->spinBox_sw_remote_port->value();
            QString http_login(ui->lineEdit_sw_remote_login->text());
            QString http_password(ui->lineEdit_sw_remote_pwd->text());

            item->trigger();

            ui->checkBox_sw_remote->setChecked(http_serve);
            ui->spinBox_sw_remote_port->setValue(http_port);
            ui->lineEdit_sw_remote_login->setText(http_login);
            ui->lineEdit_sw_remote_pwd->setText(http_password);
        } else {
            item->trigger();
        }
    }
    else {
        qDebug() << "[MAIN] unimplemented method for " << objTypeName;
        return false;
    }

    qDebug() << "[MAIN] done";
    return true;
}


