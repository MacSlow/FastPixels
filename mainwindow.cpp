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

#include <iostream>
#include <chrono>
#include <string>
#include <iomanip>
#include <sstream>
#include <memory>
#include <future>
#include <cmath>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "asm.h"

using namespace std;

struct Input
{
    const uchar* src;
    uchar* dst;
    int numBytes;
    int value;
};

void changeBrightnessSSSE3threaded (future<Input>& f)
{
    Input input = f.get ();
    changeBrightnessSSSE3 (input.src,
                           input.dst,
                           input.numBytes,
                           input.value);
}

MainWindow::MainWindow (QWidget* parent) :
    QMainWindow (parent),
    fileDialog_ (make_unique<QFileDialog> ()),
    ui_ (new Ui::MainWindow),
    scale_ (1.0),
    offset_ (QPoint (0, 0)),
    computeType_ (ComputeType::CPPDIM1PASS),
    last_ (QPoint (0, 0)),
    lmb_ (false)
{
    ui_->setupUi (this);
    imageSelected (QString ("/home/mirco/Bilder/Husky-mean-lean-1.jpg"));

    connect (ui_->horizontalSlider,
             SIGNAL (valueChanged (int)),
             this,
             SLOT (changeValue (int)));
    connect (ui_->comboBox,
             SIGNAL (currentIndexChanged (int)),
             this,
             SLOT (changeComputeType (int)));
}

MainWindow::~MainWindow ()
{
}

uchar clamp (int value)
{
    uchar result = 0;

    if (value < 0)
        result = 0;
    else if (value > 255)
        result = 255;
    else
        result = (uchar) value;

    return result;
}

void changeBrightness1PassCPP (const uchar* src,
                               uchar* dst,
                               int numBytes,
                               int value)
{
    for (int i = 0; i < numBytes; ++i) {
        dst[i] = clamp (src[i] + value);
    }
}

void changeBrightness2PassCPP (const uchar* src,
                               uchar* dst,
                               int width,
                               int height,
                               int value)
{
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            unsigned index = 4 * (y * width + x);

            dst[index] = clamp (src[index] + value);
            dst[index+1] = clamp (src[index+1] + value);
            dst[index+2] = clamp (src[index+2] + value);
        }
    }
}

void boxBlur1PassCPP (const uchar* src,
                   uchar* dst,
                   unsigned width,
                   unsigned height,
                   int value)
{
    float sum[3] = {.0f, .0f, .0f};
    unsigned s = 0;
    int absValue = abs (value);

    // sanity check if there's anything to do
    if (absValue == 0)
        return;

    // walk over the whole image
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {

            // walk over the convolution kernel per pixel
            for (int offsetX = -absValue; offsetX < absValue + 1; ++offsetX) {
                for (int offsetY = -absValue; offsetY < absValue + 1; ++offsetY) {
                    s = 4 * ((y + offsetY) * width + x + offsetX);
                    if (x + offsetX >= 0 &&
                        x + offsetX <= width &&
                        y + offsetY >= 0 &&
                        y + offsetY <= height) {
                        sum[0] += src[s] * src[s];
                        sum[1] += src[s+1] * src[s+1];
                        sum[2] += src[s+2] * src[s+2];
                    }
                }
            }

            // normalize result
            float avg = 2.f * absValue + 1.f;
            avg *= avg;
            sum[0] /= avg;
            sum[1] /= avg;
            sum[2] /= avg;

            // write final pixel
            unsigned d = 4 * (y * width + x);
            dst[d+0] = clamp ((unsigned) sqrt (sum[0]));
            dst[d+1] = clamp ((unsigned) sqrt (sum[1]));
            dst[d+2] = clamp ((unsigned) sqrt (sum[2]));
        }
    }
}

void pixelSumHoriz (const uchar* src,
                    uchar* dst,
                    int y,
                    int width,
                    int value)
{
    float sum[3] = {.0f, .0f, .0f};
    float lastSum[3] = {.0f, .0f, .0f};
    int s = 0;
    float avg = 2.f * value + 1.f;

    // walk a full row of pixels in the image
    for (int x = 0; x < width; ++x) {
        // calculate the full convolution sum/average, if at the beginning...
        if (x == 0) {
            for (int offset = -value; offset < value + 1; ++offset) {
                s = 4 * (y * width + x + offset);
                if (x + offset >= 0 && x + offset <= width) {
                    sum[0] += src[s] * src[s];
                    sum[1] += src[s+1] * src[s+1];
                    sum[2] += src[s+2] * src[s+2];
                }
            }

            sum[0] /= avg;
            sum[1] /= avg;
            sum[2] /= avg;

        } else {
            // ... otherwise use the sliding window shortcut
            int c = 4 * (y * width + x + value);
            int l = 4 * (y * width + x - value - 1);
            float curr[3] = {.0f, .0f, .0f};
            float last[3] = {.0f, .0f, .0f};
            if (x + value < width) {
                curr[0] = src[c];
                curr[1] = src[c+1];
                curr[2] = src[c+2];
            }
            if (x - value - 1 >= 0) {
                last[0] = src[l];
                last[1] = src[l+1];
                last[2] = src[l+2];
            }
            sum[0] = lastSum[0] + (curr[0] * curr[0] - last[0] * last[0]) / avg;
            sum[1] = lastSum[1] + (curr[1] * curr[1] - last[1] * last[1]) / avg;
            sum[2] = lastSum[2] + (curr[2] * curr[2] - last[2] * last[2]) / avg;
        }

        // update the destination with the calculated average
        unsigned d = 4 * (y * width + x);
        dst[d] = clamp ((unsigned) sqrt (sum[0]));
        dst[d+1] = clamp ((unsigned) sqrt (sum[1]));
        dst[d+2] = clamp ((unsigned) sqrt (sum[2]));

        lastSum[0] = sum[0];
        lastSum[1] = sum[1];
        lastSum[2] = sum[2];
    }
}

