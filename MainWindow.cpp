#include <QDebug>
#include <QObject>
#include <QMessageBox>

#include "MainWindow.hpp"
#include "./ui_MainWindow.h"
#include "OpenProject.hpp"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), clientDialog{nullptr} {
    ui->setupUi(this);

    connect(ui->actionShow_Clang_Debug, &QAction::triggered, this,
            &MainWindow::showClangDebugDialog);
    connect(ui->actionOpen_project, &QAction::triggered, this, &MainWindow::showOpenProject);

}


void MainWindow::showOpenProject(bool /*triggered*/)
{
    OpenProject openProject;
    auto result = openProject.exec();
    if(result == OpenProject::Accepted)
    {
        ClangdProject clangdProject = openProject.getClangdProject();
        clientDialog.reset(new ClangClientDialog{std::move(clangdProject), this});
    }
}

void MainWindow::showClangDebugDialog(bool /*triggered*/)
{
    if(!clientDialog)
    {
        QMessageBox::critical(this, "Cannot open the debug dialog", "Cannot open the debug dialog\nNo project open");
        return;
    }
    clientDialog->show();
}

MainWindow::~MainWindow() {}
