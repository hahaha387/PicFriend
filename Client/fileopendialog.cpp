
#include "fileopendialog.h"


#include "client.h"
#include "clientInfo.h"

#include <QBuffer>
#include <QApplication>
#include <QDebug>
#include <QQuickWindow>

FileOpenDialog::FileOpenDialog(QQuickItem *parent)
    : QQuickItem(parent)
    , m_dlgHelper(init_helper())
    , m_modality(Qt::WindowModal)
    #if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
    , m_options(QFileDialogOptions::create())
    #else
    , m_options(QSharedPointer<QFileDialogOptions>(new QFileDialogOptions()))
    #endif
{
    /*
     * Qt Widgets support must be present, i.e. the main app is a QApplication.
     * The following line break at compile time, if the main app is a QGuiApplication
     */
    QApplication *appHasQtWidgetsSupport = qobject_cast<QApplication *>(QCoreApplication::instance());
    Q_ASSERT(appHasQtWidgetsSupport);
    Q_UNUSED(appHasQtWidgetsSupport);

    if (valid())
    {
        connect(m_dlgHelper, &QPlatformFileDialogHelper::accept,
                this, &FileOpenDialog::accept);
        connect(m_dlgHelper, &QPlatformFileDialogHelper::reject,
                this, &FileOpenDialog::reject);
    }
}

FileOpenDialog::~FileOpenDialog()
{
    if (m_dlgHelper)
        m_dlgHelper->hide();
    delete m_dlgHelper;
}

bool FileOpenDialog::valid() const
{
    if (m_dlgHelper) return true;
    else return false;
}

QUrl FileOpenDialog::fileUrl() const
{
    return fileUrl_;
}

QList<QUrl> FileOpenDialog::fileUrls() const
{
    return fileUrls_;
}

void FileOpenDialog::setFileUrl(QUrl fileUrl)
{
    if (fileUrl_ != fileUrl)
    {
        fileUrl_ = fileUrl;
        emit fileUrlChanged();
    }
}

void FileOpenDialog::setFileUrls(QList<QUrl> fileUrls)
{
    if (fileUrls_ != fileUrls)
    {
        fileUrls_ = fileUrls;
        emit fileUrlsChanged();
    }
}

void FileOpenDialog::publishJotting(QString content)
{
    qDebug()<<"发布笔记";
    qDebug()<<content;



    nlohmann::json jottingData;
    jottingData["content"] = content.toStdString();


    QVector<QPixmap> pics;
//    int i=0;
    for(auto &path:fileUrls_){
        QPixmap pic;
        std::string path1=path.toEncoded().toStdString();
        pic.load(QString::fromStdString(path1));
        pics.push_back(pic);

        qDebug()<<"path:"<<QString::fromStdString(path1);
        if(pic.isNull())
            qDebug()<<"图片是空的";

        //将图片进行编码后传输
        QByteArray imageData;
        QBuffer buffer(&imageData);
        pic.save(&buffer, "JPG");
        imageData = imageData.toBase64();
        QString str = imageData;
        jottingData["materials"].push_back(str.toStdString());

        qDebug()<<str;
    }

    nlohmann::json message{
        {"id",ID},
        {"request","PublishJotting"},
        {"jotting",jottingData}
    };

    std::cout<<"开始发送笔记信息: "<<std::endl;
    std::string mes = message.dump(4);
    Client::getInstance()->reconnect();
    Client::getInstance()->send(mes.data(),mes.size());

    std::cout<<"发布笔记完成！"<<std::endl;
}

QString FileOpenDialog::filename() const
{
    return filename_;
}

void FileOpenDialog::setFilename(QString filename)
{
    if (filename_ != filename)
    {
        filename_ = filename;
        emit filenameChanged();
    }
}

QString FileOpenDialog::title() const
{
    return title_;
}

void FileOpenDialog::setTitle(QString title)
{
    if (title_ != title)
    {
        title_ = title;
        emit titleChanged();
    }
}

QStringList FileOpenDialog::nameFilters() const
{
    return nameFilters_;
}

void FileOpenDialog::setNameFilters(QStringList nameFilters)
{
    if (nameFilters_ != nameFilters)
    {
        nameFilters_ = nameFilters;
        emit nameFiltersChanged();
    }
}

bool FileOpenDialog::selectMultiple() const
{
    return selectMultiple_;
}

void FileOpenDialog::setSelectMultiple(bool selectMultiple)
{
    if (selectMultiple_ != selectMultiple)
    {
        selectMultiple_ = selectMultiple;
        emit selectMultipleChanged();
    }
}

QPlatformFileDialogHelper* FileOpenDialog::init_helper()
{
    return static_cast<QPlatformFileDialogHelper*>(
                QGuiApplicationPrivate::platformTheme()->createPlatformDialogHelper(QPlatformTheme::FileDialog)
                );
}

void FileOpenDialog::open()
{
    if (!valid()) return;

    QQuickItem *parent = this->parentItem();
    Q_ASSERT(parent);

    QQuickWindow *window = parent->window();
    Q_ASSERT(window);

    m_parentWindow = window;

    m_options->setFileMode(selectMultiple_ ? QFileDialogOptions::ExistingFiles : QFileDialogOptions::ExistingFile);
    m_options->setAcceptMode(QFileDialogOptions::AcceptOpen);
    m_options->setWindowTitle(title());
    m_options->setNameFilters(nameFilters());

    /*
     * Mac:
     * Set filename incl. directory via setInitiallySelectedFiles()
     *
     * Windows:
     * Set filename via setInitiallySelectedFiles() and let Windows choose the directory.
     * Default directory: C:\\Users\XYZ\Downloads
     *
     * Gnome:
     * Set directory via QPlatformFileDialogHelper::setDirectory() and leave
     * filename empty, since QGtk2FileDialogHelper can not set non-existing filenames.
     *
     */
#ifdef Q_OS_OSX
    QString initialSelection = QFileInfo(QDir::homePath(), filename()).absoluteFilePath();
    qDebug() << "Initial file:" << initialSelection;
    m_options->setInitiallySelectedFiles(QList<QUrl>() << QUrl::fromLocalFile(initialSelection));
#endif
#ifdef Q_OS_WIN
    qDebug() << "Initial filename:" << filename();
    m_options->setInitiallySelectedFiles(QList<QUrl>() << QUrl::fromLocalFile(filename()));
#endif
#ifdef Q_OS_LINUX
    qDebug() << "Initial directory:" << QDir::homePath();
    m_dlgHelper->setDirectory(QUrl::fromLocalFile(QDir::homePath()));
#endif

    m_dlgHelper->setOptions(m_options);
    m_dlgHelper->setFilter(); // applyOptions();

    Qt::WindowFlags flags = Qt::Dialog;
    if (!title().isEmpty()) flags |= Qt::WindowTitleHint;

    m_visible = m_dlgHelper->show(flags, m_modality, m_parentWindow);
}

void FileOpenDialog::close()
{
    if (!valid()) return;

    m_dlgHelper->hide();
    m_visible = false;
}

void FileOpenDialog::accept()
{
    if (!valid()) return;

    m_dlgHelper->hide();

    QList<QUrl> selectedUrls = m_dlgHelper->selectedFiles();
    if (!selectedUrls.empty())
    {
        if (selectedUrls.size() == 1)
            setFileUrl(selectedUrls.at(0));
        else
            setFileUrl();

        setFileUrls(selectedUrls);
    }

    emit accepted();
}

void FileOpenDialog::reject()
{
    if (!valid()) return;

    m_dlgHelper->hide();
    emit rejected();
}