void pixelSumVert (const uchar* src,
                   uchar* dst,
                   int x,
                   int width,
                   int height,
                   int value)
{
    float sum[3] = {.0f, .0f, .0f};
    float lastSum[3] = {.0f, .0f, .0f};
    int s = 0;
    float avg = 2.f * value + 1.f;

    // walk a full column of pixels in the image
    for (int y = 0; y < height; ++y) {
        // calculate the full convolution sum/average, if at the beginning...
        if (y == 0) {
            for (int offset = -value; offset < value + 1; ++offset) {
                s = 4 * ((y + offset) * width + x);
                if (y + offset >= 0 && y + offset <= height) {
                    sum[0] += src[s] * src[s];
                    sum[1] += src[s+1] * src[s+1];
                    sum[2] += src[s+2] * src[s+2];
                }
            }

            sum[0] /= avg;
            sum[1] /= avg;
            sum[2] /= avg;
        } else {
            // ... otherwise use the sliding window shortcut
            int c = 4 * ((y + value) * width + x);
            int l = 4 * ((y - value - 1) * width + x);
            float curr[3] = {.0f, .0f, .0f};
            float last[3] = {.0f, .0f, .0f};
            if (y + value < height) {
                curr[0] = src[c];
                curr[1] = src[c+1];
                curr[2] = src[c+2];
            }
            if (y - value - 1 >= 0) {
                last[0] = src[l];
                last[1] = src[l+1];
                last[2] = src[l+2];
            }
            sum[0] = lastSum[0] + (curr[0] * curr[0] - last[0] * last[0]) / avg;
            sum[1] = lastSum[1] + (curr[1] * curr[1] - last[1] * last[1]) / avg;
            sum[2] = lastSum[2] + (curr[2] * curr[2] - last[2] * last[2]) / avg;
        }

        // update the destination with the calculated average
        unsigned d = 4 * (y * width + x);
        dst[d] = clamp ((unsigned) sqrt (sum[0]));
        dst[d+1] = clamp ((unsigned) sqrt (sum[1]));
        dst[d+2] = clamp ((unsigned) sqrt (sum[2]));

        lastSum[0] = sum[0];
        lastSum[1] = sum[1];
        lastSum[2] = sum[2];
    }
}

void boxBlur2PassCPP (const uchar* src,
                      uchar* scratch,
                      uchar* dst,
                      unsigned width,
                      unsigned height,
                      int value)
{
    // sanity check if there's anything to do
    if (value == 0)
        return;

    // horizontal pass
    for (unsigned y = 0; y < height; ++y) {
        pixelSumHoriz (src, scratch, (int) y, (int) width, abs (value));
    }

    // vertical pass
    for (unsigned x = 0; x < width; ++x) {
        pixelSumVert (scratch, dst, (int) x, (int) width, (int) height, abs (value));
    }
}

