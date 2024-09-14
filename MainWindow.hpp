#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>

#include <QMainWindow>

#include "ClangClientDialog.hpp"
#include "ClangdClient.hpp"
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
    // Store the QObject based in unique_ptr because they are not moveable or copyable
    std::unique_ptr<ClangdClient> clangdClient;
    std::unique_ptr<ClangClientDialog> clientDialog;
    std::unique_ptr<ProjectModel> projectModel;
    std::unique_ptr<ExtensionFilterProxyModel> projectModelFilter;
    std::unique_ptr<Ui::MainWindow> ui; // Must be last to make sure that all the objects are deleted before the UI

    void closeTab(int index);
private slots:
    void showOpenProject(bool trigger = false);
    void showClangDebugDialog(bool triggered = false);
    void onProjectFileDoubleClick(const QModelIndex &index);
    void tabCloseRequested(int index);
};
#endif // MAINWINDOW_H
