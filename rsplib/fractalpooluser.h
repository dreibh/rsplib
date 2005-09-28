/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
 *
 * $Id$
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Contact: rsplib-discussion@sctp.de
 *          dreibh@iem.uni-due.de
 *
 */

#ifndef FRACTALPOOLUSER_H
#define FRACTALPOOLUSER_H


#include <qmainwindow.h>
#include <qthread.h>
#include <qimage.h>
#include <qdir.h>

#include "fractalgeneratorpackets.h"


class FractalPU : public QMainWindow,
                  public QThread
{
   Q_OBJECT

   public:
   FractalPU(const size_t width,
             const size_t height,
             const char*  poolHandle,
             const char*  configDirName,
             QWidget*     parent = NULL,
             const char*  name   = NULL);
   ~FractalPU();

   void restartCalculation();

   inline unsigned int getPoint(const size_t x, const size_t y) {
      return(Image->pixel(x, y));
   }

   inline void setPoint(const size_t x, const size_t y, const unsigned int value) {
      Image->setPixel(x, y, value);
   }


   protected:
   void paintEvent(QPaintEvent* paintEvent);
   void closeEvent(QCloseEvent* closeEvent);


   private:
   virtual void run();
   bool sendParameter();
   void getNextParameters();
   void paintImage(const size_t startY, const size_t endY);

   enum DataStatus {
      Okay      = 0,
      Finalizer = 1,
      Invalid   = 2
   };
   DataStatus handleData(const FGPData* data,
                         const size_t   size);

   bool                      Running;
   QImage*                   Image;

   const unsigned char*      PoolHandle;
   size_t                    PoolHandleSize;
   int                       Session;
   FGPParameter              Parameter;
   size_t                    Run;
   size_t                    PoolElementUsages;

   QStringList               ConfigList;
   QDir                      ConfigDirectory;
};


#endif
