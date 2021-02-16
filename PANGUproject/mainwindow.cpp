#include "mainwindow.h"
#include "ui_mainwindow.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include <QMenuBar>
#include <QStatusBar>
#include <QGridLayout>
#include <QTextEdit>
#include <QMessageBox>
#include <QCloseEvent>
#include <QFileDialog>
#include <QDir>
#include <QTextStream>
#include <QFile>
#include <QStringList>
#include <QPieSeries>
#include <QChartView>
#include <QChart>
#include <QPointF>
#include <QGraphicsScene>
#include <QVector>
#include <QDebug>
#include <QGraphicsPixmapItem>
#include <QPixmap>
#include <sstream>


static int get_and_save_image(SOCKET sock, char *fname) {
    unsigned long todo;
    unsigned char *ptr, *img;
    FILE *handle;

    /* Retrieve an image */
    (void)fprintf(stderr, "Getting image '%s'\n", fname);
    img = pan_protocol_get_image(sock, &todo);

    /* Open the output file for writing */
    handle = fopen(fname, "wb");
    if (!handle)
    {
        const char *fmt = "Error: failed to open '%s' for writing\n";
        (void)fprintf(stderr, fmt, fname);
        return 4;
    }
    printf("okay so far");
    /* Write the image data to the file */
    ptr = img;
    while (todo > 1024) {
        long wrote;
        wrote = fwrite(ptr, 1, 1024, handle);
        if (wrote < 1) {
            const char *fmt = "Error writing to '%s'\n";
            (void)fprintf(stderr, fmt, fname);
            (void)fclose(handle);
            return 5;
        } else {
            todo -= wrote;
            ptr += wrote;
        }
    }
    if (todo) {
        printf("Writing\n");
        long wrote;
        wrote = fwrite(ptr, 1, todo, handle);
        if (wrote < 1) {
            const char *fmt = "Error writing to '%s'\n";
            (void)fprintf(stderr, fmt, fname);
            (void)fclose(handle);
            return 5;
        }
    }

    /* Close the file */
    (void)fclose(handle);

    /* Release the image data */
    (void)free(img);

    /* Return success */
    printf("Returning success");
    return 0;
}

/* The get image function is used to get the image from the camera view and display it in
 the spacecraft view of the GUI */
static unsigned char* get_image(SOCKET sock, unsigned long& imageLength) {
    //unsigned long todo;
    unsigned char *img;

    /* Retrieve an image */
    img = pan_protocol_get_image(sock, &imageLength);

    return img;
}

// constructor for the main window
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), fileName(""), displayFileName("") {
    ui->setupUi(this);

    //setting the window title
    setWindowTitle("PANGU - Spacecraft flightpath planner GUI v1.0");

    //adding status bar
    statusBar()->showMessage("");

    //setting up a grid layout for three sections
    QGridLayout *layout = new QGridLayout(this);
    // set into rows
    layout->addWidget(ui->fliEditorgroupBox,0,0);
    layout->addWidget(ui->modelViewgroupBox,0,1);
    layout->addWidget(ui->spacecraftViewgroupBox,0,2);

    // settting the scroll bars to off in the main window
    ui->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->spacecraftView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->spacecraftView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // adding tool tips
    ui->uploadFileButton->setToolTip("Choose a .fli file to upload");
    ui->saveAsFileButton->setToolTip("Save your .fli file as a new file here");
    ui->SaveFileButton->setToolTip("Save your fli file here");
    ui->saveImageButton->setToolTip("If you are happy, press save to save the current image.");
    ui->pbReplot->setToolTip("Replot the coordinates");
    ui->nextCamPosButton->setToolTip("Change the camera to the next position");
    ui->prevCamPosButton->setToolTip("Change the camera to the previous position");
}

// setting the socket connection,adding the socket into the object
void MainWindow::setSock(SOCKET sock) {
    this->sock = sock;
}

// getting the socket connection, getting the socket from the object
SOCKET MainWindow::getSock() {
    return sock;
}

// the destructor
MainWindow::~MainWindow() {
    pan_protocol_finish(this->getSock());  // calling the function on mainwindow destruction
    SOCKET_CLOSE(this->getSock());         // calling the socket close function on mainwindow destruction

#ifdef _WIN32
    WSACleanup();
#endif
    delete ui;
}

