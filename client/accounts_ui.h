/**
 * client/accounts_ui.h — Accounts UI screens (header)
 * ==================================================
 * Login, sign-up, and account management screens.
 */

#ifndef METEOR_ACCOUNTS_UI_H
#define METEOR_ACCOUNTS_UI_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>
#include <QString>

#include <memory>

// Forward declarations
class QVBoxLayout;
class QHBoxLayout;
class QCheckBox;

/**
 * Login screen widget.
 */
class LoginScreen : public QWidget {
    Q_OBJECT

public:
    LoginScreen(QWidget* parent = nullptr);

signals:
    void loginAttempted(const QString& email, const QString& password);
    void switchToSignUp();
    void switchToResetPassword();

private slots:
    void onLoginClicked();

private:
    void initUI();

    QLineEdit* m_emailInput;
    QLineEdit* m_passwordInput;
    QPushButton* m_loginBtn;
    QPushButton* m_signUpBtn;
    QPushButton* m_forgotBtn;
    QCheckBox* m_rememberMe;
};

/**
 * Sign-up screen widget.
 */
class SignUpScreen : public QWidget {
    Q_OBJECT

public:
    SignUpScreen(QWidget* parent = nullptr);

signals:
    void signUpAttempted(const QString& email, const QString& password, const QString& confirmPassword);
    void switchToLogin();

private slots:
    void onSignUpClicked();

private:
    void initUI();
    bool validateForm();

    QLineEdit* m_emailInput;
    QLineEdit* m_passwordInput;
    QLineEdit* m_confirmPasswordInput;
    QLineEdit* m_usernameInput;
    QPushButton* m_signUpBtn;
    QPushButton* m_backBtn;
    QLabel* m_errorLabel;
};

/**
 * Password reset screen widget.
 */
class ResetPasswordScreen : public QWidget {
    Q_OBJECT

public:
    ResetPasswordScreen(QWidget* parent = nullptr);
    void setStatus(const QString& message, const QString& color = "#FF3B30");

signals:
    void resetAttempted(const QString& email);
    void switchToLogin();

private slots:
    void onResetClicked();

private:
    void initUI();

    QLineEdit* m_emailInput;
    QPushButton* m_resetBtn;
    QPushButton* m_backBtn;
    QLabel* m_statusLabel;
};

/**
 * Account dashboard/profile screen widget.
 */
class AccountDashboard : public QWidget {
    Q_OBJECT

public:
    AccountDashboard(const QString& email, QWidget* parent = nullptr);

signals:
    void logout();

private slots:
    void onLogoutClicked();
    void onChangePasswordClicked();
    void onUpdateProfileClicked();

private:
    void initUI();
    void loadUserInfo();

    QString m_email;
    QLabel* m_usernameLabel;
    QLabel* m_emailLabel;
    QLabel* m_joinDateLabel;
    QPushButton* m_changePasswordBtn;
    QPushButton* m_updateProfileBtn;
    QPushButton* m_logoutBtn;
};

/**
 * Main accounts manager that switches between different screens.
 */
class AccountsManager : public QWidget {
    Q_OBJECT

public:
    explicit AccountsManager(QWidget* parent = nullptr);

private slots:
    void showLoginScreen();
    void showSignUpScreen();
    void showResetPasswordScreen();
    void showDashboard(const QString& email);
    void handleLogout();
    void handleLogin(const QString& email, const QString& password);
    void handleSignUp(const QString& email, const QString& password, const QString& confirmPassword);
    void handleReset(const QString& email);

private:
    void initUI();

    QStackedWidget* m_stackedWidget;
    std::unique_ptr<LoginScreen> m_loginScreen;
    std::unique_ptr<SignUpScreen> m_signUpScreen;
    std::unique_ptr<ResetPasswordScreen> m_resetScreen;
    std::unique_ptr<AccountDashboard> m_dashboard;

    // Screen indices
    static constexpr int LOGIN_SCREEN = 0;
    static constexpr int SIGNUP_SCREEN = 1;
    static constexpr int RESET_SCREEN = 2;
    static constexpr int DASHBOARD_SCREEN = 3;
};

#endif // METEOR_ACCOUNTS_UI_H
