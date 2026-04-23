#pragma once

#include <QFileDialog>
#include <QMessageBox>
#include <QString>

class QWidget;

class ProfileSetupDialogs
{
public:
    virtual ~ProfileSetupDialogs() = default;

    virtual QString getSaveFilePath(QWidget* parent, const QString& defaultDir) = 0;
    virtual QString getOpenFilePath(QWidget* parent, const QString& defaultDir) = 0;
    virtual void showError(QWidget* parent, const QString& title, const QString& message) = 0;
    virtual void showInfo(QWidget* parent, const QString& title, const QString& message) = 0;
    virtual void showWarning(QWidget* parent, const QString& title, const QString& message) = 0;
};

class DefaultProfileSetupDialogs : public ProfileSetupDialogs
{
public:
    QString getSaveFilePath(QWidget* parent, const QString& defaultDir) override
    {
        return QFileDialog::getSaveFileName(
            parent,
            "Save Profile",
            defaultDir + "/profile.yaml",
            "YAML Files (*.yaml *.yml);;All Files (*)");
    }

    QString getOpenFilePath(QWidget* parent, const QString& defaultDir) override
    {
        return QFileDialog::getOpenFileName(
            parent,
            "Load Profile",
            defaultDir,
            "YAML Files (*.yaml *.yml);;All Files (*)");
    }

    void showError(QWidget* parent, const QString& title, const QString& message) override
    {
        QMessageBox::critical(parent, title, message);
    }

    void showInfo(QWidget* parent, const QString& title, const QString& message) override
    {
        QMessageBox::information(parent, title, message);
    }

    void showWarning(QWidget* parent, const QString& title, const QString& message) override
    {
        QMessageBox::warning(parent, title, message);
    }
};
