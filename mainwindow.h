////////////////////////////////////////////////////////////////////////////////
//3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
//
// FastPixels - a experiment into hand-tuned x86-assembly
//
// Copyright 2016-2017 Mirco Müller
//
// Author(s):
//   Mirco "MacSlow" Müller <macslow@gmail.com>
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License version 3, as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranties of
// MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
// PURPOSE.  See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>

#include <QMainWindow>
#include <QFileDialog>
#include <QSlider>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QPixmap>
#include <QImage>
#include <QLabel>

enum class ComputeType {
    CPPDIM1PASS,
    CPPDIM2PASS,
    ASM,
    SSSE3,
    SSSE3MT,
    CPPBLUR1PASS,
    CPPBLUR2PASS,
    AVXBLUR2PASS
};

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    public:
        explicit MainWindow (QWidget *parent = 0);
        ~MainWindow ();

    public slots:
        void keyPressEvent (QKeyEvent* event);
        void wheelEvent (QWheelEvent* event);
        void mousePressEvent (QMouseEvent* event);
        void mouseReleaseEvent (QMouseEvent* event);
        void mouseMoveEvent (QMouseEvent* event);
        void changeValue (int value);
        void changeComputeType  (int index);
        void on_pushButton_clicked ();
        void imageSelected (const QString&);

    private slots:
        void update ();

    private:
        std::unique_ptr<QFileDialog> fileDialog_;
        QImage orig_;
        QImage front_;
        QPixmap pixmap_;
        std::shared_ptr<QLabel> imageDisplay_;
        std::unique_ptr<Ui::MainWindow> ui_;
        float scale_;
        QPoint offset_;
        ComputeType computeType_;
        QPoint last_;
        bool lmb_;
};

#endif // MAINWINDOW_H