// function to override the close event when a user clicks on the cross to close the application.
void MainWindow::closeEvent (QCloseEvent *event) {
    // function opens a dialog window asking user if they are they want to quit the application
    QMessageBox::StandardButton resBtn = QMessageBox::question(this,"PANGU - Spacecraft flightpath planner", "Make sure you have saved all your files.\nAre you sure you want to quit?\n",
                                                               QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
                                                               QMessageBox::Yes);

    if (resBtn != QMessageBox::Yes) {
        event->ignore();
    } else {
        event->accept();
    }

}

// Function to handle the reading of a file and filling of the coordinates vector with values from the file
void MainWindow::uploadFile() {
    QString fileContent;  // used to store a line of string data from the text file
    QString startContent;

    if(fileName.isEmpty()) // checking if the filename variable is empty, dont want to work on an empty file
        return;

    QFile file(fileName);  // passing in the filename to a QFile object called file

    //Opening the file.
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;           // if file doesn't open, exit.

    // Clear the existing vector (for case, file previously opened)
    this->coordinates.clear();

    QTextStream in(&file);          // in, used for streaming the information into the file object
    QString line = in.readLine();   // reads one line of text from the stream and returns a string

    // filtering out the coordinate data from the flight file
    while(!line.isNull()) {               // while there is a string stored in line

        fileContent.append(line + "\n");  // adds the string to variable fileContent and then starts a new line
        line = in.readLine();             // line stores the texts taken in from the textstream object

        // command to begin camera control beings with the word "start", which we want to filter out
        if(line.startsWith("start")) {
           QString tempLine = line.simplified().replace(" ",",");  // simplified removes whitespace
           QStringList tempList = tempLine.split(",");   // stores the numbers in a list seperated by a comma

           // assigning the x y and z coordinates for each camera position, turns the string values into numbers
           float x = tempList[1].toFloat();
           float y = tempList[2].toFloat();
           float z = tempList[3].toFloat();
           float yw = tempList[4].toFloat();
           float pi = tempList[5].toFloat();
           float rl = tempList[6].toFloat();

           //reassigning the max and min coordinate values
           if (x > this->max_x)
               this->max_x = x;
           if (x < this->min_x)
               this->min_x = x;
           if (y >  this->max_y)
                this->max_y = y;
           if (y <  this->min_y)
                this->min_y = y;
           if (z >  this->max_z)
                this->max_z = z;
           if (z <  this->min_z)
                this->min_z = z;
           if (yw >  this->max_yw)
                this->max_yw = yw;
           if (yw <  this->min_yw)
                this->min_yw = yw;
           if (pi >  this->max_pi)
                this->max_pi = pi;
           if (pi <  this->min_pi)
                this->min_pi = pi;
           if (rl >  this->max_rl)
                this->max_rl = rl;
           if (rl <  this->min_rl)
                this->min_rl = rl;

           // storing the coordinate information in a temporary vector then adding to the coordinate vector
           std::vector<float> tempVector;
           tempVector.push_back(x);
           tempVector.push_back(y);
           tempVector.push_back(z);
           tempVector.push_back(yw);
           tempVector.push_back(pi);
           tempVector.push_back(rl);
           coordinates.push_back(tempVector);           
       } else {
            qDebug() << "Line didnt start with start: " << line;
       }
    }

    file.close();

    ui->textEdit->clear();                   // making sure the text edit is clear before loading the file data
    ui->textEdit->setPlainText(fileContent); // uploading the contents of the file into the text editor

    this->drawTrajectory(0, 1);  // call function to draw model view (defaults X vs Y)
    this->stepFrame();           // call step frame to start rendering the spacecraft view

}

// Function for when the user chooses to upload a flight file by pressing the upload button
void MainWindow::on_uploadFileButton_clicked() {

   // Opens up a Dialog window and allows a user to select a fli file to open
   fileName = QFileDialog::getOpenFileName(this, "Open file", QDir::homePath(), "FLI file (*.fli)");
   this->uploadFile();  // call the upload file function

   this->fileUpdated("Loaded .fli file");
}

// function for case where the file in the editor has been updated
void MainWindow::fileUpdated(QString message) {

    // Need to filter out the file name
    displayFileName = fileName;
    for (int i = 0; i < displayFileName.size(); i++) {
        if (displayFileName[i] == "/")
            displayFileName = displayFileName.remove(0,(i+1));
    }

    // displaying the file name
    ui->fileNamelabel->setText(displayFileName);
    if (fileName != "") {
        ui->SaveFileButton->setEnabled(true);
    }
    // updating status bar information when a file has been selected
    statusBar()->showMessage(message, 2000);
}

