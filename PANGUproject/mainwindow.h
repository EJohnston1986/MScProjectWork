#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "socket_stuff.h"
#include "pan_protocol_lib.h"
#include <QVector3D>
#include <QGraphicsScene>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void setSock(SOCKET sock);  // function to set the socket
    SOCKET getSock();           // function to get the socket
    std::vector<std::vector<float>> coordinates; // vector of vectors to store the 6 coordinate positions (x,y,z,a,b,c)

private slots:
    void on_uploadFileButton_clicked();
    void on_saveAsFileButton_clicked();
    void on_pbReplot_clicked();
    void on_nextCamPosButton_clicked();
    void on_prevCamPosButton_clicked();
    void on_SaveFileButton_clicked();
    void on_saveImageButton_clicked();

private:
    Ui::MainWindow *ui;                  // ui pointer
    SOCKET sock;                         // the socket
    void closeEvent(QCloseEvent *event); // overriding the close event to show a dialog
    void drawTrajectory(int x, int y);   // function to draw the plot in model view
    void fileUpdated(QString message);   // function called when a file is updated (upload, save)
    void uploadFile();                   // function called to load/reload file from disk to editor

    // the following store the max and min values of the 6 dimensions, used to decide width/height of canvas
    float max_x = -1e9;
    float min_x = 1e9;
    float max_y = -1e9;
    float min_y = 1e9;
    float max_z = -1e9;
    float min_z = 1e9;
    float max_yw = -1e9;
    float min_yw = 1e9;
    float max_pi = -1e9;
    float min_pi = 1e9;
    float max_rl = -1e9;
    float min_rl = 1e9;

    QGraphicsScene * scene = new QGraphicsScene();  // graphics scene object used to draw the model view on
    float scale_height = 1;                         // scale factor, to scale from dimension height to canvas height
    float scale_width = 1;                          // scale factor to scale from dimension width to canvas width
    unsigned int frame = 0;                         // frame index, used for stepping through frames
    unsigned long * imageLength = new unsigned long(); // data length of image
    unsigned char *img;                                // the image data to be rendered
    QString fileName;                               // full file name including path
    QString displayFileName;                        // display filename with path removed
    QPixmap *pixmap = new QPixmap(300, 300);        // QPixmap object used to render image to screen in spacecraft view
    QGraphicsScene* SVscene = new QGraphicsScene(); // QGraphicsscene object to draw the spacecraft view on
    void stepFrame();                               // function to step through and render each frame
};
#endif // MAINWINDOW_H
