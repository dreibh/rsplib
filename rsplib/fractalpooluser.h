/* $Id$
 * --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //       //   //===//
 *             //    //  //        //    //  //       //   //    //
 *            //===//   //=====   //===//   //       //   //===<<
 *           //   \\         //  //        //       //   //    //
 *          //     \\  =====//  //        //=====  //   //===//    Version II
 *
 * ------------- An Efficient RSerPool Prototype Implementation -------------
 *
 * Copyright (C) 2002-2008 by Thomas Dreibholz
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact: dreibh@iem.uni-due.de
 */

#ifndef FRACTALPOOLUSER_H
#define FRACTALPOOLUSER_H


#include <qthread.h>
#include <qimage.h>
#include <qdir.h>
#include <qstatusbar.h>

#include "tdtypes.h"
#include "rserpool.h"
#include "fractalgeneratorpackets.h"


class FractalPU;


class FractalCalculationThread : public QThread
{
   public:
   FractalCalculationThread(FractalPU*         fractalPU,
                            const unsigned int threadID,
                            const size_t       viewX,
                            const size_t       viewY,
                            const size_t       viewWidth,
                            const size_t       viewHeight,
                            const bool         showStatus,
                            const bool         colorMarks);
   virtual void run();

   inline bool getSuccess() const {
      return(Success);
   }

   private:
   enum DataStatus {
      Okay      = 0,
      Finalizer = 1,
      Invalid   = 2
   };

   bool sendParameterMessage();
   DataStatus handleDataMessage(const FGPData* data,
                                const size_t   size);

   unsigned int ThreadID;
   FractalPU*   Master;
   size_t       ViewX;
   size_t       ViewY;
   size_t       ViewWidth;
   size_t       ViewHeight;
   bool         ShowStatus;
   bool         ColorMarks;
   bool         Success;
   int          Session;
   size_t       PoolElementUsages;
};


class ImageDisplay : public QWidget
{
   Q_OBJECT

   public:
   ImageDisplay(QWidget*    parent = NULL,
                const char* name   = NULL);
   ~ImageDisplay();

   void initialize(const size_t width, const size_t height);
   void destroy();
   void paintImage(const size_t startY, const size_t endY);

   inline unsigned int getPixel(const size_t x, const size_t y) {
      return(Image->pixel(x, y));
   }

   inline void setPixel(const size_t x, const size_t y, const unsigned int color) {
      if( (y < (size_t)Image->height()) &&
          (x < (size_t)Image->width()) ) {
         Image->setPixel(x, y, color);
      }
   }

   inline void fillRect(const size_t x,     const size_t y,
                        const size_t width, const size_t height,
                        const unsigned int color) {
      if(Image) {
         for(size_t j = y;j < min(y + height, (size_t)Image->height());j++) {
            for(size_t i = x;i < min(x + width, (size_t)Image->width());i++) {
               Image->setPixel(i, j, color);
            }
         }
      }
   }

   protected:
   void paintEvent(QPaintEvent* paintEvent);

   public:
   QImage* Image;
};


class FractalPU : public QWidget,
                  public QThread
{
   Q_OBJECT

   friend class FractalCalculationThread;

   public:
   FractalPU(const size_t       width,
             const size_t       height,
             const char*        poolHandle,
             const char*        configDirName,
             const unsigned int sendTimeout,
             const unsigned int recvTimeout,
             const unsigned int interImageTime,
             const char*        imageStoragePrefix,
             const bool         colorMarks,
             const size_t       threads,
             QWidget*           parent = NULL,
             const char*        name   = NULL);
   ~FractalPU();


   protected:
   void closeEvent(QCloseEvent* closeEvent);
   void resizeEvent(QResizeEvent* resizeEvent);


   private:
   virtual void run();
   void getNextParameters();

   struct FractalParameter
   {
      unsigned int Width;
      unsigned int Height;
      unsigned int MaxIterations;
      unsigned int AlgorithmID;
      double       C1Real;
      double       C1Imag;
      double       C2Real;
      double       C2Imag;
      double       N;
   };

   enum FractalGeneratorStatus {
      FPU_Shutdown       = 0,
      FPU_CalcAborted    = 1,
      FPU_CalcInProgress = 2
   };

   FractalCalculationThread** CalculationThreadArray;
   FractalParameter           Parameter;
   FractalGeneratorStatus     Status;
   ImageDisplay*              Display;
   QStatusBar*                StatusBar;
   size_t                     Run;

   const unsigned char*       PoolHandle;
   size_t                     PoolHandleSize;
   unsigned int               SendTimeout;
   unsigned int               RecvTimeout;
   unsigned int               InterImageTime;
   bool                       ColorMarks;
   size_t                     Threads;

   QStringList                ConfigList;
   QDir                       ConfigDirectory;
   QString                    ImageStoragePrefix;
   size_t                     FileNumber;
};


#endif