// function ito draw plot in model view, takes numerical parameters representing the axis to render, X = 0, Y = 1, Z = 2
void MainWindow::drawTrajectory(int x_axis, int y_axis) {

    // setting the width and height of the scene based on the min and max coordinate values
    float width = 0;
    float local_min_x = 0;
    float local_min_y = 0;


    if (x_axis == 0) {
        width = this->max_x - this->min_x;
        local_min_x = this->min_x;
    } else if (x_axis == 1) {
        width = this->max_y - this->min_y;
        local_min_x = this->min_y;
    } else if (x_axis == 2) {
        width = this->max_z - this->min_z;
        local_min_x = this->min_z;
    } else if (x_axis == 3) {
        width = this->max_yw - this->min_yw;
        local_min_x = this->min_yw;
    } else if (x_axis == 4) {
        width = this->max_pi - this->min_pi;
        local_min_x = this->min_pi;
    } else if (x_axis == 5) {
        width = this->max_rl - this->min_rl;
        local_min_x = this->min_rl;
    }

    float height = 0;
    if (y_axis == 0) {
        height = this->max_x - this->min_x;
        local_min_y = this->min_x;
    } else if (y_axis == 1) {
        height = this->max_y - this->min_y;
        local_min_y = this->min_y;
    } else if (y_axis == 2) {
        height = this->max_z - this->min_z;
        local_min_y = this->min_z;
    } else if (y_axis == 3) {
        height = this->max_yw - this->min_yw;
        local_min_y = this->min_yw;
    } else if (y_axis == 4) {
        height = this->max_pi - this->min_pi;
        local_min_y = this->min_pi;
    } else if (y_axis == 5) {
        height = this->max_rl - this->min_rl;
        local_min_y = this->min_rl;
    }

    if (height > width)
        width = height;
    else if (width > height)
        height = width;

    // adding a 10% margin on the height
    height = height * 1.1;
    width = width * 1.1;

    QRectF exactRect(local_min_x, local_min_y, height, height);
    QPen pen = QPen(Qt::black);
    pen.setWidth(100);

    // creating the QGraphics scene where the data is stored    
    this->scene->clear();
    ui->graphicsView->items().clear();
    ui->graphicsView->setScene(this->scene);
    ui->graphicsView->setSceneRect(exactRect);
    ui->graphicsView->scale(1/this->scale_width, 1/this->scale_height);
    this->scale_width = ui->graphicsView->width()/exactRect.width();
    this->scale_height = -1*ui->graphicsView->height()/exactRect.height();

    ui->graphicsView->scale(this->scale_width, this->scale_height);

    // looping over coordinates to draw line (this is done first so the points which are rendered later always sit on top of the line)
    for(unsigned int i = 0; i < this->coordinates.size(); i++) {
        if (i > 0) {
           scene->addLine(this->coordinates[i-1][x_axis], this->coordinates[i-1][y_axis], this->coordinates[i][x_axis], this->coordinates[i][y_axis], pen);
        }
 }
    // Looping over the coordinates to draw the points (this is done secont so the points are drawn on top of the line rendered above)
    for(unsigned int i = 0; i < this->coordinates.size(); i++) {
        pen = QPen(Qt::green);
        QBrush brush(Qt::green);
        pen.setWidth(100);
        if (i == this->frame) {
            pen.setColor(Qt::yellow);
            brush.setColor(Qt::yellow);
        }
        else if (i == 0) {
            pen.setColor(Qt::blue);
            brush.setColor(Qt::blue);
        }
        else if (i == this->coordinates.size() - 1) {
            pen.setColor(Qt::red);
            brush.setColor(Qt::red);
        }

        scene->addEllipse(this->coordinates[i][x_axis], this->coordinates[i][y_axis], 1000, 1000, pen, brush);
 }
    ui->graphicsView->update();
    ui->graphicsView->show();

}
void MainWindow::on_saveAsFileButton_clicked() {
    // save file
    fileName = QFileDialog::getSaveFileName(this, "Save as",QDir::homePath(),"FLI file (*.fli)");   
    if (fileName.isEmpty())
        return;

    QFile file(fileName);

    // open the file
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))
        return;

    // text stream
    QTextStream out(&file);
    out << ui->textEdit->toPlainText() << "\n";

    // close file
    file.close();

    this->fileUpdated("Saved .fli file");
    this->uploadFile();

}

