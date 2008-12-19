#include "casteralertlistenerimpl.h"


CasterAlertListenerImpl::CasterAlertListenerImpl( QWidget * parent, Qt::WFlags f) : QMainWindow(parent, f), sockM(NULL), mediaObject(NULL), audioOutput(NULL)
{
    setupUi(this);

    // Read settings
    readSettings();

    connect(startButton, SIGNAL(clicked()), this, SLOT(startListenning()));
    connect(stopButton, SIGNAL(clicked()),this,SLOT(stopListenning()),Qt::BlockingQueuedConnection);

}

void CasterAlertListenerImpl::startListenning()
{
    sockM = new QUdpSocket(this);

    portM = 22512;
    addrM = "225.1.2.3";
    sockM->bind(this->portM, QUdpSocket::ReuseAddressHint);  // Surcharge de bind, prend QHostAddress::Any

    // S'abonner a l adresse multicast ! (Qt ne gère pas, il faut appeler les méthode natives mais qd meme standard)
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr((const char*)addrM.toString().toStdString().c_str());
    mreq.imr_interface.s_addr = INADDR_ANY;
    int r = ::setsockopt(sockM->socketDescriptor(), IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char *)&mreq, sizeof(struct ip_mreq));
    qDebug("setsockopt returned %d", r);

    connect(sockM, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));
    stopButton->setEnabled(true);
}

void CasterAlertListenerImpl::stopListenning()
{
    sockM->abort();
}

void CasterAlertListenerImpl::readPendingDatagrams()
{
    //qDebug() << "Receiving datagram";

    while (sockM->hasPendingDatagrams())
    {
        QByteArray datagram;
        QHostAddress sender;
        quint16 senderPort;

        datagram.resize(sockM->pendingDatagramSize());
        sockM->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        CasterAlert ca;
        QDataStream in(&datagram, QIODevice::ReadOnly);
        in >> ca;

        infoConsole->appendPlainText("From : " + ca.getFrom());

        if (checkConcern(ca))
        {
            infoConsole->appendPlainText("Ca nous concerne !!");
            performAlert(ca);
        }
        else
            infoConsole->appendPlainText("C'est pas pour nous");

        //qDebug() << datagram.data();
        //qDebug() << sender;
        //qDebug() << senderPort << endl;
    }
}

void CasterAlertListenerImpl::performAlert(const CasterAlert &ca)
{
    if (actionSound->isChecked())
        playSound();
    else if (actionVisual->isChecked())
        return;
    else
        QApplication::beep();
}

bool CasterAlertListenerImpl::checkConcern(const CasterAlert &ca) const
{
     QList<QListWidgetItem *> selectedUsers = userList->selectedItems();
     QList<QString> to = ca.getTo();

    for(int i=0; i < selectedUsers.size(); i++)
    {
        if ( to.contains(selectedUsers.at(i)->text()) )
            return true;
    }

    return false;
}

void CasterAlertListenerImpl::playSound()
{
    if (mediaObject == NULL)
    {
        mediaObject = new Phonon::MediaObject(this);
        audioOutput = new Phonon::AudioOutput(Phonon::MusicCategory, this);
        Phonon::createPath(mediaObject, audioOutput);
    }

    mediaObject->setCurrentSource(Phonon::MediaSource("../sound/Gong3s.ogg"));    
    mediaObject->play();
}

void CasterAlertListenerImpl::on_addUserButton_clicked()
{
    QString user = QInputDialog::getText(this, trUtf8("Username"), trUtf8("Username to add to the list :               "), QLineEdit::Normal, trUtf8("username"));

    if ( user != "" )
        userList->addItem(user);
    // TODO : check if username not already in and Warning in both case?

}

void CasterAlertListenerImpl::on_removeUserButton_clicked()
{
    // TODO : doesn't work
    // for (int i=0; i<userList->selectedItems().size(); i++)
    //   userList->removeItemWidget(userList->selectedItems().at(i));
}

void CasterAlertListenerImpl::readSettings()
{
    QSettings settings;

    actionSound->setChecked(settings.value("alert/sound", false).toBool());
    actionVisual->setChecked(settings.value("alert/visual", false).toBool());
}

void CasterAlertListenerImpl::writeSettings()
{
    QSettings settings;

    settings.setValue("alert/sound", actionSound->isChecked());
    settings.setValue("alert/visual", actionVisual->isChecked());
}

 void CasterAlertListenerImpl::closeEvent(QCloseEvent *event)
 {
    writeSettings();
    event->accept();
 }