void MainWindow::changeValue (int value)
{
    const uchar* src = orig_.bits ();
    const uchar* tmp = front_.bits ();
    uchar* scratch = const_cast<uchar*> (tmp);
    uchar* dst = const_cast<uchar*> (tmp);
    unsigned int numBytes = 4 * orig_.width () * orig_.height ();

    auto start = chrono::high_resolution_clock::now ();

    switch (computeType_) {
        case ComputeType::CPPDIM1PASS :
            changeBrightness1PassCPP (src, dst, numBytes, value);
        break;

        case ComputeType::CPPDIM2PASS :
            changeBrightness2PassCPP (src,
                                 dst,
                                 orig_.width (),
                                 orig_.height (),
                                 value);
        break;

        case ComputeType::ASM :
            changeBrightnessASM (src, dst, numBytes, value);
        break;

        case ComputeType::SSSE3 :
            changeBrightnessSSSE3 (src, dst, numBytes, value);
        break;

        case ComputeType::CPPBLUR1PASS :
            boxBlur1PassCPP (src, dst, orig_.width (), orig_.height (), value);
        break;

        case ComputeType::CPPBLUR2PASS :
            boxBlur2PassCPP (src,
                       scratch,
                       dst,
                       orig_.width (),
                       orig_.height (),
                       value);
        break;

        case ComputeType::SSSE3MT :
            promise<Input> p1;
            promise<Input> p2;
            promise<Input> p3;
            promise<Input> p4;

            future<Input> fInput1 = p1.get_future ();
            future<Input> fInput2 = p2.get_future ();
            future<Input> fInput3 = p3.get_future ();
            future<Input> fInput4 = p4.get_future ();

            future<void> f1 (async (launch::async,
                                    changeBrightnessSSSE3threaded,
                                    ref (fInput1)));
            future<void> f2 (async (launch::async,
                                    changeBrightnessSSSE3threaded,
                                    ref (fInput2)));
            future<void> f3 (async (launch::async,
                                    changeBrightnessSSSE3threaded,
                                    ref (fInput3)));
            future<void> f4 (async (launch::async,
                                    changeBrightnessSSSE3threaded,
                                    ref (fInput4)));

            int chunkSize = numBytes / 4;
            Input input = {src, dst, chunkSize, value};
            p1.set_value (input);
            input = {src + chunkSize, dst+ chunkSize, chunkSize, value};
            p2.set_value (input);
            input = {src + 2 * chunkSize,
                     dst + 2 * chunkSize,
                     chunkSize,
                     value};
            p3.set_value (input);
            input = {src + 3 * chunkSize,
                     dst + 3 * chunkSize,
                     chunkSize,
                     value};
            p4.set_value (input);

            f1.get ();
            f2.get ();
            f3.get ();
            f4.get ();
        break;
    }

    auto diff = chrono::duration_cast<chrono::milliseconds> (chrono::high_resolution_clock::now () - start);
    stringstream title;
    title << "Time (ms): " << setprecision (4) << diff.count ();
    string str (title.str ());
    ui_->labelTime->setText (str.c_str ());

    pixmap_.convertFromImage (front_);
    QSize size (pixmap_.size().width() * scale_,
                pixmap_.size().height() * scale_);
    imageDisplay_->setPixmap (pixmap_.scaled (size,
                                              Qt::KeepAspectRatio,
                                              Qt::FastTransformation));
}

void MainWindow::on_pushButton_clicked ()
{
    disconnect (fileDialog_.get (), 0, 0, 0);
    connect (fileDialog_.get (),
             SIGNAL (fileSelected (const QString)),
             this,
             SLOT (imageSelected (const QString)));
    fileDialog_->setWindowTitle ("Select an image...");
    fileDialog_->show ();
}

void MainWindow::imageSelected (const QString& image)
{
    orig_ = QImage (image);
    front_ = orig_;
    imageDisplay_ = make_shared<QLabel> ();
    pixmap_ = QPixmap ();
    pixmap_.convertFromImage (front_);
    imageDisplay_->setPixmap (pixmap_);
    imageDisplay_->setSizePolicy(QSizePolicy::Ignored,
                                 QSizePolicy::Ignored);
    ui_->scrollArea->setWidget (imageDisplay_.get ());
    ui_->scrollArea->setWidgetResizable (true);
    ui_->labelTime->setText ("Time (ms): 0");
    ui_->labelSize->setText ("Size: " +
                             QString::number (orig_.width ()) +
                             "x" +
                             QString::number (orig_.height ())
                             + "  -");
    setWindowIcon (QIcon (pixmap_.scaledToWidth (256,
                                                 Qt::FastTransformation)));
}
void MainWindow::changeComputeType (int value)
{
    computeType_ = (ComputeType) value;
}

void MainWindow::keyPressEvent (QKeyEvent* event)
{
    if (event->key () == Qt::Key_Escape ||
        event->key () == Qt::Key_Q) {
        close ();
    }
}

void MainWindow::wheelEvent (QWheelEvent* event)
{
    QPoint p = event->angleDelta();

    if (p.y () < 0) {
        scale_ *= (scale_ >= .25 ? .95 : 1.);
    } else {
        scale_ *= (scale_ <= 3. ? 1.05 : 1.);
    }

    QSize size (pixmap_.size().width() * scale_,
                pixmap_.size().height() * scale_);
    imageDisplay_->setPixmap (pixmap_.scaled (size,
                                              Qt::KeepAspectRatio,
                                              Qt::FastTransformation));
}

void MainWindow::mousePressEvent (QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        lmb_ = true;
        last_ = event->pos();
    }
}

void MainWindow::mouseReleaseEvent (QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        lmb_ = false;
    }
}

void MainWindow::mouseMoveEvent (QMouseEvent* event)
{
    if (lmb_) {
        offset_ = event->pos () - last_;
    }

    last_ = event->pos ();
    update ();
}

void MainWindow::update ()
{
    ui_->scrollArea->scroll (offset_.x (), offset_.y ());
}