// Function to step through and render each of the frames
void MainWindow::stepFrame() {

        // case there is no data to work with, exit function
        if (this->coordinates.size() < 1)
            return;
        // stringstreams are a nice and safe way of converting numerical values to strings
        std::stringstream ss;
        ss << "Displaying frame " << this->frame;
        std::string message = ss.str();
        statusBar()->showMessage(message.c_str());

        // extracting cam position info from coordinates vector
        float x = this->coordinates[this->frame][0];
        float y = this->coordinates[this->frame][1];
        float z = this->coordinates[this->frame][2];
        float yw = this->coordinates[this->frame][3];
        float pi = this->coordinates[this->frame][4];
        float rl = this->coordinates[this->frame][5];
        qDebug() << "x: " << x << ", y: " << y << ", z: " << z << ", yw: " << yw << ", pi: " << pi << ", rl: " << rl;

        // making use of the pan_protocol helper function to set position of the camera
        pan_protocol_set_viewpoint_by_angle_s(this->getSock(), x, y, z, yw, pi, rl);

        // retrieving image data from PANGU
        img = pan_protocol_get_viewpoint_by_camera(this->getSock(), 0, imageLength);

        // display data on QPixmap object
        pixmap->loadFromData(img, *imageLength);

        QGraphicsPixmapItem* item = new QGraphicsPixmapItem(*pixmap);
        SVscene->clear();
        SVscene->addItem(item);
        ui->spacecraftView->setScene(SVscene);
        ui->spacecraftView->show();
        ui->spacecraftView->viewport()->repaint();

        if (ui->comboBox_X->currentIndex() != 0 && ui->comboBox_Y->currentIndex() != 0) {
           ui->ZCoordNumLabel->setNum(x);
        } else if (ui->comboBox_X->currentIndex() != 1 && ui->comboBox_Y->currentIndex() != 1) {
           ui->ZCoordNumLabel->setNum(y);
        } else if (ui->comboBox_X->currentIndex() != 2 && ui->comboBox_Y->currentIndex() != 2) {
            ui->ZCoordNumLabel->setNum(z);
        }
    // draw trajectory on model view with X and Y values chosen by users from combo boxes
    this->drawTrajectory(ui->comboBox_X->currentIndex(), ui->comboBox_Y->currentIndex());
}


void MainWindow::on_pbReplot_clicked() {
    // event, file has not been loaded into the GUI
    if(fileName.isEmpty()) {
        statusBar()->showMessage("File not uploaded, cannot complete replot", 3000);
        return;
    }

    //Get x and y axis info from users choice on GUI combo boxes
    int x_axis = ui->comboBox_X->currentIndex();
    int y_axis = ui->comboBox_Y->currentIndex();

    // call drawTrajectory with chosen axes to trigger replot of model view with these axes
    this->drawTrajectory(x_axis, y_axis);
}

// function to increment the frame and trigger all the re-renderings
void MainWindow::on_nextCamPosButton_clicked() {    

    // event, file has not been loaded into the GUI.
    if(fileName.isEmpty()) {
        statusBar()->showMessage("File not uploaded, cannot change camera position", 3000);
        return;
    }

    if(this->frame < this->coordinates.size()-1) {
        this->frame = this->frame+1;
    }
    this->stepFrame();
}

// function to decrement the frame and trigger all the re-renderings
void MainWindow::on_prevCamPosButton_clicked() {

    // event file has not been loaded into the GUI
    if(fileName.isEmpty()) {
        statusBar()->showMessage("File not uploaded, cannot change camera position");
        return;
    }

    if(this->frame > 0) {
        this->frame = this->frame-1;
    }
    this->stepFrame();
}


// saving a file that has been opened or created in the editor
void MainWindow::on_SaveFileButton_clicked() {

    if (fileName.isEmpty())
        return;

    QFile file(fileName);

    if(!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    this->fileUpdated("Saved .fli file");
    // text stream
    QTextStream out(&file);
    out << ui->textEdit->toPlainText() << "\n";

    // close file
    file.close();
    this->fileUpdated("Saved .fli file");

    // trigger re-read of file data into QTextEdit and repopulate the coordinates vector etc
    this->uploadFile();

}

// function to save an image displayed in the GUI
void MainWindow::on_saveImageButton_clicked() {

    int a = 10;

    // use stringstream to build a filename with the frame index
    std::stringstream ss;
    ss << QDir::homePath().toLocal8Bit().constData();
    ss << "/SpaceCraftFlightPathPlanner_";
    ss << this->frame;
    ss << ".png";
    std::string str = ss.str();

    char * name = &str[0];

    // use helper function to actually get image and do save
    get_and_save_image(this->getSock(), name );
    statusBar()->showMessage(("Saved image " + str).c_str(), 3000);

}
