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
 * Copyright (C) 2002-2009 by Thomas Dreibholz
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

// #define FRACTALPOOLUSER_USE_KDE

#ifdef FRACTALPOOLUSER_USE_KDE
#include <kapplication.h>
#include <kmainwindow.h>
#endif

#include <QMainWindow>
#include <QThread>
#include <QMutex>
#include <QImage>
#include <QDir>

#include "tdtypes.h"
#include "rserpool.h"
#include "fractalgeneratorpackets.h"


class FractalPU;


class ImageDisplay : public QWidget
{
   Q_OBJECT

   // ====== Public methods =================================================
   public:
   ImageDisplay(QWidget* parent = NULL);
   ~ImageDisplay();

   void initialize(const size_t width, const size_t height);
   void destroy();

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

   inline void lock() {
      ImageMutex.lock();
   }

   inline void unlock() {
      ImageMutex.unlock();
   }


   // ====== Protected methods ==============================================
   protected:
   void paintEvent(QPaintEvent* paintEvent);

   
   // ====== Private data ===================================================
   public:   // ?????????????????
   QImage* Image;
   QMutex  ImageMutex;
};


class FractalCalculationThread : public QThread
{
   Q_OBJECT

   // ====== Public methods =================================================
   public:
   FractalCalculationThread(FractalPU*         fractalPU,
                            const unsigned int threadID,
                            const size_t       viewX,
                            const size_t       viewY,
                            const size_t       viewWidth,
                            const size_t       viewHeight,
                            const bool         showStatus);
   virtual void run();

   inline bool getSuccess() const {
      return(Success);
   }

   // ====== Signals ========================================================
   signals:
   void updateImage(int start, int end);
   void updateStatus(QString statusText);

   // ====== Private methods and data =======================================
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
   bool         Success;
   int          Session;
   size_t       PoolElementUsages;
};


class FractalPU
#ifdef FRACTALPOOLUSER_USE_KDE
   : public KMainWindow
#else
   : public QMainWindow
#endif
{
   Q_OBJECT

   // ====== Definitions ====================================================
   public:
   enum FractalGeneratorStatus {
      FPU_Shutdown       = 0,
      FPU_CalcAborted    = 1,
      FPU_CalcInProgress = 2,
      FPU_Completed      = 3
   };

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

   
   // ====== Public methods =================================================
   public:
   FractalPU(const size_t       width,
             const size_t       height,
             const char*        poolHandle,
             const char*        configDirName,
             const unsigned int sendTimeout,
             const unsigned int recvTimeout,
             const unsigned int interImageTime,
             const char*        imageStoragePrefix,
             const bool         showFailoverMarks,
             const bool         showSessions,
             const size_t       threads,
             QWidget*           parent = NULL);
   ~FractalPU();

   inline const FractalParameter& getParameters() const {
      return(Parameter);
   }
   inline void setStatus(const FractalGeneratorStatus status) {
      StatusMutex.lock();
      Status = status;
      StatusMutex.unlock();
   }
   inline const FractalGeneratorStatus getStatus() {
      StatusMutex.lock();
      const FractalGeneratorStatus status = Status;
      StatusMutex.unlock();
      return(status);
   }
   inline ImageDisplay* getDisplay() {
      return(Display);
   }
   inline bool getShowFailoverMarks() const {
      return(ShowFailoverMarks);
   }
   inline bool getShowSessions() const {
      return(ShowSessions);
   }
   inline size_t getRun() const {
      return(Run);
   }
   inline const unsigned char* getPoolHandle() const {
      return(PoolHandle);
   }
   inline size_t getPoolHandleSize() const {
      return(PoolHandleSize);
   }
   inline unsigned int getSendTimeout() const {
      return(SendTimeout);
   }
   inline unsigned int getRecvTimeout() const {
      return(RecvTimeout);
   }
   

   // ====== Protected methods ==============================================
   protected:
   void closeEvent(QCloseEvent* closeEvent);
   void resizeEvent(QResizeEvent* resizeEvent);
   void contextMenuEvent(QContextMenuEvent* event);

   
   // ====== Slots ==========================================================
   public slots:
   void countDown();
   void startNextJob();
   void handleCompletedSession();
   void redrawImage(int start, int end);
   void changeStatus(QString statusText);
   void changeShowFailoverMarks(bool checked);
   void changeShowSessions(bool checked);
   void changeThreads(QAction* action);
   void about();
   void quit();

   
   // ====== Private methods and data =======================================
   private:
   void getNextParameters();

   FractalCalculationThread** CalculationThreadArray;
   FractalParameter           Parameter;
   FractalGeneratorStatus     Status;
   QMutex                     StatusMutex;
   ImageDisplay*              Display;
   size_t                     Run;

   const unsigned char*       PoolHandle;
   size_t                     PoolHandleSize;
   unsigned int               SendTimeout;
   unsigned int               RecvTimeout;
   unsigned int               InterImageTime;
   bool                       ShowFailoverMarks;
   bool                       ShowSessions;
   size_t                     ConfiguredThreads;
   size_t                     CurrentThreads;

   QStringList                ConfigList;
   QDir                       ConfigDirectory;
   QString                    ImageStoragePrefix;
   size_t                     FileNumber;
   bool                       Success;
   unsigned int               CountDown;
};


#endif
