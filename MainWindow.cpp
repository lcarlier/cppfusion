#include <QDebug>
#include <QObject>
#include <QMessageBox>

#include "MainWindow.hpp"
#include "./ui_MainWindow.h"
#include "OpenProject.hpp"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), clientDialog{nullptr}, ui(new Ui::MainWindow) {
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
        {
            projectModel.reset(new ProjectModel{clangdProject, ui->treeViewProject});
            projectModelFilter.reset(new ExtensionFilterProxyModel{clangdProject});
            projectModelFilter->setSourceModel(projectModel.get());
            ui->treeViewProject->setModel(projectModelFilter.get());
            ui->treeViewProject->setRootIndex(projectModelFilter->mapFromSource(projectModel->index(clangdProject.projectRoot)));
            ui->treeViewProject->hideColumn(1);
            ui->treeViewProject->hideColumn(2);
            ui->treeViewProject->hideColumn(3);
        }
        clangdClient.reset(new ClangdClient{clangdProject, this});
        clientDialog.reset(new ClangClientDialog{*clangdClient, clangdProject, this});
        clientDialog->setWindowFlags(clientDialog->windowFlags() | Qt::WindowMaximizeButtonHint | Qt::Window);
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
