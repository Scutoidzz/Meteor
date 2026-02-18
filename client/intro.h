/**
 * client/intro.h — Meteor intro screen header
 * ------------------------------------------
 * Header file for the intro screen widget and background workers.
 */

#ifndef METEOR_INTRO_H
#define METEOR_INTRO_H

#include <QWidget>
#include <QLineEdit>
#include <QGridLayout>
#include <QObject>
#include <QThread>
#include <QString>
#include <QPixmap>
#include <QStringList>

#include <memory>
#include <vector>

// Forward declarations
class QLabel;
class QScrollArea;
class QPushButton;
class QNetworkAccessManager;

/**
 * Background worker thread for fetching covers from the server.
 */
class CoverFetcher : public QObject {
    Q_OBJECT

public:
    explicit CoverFetcher(const QString& serverUrl, QObject* parent = nullptr);
    void fetch();

signals:
    void coversFound(const QStringList& urls);
    void errorOccurred(const QString& message);
    void finished();

private slots:
    void run();

private:
    QString m_serverUrl;
    QNetworkAccessManager* m_manager;
};

/**
 * Background worker thread for loading individual cover images.
 */
class ImageLoader : public QObject {
    Q_OBJECT

public:
    explicit ImageLoader(const QString& url, int index, QObject* parent = nullptr);
    void load();

signals:
    void imageLoaded(const QPixmap& pixmap, int index);
    void finished();

private slots:
    void run();

private:
    QString m_url;
    int m_index;
    QNetworkAccessManager* m_manager;
};

/**
 * Main intro screen widget.
 */
class IntroScreen : public QWidget {
    Q_OBJECT

public:
    IntroScreen(QWidget* parent = nullptr);
    ~IntroScreen();

private slots:
    void fetchCovers();
    void handleFetchError(const QString& errorMsg);
    void handleCoversFound(const QStringList& urls);
    void updateImage(const QPixmap& pixmap, int index);

private:
    void initUI();
    void loadConfig();

    QWidget* m_inputWidget;
    QLineEdit* m_serverInput;
    QGridLayout* m_collageLayout;
    std::vector<std::unique_ptr<ImageLoader>> m_loaders;
};

#endif // METEOR_INTRO_H
