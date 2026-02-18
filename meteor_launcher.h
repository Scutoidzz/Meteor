/**
 * meteor_launcher.h — Main launcher header
 */

#ifndef METEOR_LAUNCHER_H
#define METEOR_LAUNCHER_H

#include <QMainWindow>
#include <QWidget>

class MeteorMainWindow : public QMainWindow {
    Q_OBJECT

public:
    MeteorMainWindow(QWidget* parent = nullptr);

private:
    void initUI();
    QWidget* createIntroStub();
    QWidget* createAccountsStub();

private slots:
    void onLaunchIntro();
    void onLaunchAccounts();
};

class ServerLauncher : public QWidget {
    Q_OBJECT

public:
    ServerLauncher(const QString& projectRoot, QWidget* parent = nullptr);

private:
    void initUI();
    QString m_projectRoot;

private slots:
    void onStartServer();
    void onStopServer();
};

#endif // METEOR_LAUNCHER_H
