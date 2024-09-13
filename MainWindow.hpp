#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>

#include <QMainWindow>

#include "ClangClientDialog.hpp"
#include "ProjectModel.hpp"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    std::unique_ptr<Ui::MainWindow> ui;
    std::unique_ptr<ClangClientDialog> clientDialog;
    std::unique_ptr<ProjectModel> projectModel;
    std::unique_ptr<ExtensionFilterProxyModel> projectModelFilter;
private slots:
    void showOpenProject(bool trigger = false);
    void showClangDebugDialog(bool triggered = false);
};
#endif // MAINWINDOW_H
