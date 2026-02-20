#ifndef METEOR_INTRO_H
#define METEOR_INTRO_H

#include <QWidget>
#include <QThread>
#include <QPixmap>
#include <QLabel>
#include <QGridLayout>
#include <QLineEdit>
#include <QPushButton>

class ImageLoader : public QThread {
    Q_OBJECT
public:
    ImageLoader(const QString& url, int index, QObject* parent = nullptr);
    void run() override;

signals:
    void imageLoaded(const QPixmap& pixmap, int index);

private:
    QString m_url;
    int m_index;
};

class CoverFetcher : public QThread {
    Q_OBJECT
public:
    CoverFetcher(const QString& serverUrl, QObject* parent = nullptr);
    void run() override;

signals:
    void coversFound(const QStringList& urls);
    void errorOccurred(const QString& errorMsg);

private:
    QString m_serverUrl;
};

class IntroScreen : public QWidget {
    Q_OBJECT
public:
    IntroScreen(QWidget* parent = nullptr);
    ~IntroScreen();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void fetchCovers();
    void handleFetchError(const QString& errorMsg);
    void handleCoversFound(const QStringList& urls);

private:
    void initUi();
    void loadConfig();

    QWidget* m_inputWidget;
    QLineEdit* m_serverInput;
    QPushButton* m_connectBtn;
    
    QWidget* m_collageContainer;
    QGridLayout* m_collageLayout;
    
    QList<ImageLoader*> m_loaders;
    CoverFetcher* m_fetcher;
};

#endif // METEOR_INTRO_H
