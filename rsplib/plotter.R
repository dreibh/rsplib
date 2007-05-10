# ###########################################################################
#             Thomas Dreibholz's R Simulation Scripts Collection
#                  Copyright (C) 2005-2007 Thomas Dreibholz
#
#           Author: Thomas Dreibholz, dreibh@exp-math.uni-essen.de
# ###########################################################################
# $Id: plotter.R 1614 2007-05-09 09:38:16Z dreibh $


# Get array of gray tones (equivalent of rainbow() for b/w laser printing)
graybow <- function(n)
{
   maxGray <- 0.5   # Light gray
   minGray <- 0.0   # Black
   return(gray(seq(maxGray, minGray, -((maxGray - minGray) / n))))
}


# Get background color
cmColor <- 2
cmGrayScale <- 1
cmBlackAndWhite <- 0
getBackgroundColor <- function(index, colorMode = cmColor, pStart = 0)
{
   if(colorMode == cmColor) {
      bgColorSet <- c("#ffffe0", "#ffe0ff", "#e0ffff", "#ffe0e0", "#e0ffe0", "#e0e0ff", "#e0e0c0", "#eeeeee", "#000000")
   }
   else if(colorMode == cmGrayScale) {
      bgColorSet <- c("#ffffff", "#f8f8f8", "#f0f0f0", "#e8e8e8", "#e0e0e0", "#d8d8d8", "#d0d0d0", "#c8c8c8", "#000000")
   }
   else {
      bgColorSet <- c("#ffffff", "#ffffff", "#ffffff", "#ffffff", "#ffffff", "#ffffff", "#ffffff", "#ffffff", "#ffffff")
   }

   bgColor <- bgColorSet[1 + ((pStart + index) %% length(bgColorSet))]
   return(bgColor)
}


titleRegExpr <- "([^\[\{]*)([\{]([^\}]*)[\}]){0,1}([^\[]*)([\[]([^\]]*)[\]]){0,1}"
removeSpaceRegExpr <- "([[:space:]]*)([^ ]*)([[:space:]]*)"


# Extract variable from axis title
getVariable <- function(title)
{
   result <- sub(extended=TRUE, titleRegExpr, "\\1", title)
   return(result)
}


# Extract abbreviated variable name from axis title
getAbbreviation <- function(title)
{
   result <- sub(extended=TRUE, titleRegExpr, "\\3", title)
   if(result == "") {
      return(getVariable(title))
   }
   return(result)
}


# Extract unit from axis title
getUnit <- function(title)
{
   result <- sub(extended=TRUE, titleRegExpr, "\\6", title)
   return(result)
}




# Check sets
checkSets <- function(data,
                      xSet=c(), ySet=c(), zSet=c(),
                      vSet=c(), wSet=c(),
                      aSet=c(), bSet=c(), pSet=c(),
                      runNoSet=c())
{
   if(length(xSet) < 1) {
      stop("ERROR: checkSets: xSet is empty!")
   }
   if( (length(xSet) != length(ySet)) ) {
      stop("ERROR: checkSets: xSet and ySet lengths differ!")
   }
   if( (length(zSet) > 0) && (length(xSet) != length(zSet)) ) {
      stop("ERROR: checkSets: xSet and zSet length differ!")
   }
   if( (length(vSet) > 0) && (length(xSet) != length(vSet)) ) {
      stop("ERROR: checkSets: xSet and vSet length differ!")
   }
   if( (length(wSet) > 0) && (length(xSet) != length(wSet)) ) {
      stop("ERROR: checkSets: xSet and wSet length differ!")
   }
   if( (length(aSet) > 0) && (length(xSet) != length(aSet)) ) {
      stop("ERROR: checkSets: xSet and aSet length differ!")
   }
   if( (length(bSet) > 0) && (length(xSet) != length(bSet)) ) {
      stop("ERROR: checkSets: xSet and bSet length differ!")
   }
   if( (length(pSet) > 0) && (length(xSet) != length(pSet)) ) {
      stop("ERROR: checkSets: xSet and pSet length differ!")
   }

   if(length(runNoSet) > 0) {
      runs <- length(levels(factor(runNoSet)))

      zFilter <- TRUE
      if(length(zSet) > 0) {
         zFilter <- (zSet == zSet[1])
      }
      vFilter <- TRUE
      if(length(vSet) > 0) {
         vFilter <- (vSet == vSet[1])
      }
      wFilter <- TRUE
      if(length(wSet) > 0) {
         wFilter <- (wSet == wSet[1])
      }
      aFilter <- TRUE
      if(length(aSet) > 0) {
         aFilter <- (aSet == aSet[1])
      }
      bFilter <- TRUE
      if(length(bSet) > 0) {
         bFilter <- (bSet == bSet[1])
      }
      pFilter <- TRUE
      if(length(pSet) > 0) {
         pFilter <- (pSet == pSet[1])
      }

      filter <- (xSet == xSet[1]) &
                zFilter & vFilter & wFilter & aFilter & bFilter & pFilter
      ySubset <- subset(data$ValueNo, filter)
      n <- length(ySubset)

      if(n != runs) {
         cat(sep="", "ERROR: checkSets: Number of values differs from number of runs!\n",
                     "      n=", n, " expected=", runs, "\n",
                     "      ySubset=")
         print(ySubset)
         stop("Aborted.")
      }
   }
}


# Default hbar aggregator
hbarDefaultAggregator <- function(xSet, ySet, hbarSet, zValue, confidence)
{
   mSet <- ySet
   mMean <- mean(mSet)
   mMin <- min(mSet)
   mMax <- max(mSet)
   if(mMin != mMax) {
      mTest <- t.test(mSet, conf.level=confidence)
      mMin  <- mTest$conf.int[1]
      mMax  <- mTest$conf.int[2]
   }
   return(c(mMean,mMin,mMax))
}


hbarHandlingSpeedAggregator <- function(xSet, ySet, hbarSet, zValue, confidence)
{
   handlingTime  <- 60 * (xSet - hbarSet)
   handlingSpeed <- ySet
   totalHandlingTime <- 0
   totalJobSize <- 0
   for(i in seq(1, length(handlingTime))) {
      totalHandlingTime <- totalHandlingTime + handlingTime[i]
      totalJobSize <- totalJobSize + (handlingTime[i] * handlingSpeed[i])
   }
   mMean <- totalJobSize / totalHandlingTime
   mMin <- mMean
   mMax <- mMean
   return(c(mMean,mMin,mMax))
}


# Plot x/y plot with different curves as z with confidence intervals in
# y direction. x and z can be numeric or strings, y must be numeric since
# confidence intervals have to be computed.
plotstd3 <- function(mainTitle,
                     xTitle, yTitle, zTitle,
                     xSet, ySet, zSet,
                     vSet              = c(),
                     wSet              = c(),
                     vTitle            = "??vTitle??",
                     wTitle            = "??wTitle??",
                     xAxisTicks        = c(),
                     yAxisTicks        = c(),
                     confidence        = 0.95,
                     hbarSet           = c(),
                     hbarMeanSteps     = 10,
                     hbarAggregator    = hbarDefaultAggregator,
                     xSeparatorsSet    = c(),
                     xSeparatorsTitles = c(),
                     xSeparatorsColors  = c(),
                     type              = "lines",
                     hideLegend        = FALSE,
                     legendOnly        = FALSE,
                     legendPos         = c(0,1),
                     colorMode         = FALSE,
                     zColorArray       = c(),
                     frameColor        = par("fg"),
                     legendSizeFactor  = 0.8,
                     xValueFilter      = "%s",
                     yValueFilter      = "%s",
                     zValueFilter      = "%s",
                     vValueFilter      = "%s",
                     wValueFilter      = "%s")
{
   xLevels <- levels(factor(xSet))
   yLevels <- levels(factor(ySet))
   zLevels <- levels(factor(zSet))
   if(length(xLevels) < 1) {
      cat("WARNING: plotstd3() - xLevels=c()\n")
      return(0);
   }
   if(length(yLevels) < 1) {
      cat("WARNING: plotstd3() - yLevels=c()\n")
      return(0);
   }
   if(length(zLevels) < 1) {
      cat("WARNING: plotstd3() - zLevels=c()\n")
      return(0);
   }
   if(length(vSet) < 1) {
      vSet <- rep(0, length(zSet))
   }
   vLevels <- levels(factor(vSet))
   if(length(wSet) < 1) {
      wSet <- rep(0, length(zSet))
   }
   wLevels <- levels(factor(wSet))

   if(length(zColorArray) == 0) {
      if(colorMode == cmColor) {
         if(length(zLevels) <= 4) {
            zColorArray <- rainbow(length(zLevels))
         }
         else {
            zColorArray <- rainbow(length(zLevels), gamma=2)
         }
      }
      else if(colorMode == cmGrayScale) {
         zColorArray <- graybow(length(zLevels))
         frameColor  <- par("fg")
      }
      else {
         zColorArray <- rep(par("fg"), length(zLevels))
         frameColor  <- par("fg")
      }
   }

   xRange <- range(as.numeric(xSet), finite=TRUE)
   yRange <- range(as.numeric(ySet), finite=TRUE)
   if(missing(xAxisTicks) || (length(xAxisTicks) == 0) || nlevels(xSet)) {
      xAxisTicks <- xLevels
   }
   else {
      xRange <- range(as.numeric(xAxisTicks), finite=TRUE)
   }
   if(missing(yAxisTicks) || (length(yAxisTicks) == 0)) {
      yAxisTicks <- yLevels
   }
   else {
      yRange <- range(as.numeric(yAxisTicks), finite=TRUE)
   }


   # ------ Create plot window ----------------------------------------------
   plot.new()
   plot.window(xRange, yRange)
   if(!legendOnly) {
      if(nlevels(xSet)) {
         axis(1, seq(length(xLevels)), xLevels, col=frameColor, col.axis=frameColor)
      }
      else {
         axis(1, xAxisTicks, col=frameColor, col.axis=frameColor)
      }
      axis(2, yAxisTicks, col=frameColor, col.axis=frameColor)

      grid(20, 20, lty=1)
      box(col=frameColor)
      xLabel <- getVariable(xTitle)
      if(getAbbreviation(xTitle) != getVariable(xTitle)) {
         xLabel <- paste(sep="", xLabel, " ", getAbbreviation(xTitle), "")
      }
      if(getUnit(xTitle) != "") {
         xLabel <- paste(sep="", xLabel, " [", getUnit(xTitle), "]")
      }
      yLabel <- getVariable(yTitle)
      if(getAbbreviation(yTitle) != getVariable(yTitle)) {
         yLabel <- paste(sep="", yLabel, " ", getAbbreviation(yTitle), "")
      }
      if(getUnit(yTitle) != "") {
         yLabel <- paste(sep="", yLabel, " [", getUnit(yTitle), "]")
      }

      mtext(xLabel, col=frameColor,
            side = 1, at = xRange[1] + ((xRange[2] - xRange[1]) / 2), line=2.5,
            xpd = NA, font = par("font.lab"), cex = par("cex.lab"))
      mtext(yLabel, col=frameColor,
            side = 2, at = yRange[1] + ((yRange[2] - yRange[1]) / 2), line=2.5,
            xpd = NA, font = par("font.lab"), cex = par("cex.lab"))
      mtext(mainTitle, col=frameColor,
            side=3, at=xRange[1] + ((xRange[2] - xRange[1]) / 2), line=2.5,
            xpd = NA, font = par("font.main"), cex = par("cex.main"))

      zvwLabel <- getVariable(zTitle)
      if(getAbbreviation(zTitle) != getVariable(zTitle)) {
         zvwLabel <- paste(sep="", zvwLabel, " ", getAbbreviation(zTitle), "")
      }
      if(getUnit(zTitle) != "") {
         zvwLabel <- paste(sep="", zvwLabel, " [", getUnit(zTitle), "]")
      }
      if(length(vLevels) > 1) {
         zvwLabel <- paste(sep="", zvwLabel, " / ", getVariable(vTitle))
         if(getAbbreviation(vTitle) != getVariable(vTitle)) {
            zvwLabel <- paste(sep="", zvwLabel, " ", getAbbreviation(vTitle), "")
         }
         if(getUnit(vTitle) != "") {
            zvwLabel <- paste(sep="", zvwLabel, " [", getUnit(vTitle), "]")
         }
      }
      if(length(wLevels) > 1) {
         zvwLabel <- paste(sep="", zvwLabel, " / ", getVariable(wTitle))
         if(getAbbreviation(wTitle) != getVariable(wTitle)) {
            zvwLabel <- paste(sep="", zvwLabel, " ", getAbbreviation(wTitle), "")
         }
         if(getUnit(wTitle) != "") {
            zvwLabel <- paste(sep="", zvwLabel, " [", getUnit(wTitle), "]")
         }
      }
      mtext(zvwLabel, col=frameColor,
            side = 3, at = max(xRange), line=0.5, adj=1,
            xpd = NA, font = par("font.lab"), cex = par("cex.lab"))
   }


   # ------ Plot curves -----------------------------------------------------
   legendTexts  <- c()
   legendColors <- c()
   legendStyles <- c()
   legendDots   <- c()
   legendDot    <- 1
   for(zPosition in rev(seq(1, length(zLevels)))) {
      z <- zLevels[zPosition]
      for(wPosition in seq(1, length(wLevels))) {
         w <- wLevels[wPosition]
         for(vPosition in seq(1, length(vLevels))) {
            v <- vLevels[vPosition]

            # ----- Legend settings -----------------------------------------
            if((length(vLevels) > 1) ||
               (length(wLevels) > 1)) {
               legendText <- paste(sep="", getAbbreviation(zTitle), "=", gettextf(zValueFilter, z))
            }
            else {
               legendText  <- paste(sep="", z)
            }
            if(length(vLevels) > 1) {
               legendText <- paste(sep="", legendText, ", ", getAbbreviation(vTitle), "=", gettextf(vValueFilter, v))
            }
            if(length(wLevels) > 1) {
               legendText <- paste(sep="", legendText, ", ", getAbbreviation(wTitle), "=", gettextf(wValueFilter, w))
            }
            legendColor <- zColorArray[zPosition]
            legendStyle <- vPosition %% 7

            if(!legendOnly) {
               # ----- Points plot ------------------------------------------
               if((type == "p") || (type=="points")) {
                  xSubset <- subset(xSet, (zSet == z) & (vSet == v) & (wSet == w))
                  ySubset <- subset(ySet, (zSet == z) & (vSet == v) & (wSet == w))
                  points(xSubset, ySubset, col=legendColor, cex=par("cex"), pch=legendDot)

                  legendTexts <- append(legendTexts, legendText)
                  legendColors <- append(legendColors, legendColor)
                  legendStyles <- append(legendStyles, legendStyle)
                  legendDots <- append(legendDots, legendDot)
               }

               # ----- Horizontal bars plot ---------------------------------
               else if((type == "h") || (type=="hbars")) {
                  xSubset <- subset(xSet, (zSet == z) & (vSet == v) & (wSet == w))
                  ySubset <- subset(ySet, (zSet == z) & (vSet == v) & (wSet == w))
                  hbarSubset <- subset(hbarSet, (zSet == z) & (vSet == v) & (wSet == w))
                  for(x in seq(1, length(xSubset))) {
                     # points(xSubset[x],ySubset[x], col=legendColor, cex=par("cex"), pch=legendDot)
                     lines(c(hbarSubset[x], xSubset[x]),
                           c(ySubset[x], ySubset[x]),
                           col=legendColor, cex=par("cex"), pch=legendDot)
                  }
                  legendTexts  <- append(legendTexts, legendText)
                  legendColors <- append(legendColors, legendColor)
                  legendStyles <- append(legendStyles, legendStyle)
                  legendDots   <- append(legendDots, legendDot)
               }

               # ----- Lines or Steps plot ----------------------------------
               else if((type == "l") || (type=="lines") ||
                       (type == "s") || (type=="steps")) {
                  # These sets will contain mean y line and its confidence intervals
                  xPlotSet <- c()
                  yPlotMinSet <- c()
                  yPlotMeanSet <- c()
                  yPlotMaxSet <- c()
                  for(xPosition in seq(1, length(xLevels))) {
                     x <- xLevels[xPosition]

                     # ------ Calculate confidence intervals for (z,x) pos. ----
                     ySubset <- subset(ySet, (zSet == z) & (vSet == v) & (wSet == w) & (xSet == x))
                     if(length(ySubset) > 0) {
                        yMin <- min(ySubset)
                        yMean <- mean(ySubset)
                        yMax <- max(ySubset)
                        if(yMin != yMax) {
                           yTest <- t.test(ySubset, conf.level=confidence)
                           yMin <- yTest$conf.int[1]
                           yMax <- yTest$conf.int[2]
                        }

                        # ------ Add results to set ----------------------------
                        if(nlevels(xSet)) {
                           xPlotSet <- append(xPlotSet, xPosition)
                        }
                        else {
                           xPlotSet <- append(xPlotSet, as.numeric(x))
                        }
                        yPlotMinSet  <- append(yPlotMinSet, yMin)
                        yPlotMeanSet <- append(yPlotMeanSet, yMean)
                        yPlotMaxSet  <- append(yPlotMaxSet, yMax)
                     }
                  }

                  # ------ Plot line and confidence intervals ---------------
                  if(length(xPlotSet) > 0) {
                     lineWidth <- 5
                     if((length(vLevels) > 1) || (length(wLevels) > 1)) {
                        lineWidth <- 3
                     }
                     if((type == "l") || (type=="lines")) {
                        lines(xPlotSet, yPlotMeanSet,
                              col=legendColor, lty=legendStyle, lwd=lineWidth*par("cex"))
                     }
                     else if((type == "s") || (type=="steps")) {
                        for(i in seq(1, length(xPlotSet) - 1)) {
                           lines(c(xPlotSet[i], xPlotSet[i + 1]),
                                 c(yPlotMeanSet[i],yPlotMeanSet[i]),
                                 col=legendColor, lty=legendStyle, lwd=lineWidth*par("cex"))
                           lines(c(xPlotSet[i + 1], xPlotSet[i + 1]),
                                 c(yPlotMeanSet[i],yPlotMeanSet[i + 1]),
                                 col=legendColor, lty=legendStyle, lwd=lineWidth*par("cex"))
                        }
                     }

                     cintWidthFraction <- 75
                     cintWidth <- (max(xRange) - min(xRange)) / cintWidthFraction
                     for(xPosition in seq(1, length(xPlotSet))) {
                        x <- xPlotSet[xPosition]
                        lines(c(x, x),
                              c(yPlotMinSet[xPosition], yPlotMaxSet[xPosition]),
                              col=legendColor, lty=legendStyle, lwd=1*par("cex"))
                        lines(c(x - cintWidth, x + cintWidth),
                              c(yPlotMinSet[xPosition], yPlotMinSet[xPosition]),
                              col=legendColor, lty=legendStyle, lwd=1*par("cex"))
                        lines(c(x - cintWidth, x + cintWidth),
                              c(yPlotMaxSet[xPosition],yPlotMaxSet[xPosition]),
                              col=legendColor, lty=legendStyle, lwd=1*par("cex"))
                     }

                     legendTexts <- append(legendTexts, legendText)
                     legendColors <- append(legendColors, legendColor)
                     legendStyles <- append(legendStyles, legendStyle)
                     legendDots <- append(legendDots, legendDot)

                     pcex <- par("cex")
                     # if(length(wLevels) > 1) {
                        pcex <- 2 * pcex
                     # }
                     points(xPlotSet, yPlotMeanSet,
                           col=legendColor, lty=legendStyle, pch=legendDot,
                           lwd=par("cex"),
                           cex=pcex,bg="yellow")
                     legendDot <- legendDot + 1
                  }
               }

               # ----- Unknown plot type ------------------------------------
               else {
                  stop("plotstd3: Unknown plot type!")
               }
            }
         }
      }
   }


   # ----- Plot hbar mean line ----------------------------------------------
   if( ((type == "h") || (type=="hbars")) &&
       (hbarMeanSteps > 1) ) {
      xSteps <- hbarMeanSteps
      xWidth <- ((max(xSet) - min(xSet)) / xSteps)
      xSegments <- seq(min(xSet), max(xSet) - xWidth, by=xWidth)

      oldY <- c()
      for(xValue in xSegments) {
         filter <- (xSet >= xValue) &
                   (xSet <= xValue + xWidth)

         xAggSubset <- subset(xSet, filter)

         if(length(xAggSubset) > 0) {
            yAggSubset    <- subset(ySet, filter)
            hbarAggSubset <- subset(hbarSet, filter)

            aggregate <- hbarAggregator(
                           xAggSubset, yAggSubset,
                           hbarAggSubset, zValue,
                           confidence)
            mMean <- aggregate[1]
            mMin <- aggregate[2]
            mMax <- aggregate[3]

            # ------ Plot line segment -----------------------------------------
            if(colorMode == cmColor) {
               meanBarColor <- zColorArray
            }
            else {
               meanBarColor <- par("fg")
            }
            if(xValue > min(xSet)) {
               lines(c(xValue, xValue),
                     c(oldY, mMean),
                     col=meanBarColor, lwd=4*par("cex"))
            }
            lines(c(xValue, xValue + xWidth),
                  c(mMean, mMean),
                  col=meanBarColor, lwd=4*par("cex"))
            oldY <- mMean

            # ------ Plot confidence interval ----------------------------------
            if(mMin != mMax) {
               x <- xValue + (xWidth / 2)
               cintWidthFraction <- 75
               cintWidth <- (max(xSet) - min(ySet)) / cintWidthFraction
               lines(c(x, x), c(mMin, mMax),
                     col=meanBarColor, lwd=1*par("cex"))
               lines(c(x - cintWidth, x + cintWidth),
                     c(mMin, mMin),
                     col=meanBarColor, lwd=1*par("cex"))
               lines(c(x - cintWidth, x + cintWidth),
                     c(mMax, mMax),
                     col=meanBarColor, lwd=1*par("cex"))
            }
         }
      }
   }


   # ------ Plot separators -------------------------------------------------
   if(length(xSeparatorsSet) > 0) {
      if(length(xSeparatorsColors) < 1) {
         if(colorMode == cmColor) {
            xSeparatorsColors <- rainbow(length(xSeparatorsSet))
         }
         else if(colorMode == cmGrayScale) {
            xSeparatorsColors <- graybow(length(xSeparatorsSet))
         }
         else {
            xSeparatorsColors <- rep(par("fg"), length(zLevels))
            frameColor  <- par("fg")
         }
      }
      legendBackground <- "gray95"
      if(colorMode == cmBlackAndWhite) {
         legendBackground <- "white"
      }

      i <- 1
      for(xValue in xSeparatorsSet) {
         lines(c(xValue, xValue),
               c(-9e99, 9e99), lwd=4*par("cex"),
               col=xSeparatorsColors[i])
         xAdjust <- -(strwidth(xSeparatorsTitles[i]) / 2)
         rect(xValue + xAdjust, max(yAxisTicks),
              xValue + xAdjust + strwidth(xSeparatorsTitles[i]) + 2 * strwidth("i"),
              max(yAxisTicks) - strheight(xSeparatorsTitles[i]) - 1.0 * strheight("i"),
              col=legendBackground)
         text(xValue + strwidth("i"),
              max(yAxisTicks) - 0.5 * strheight(xSeparatorsTitles[i]) - 0.5 * strheight("i"),
              xSeparatorsTitles[i],
              col=xSeparatorsColors[i],
              adj=c(0.5,0.5))
         i <- i + 1
      }
   }


   # ------ Plot legend -----------------------------------------------------
   if(!hideLegend) {
      lx <- min(xRange) + ((max(xRange) - min(xRange)) * legendPos[1])
      ly <- min(yRange) + ((max(yRange) - min(yRange)) * legendPos[2])
      lxjust <- 0.5
      lyjust <- 0.5
      if(legendPos[1] < 0.5) {
         lxjust <- 0
      }
      else if(legendPos[1] > 0.5) {
         lxjust <- 1
      }
      if(legendPos[2] < 0.5) {
         lyjust <- 0
      }
      else if(legendPos[2] > 0.5) {
         lyjust <- 1
      }

      legendBackground <- "gray95"
      if(colorMode == cmBlackAndWhite) {
         legendColors <- par("fg")
         legendBackground <- "white"
      }
      legend(lx, ly,
             xjust = lxjust,
             yjust = lyjust,
             legendTexts,
             bg=legendBackground,
             col=legendColors,
             lty=legendStyles,
             pch=legendDots,
             text.col=legendColors,
             lwd=par("cex"), cex=legendSizeFactor*par("cex"))
   }
}


# 2-dimensional array of plotstd3 plots (a-dimension only)
plotstd4 <- function(mainTitle, aTitle, xTitle, yTitle, zTitle,
                     aSet, xSet, ySet, zSet,
                     vSet=c(), wSet=c(),
                     vTitle     = "??vTitle??",
                     wTitle     = "??wTitle??",
                     aAxisTicks = length(levels(factor(aSet))) > 1,
                     xAxisTicks = c(),
                     yAxisTicks = c(),
                     confidence = 0.95,
                     hbarSet           = c(),
                     hbarMeanSteps     = 10,
                     xSeparatorsSet    = c(),
                     xSeparatorsTitles = c(),
                     xSeparatorsColors  = c(),
                     type       = "lines",
                     hideLegend = FALSE,
                     legendPos  = c(0,1),
                     colorMode  = cmColor,
                     frameColor = par("fg"))
{
   aLevels <- levels(factor(aSet))
   aLevelsCount <- length(aLevels)
   if(length(aLevels) < 1) {
      cat("WARNING: plotstd4() - aLevels=c()\n")
      return(0);
   }

   if(colorMode == cmColor) {
      aLevelsColorArray <- rainbow(length(aLevels), start=0, end=3/6)
   }
   else if(colorMode == cmGrayScale) {
      aLevelsColorArray <- graybow(length(aLevels))
   }
   else {
      aLevelsColorArray <- rep(par("fg"), length(aLevels))
   }

   width  <- ceiling(sqrt(aLevelsCount))
   height <- ceiling(aLevelsCount / width)


   # ------ Get x/y tickmarks -----------------------------------------------
   if(missing(xAxisTicks) || (length(xAxisTicks) == 0)) {
      xAxisTicks = levels(factor(xSet))
      if(length(xAxisTicks) > 20) {
         unique(seq(min(xSet), max(xSet), length=10))
      }
   }
   if(missing(yAxisTicks) || (length(yAxisTicks) == 0)) {
      yAxisTicks = levels(factor(ySet))
      if(length(yAxisTicks) > 20) {
         unique(seq(min(ySet), max(ySet), length=10))
      }
   }


   # ------ Create outer margin for title and labels ------------------------
   opar <- par(oma = par("oma") + c(2,2,4,2))  # Rahmen um Gesamtarray
   on.exit(par(opar))


   # ------ Create layout ---------------------------------------------------
   layoutMatrix <- matrix(rep(0, width * 2 * height),
                          2 * height,
                          width,
                          byrow = TRUE)
   widthArray <- rep(100, width)
   heightArray <- rep(100, 2 * height)
   for(i in seq(2, 2 * height, by=2)) {
      heightArray[i] <- max(20, (100 * height) / 12)
   }

   figureIndex <- 1
   for(y in 1:height) {
      for(x in 1:width) {
         if(figureIndex <= length(aLevels)) {
            layoutMatrix[2*(y - 1) + 2, x] <- figureIndex
            figureIndex <- figureIndex + 1
         }
      }
   }
   for(y in 1:height) {
      for(x in 1:width) {
         layoutMatrix[2*(y - 1) + 1, x] <- figureIndex
         figureIndex <- figureIndex + 1
      }
   }

   nf <- layout(layoutMatrix, widthArray, heightArray, TRUE)
   # layout.show(nf)

   # ------ Create a a value labels -----------------------------------------
   for(a in 1:length(aLevels)) {
      par(mar=c(1, 1, 1, 1))
      plot.new()
      plot.window(c(0, 1), c(0, 1))
      rect(0, 0, 1, 1, col=aLevelsColorArray[a])
      text(0.5, 0.5, aLevels[a])
   }


   # ------ Plot curves -----------------------------------------------------
   labelShrinkFactor <- 0.5   # 1.0 / max(length(aLevels), length(bLevels))
   legendShrinkFactor <- 0.75

   for(aPosition in seq(1, length(aLevels))) {
      a <- aLevels[aPosition]

      oldPar <- par(mar=par("mar") + c(3,3,3,3),
                     cex.lab=par("cex.lab") * labelShrinkFactor,
                     cex=par("cex") * legendShrinkFactor)

      xSubset <- subset(xSet, (aSet == a))
      ySubset <- subset(ySet, (aSet == a))
      zSubset <- subset(zSet, (aSet == a))
      vSubset <- subset(vSet, (aSet == a))
      wSubset <- subset(wSet, (aSet == a))

      subMainTitle <- ""
      xSubTitle <- xTitle
      ySubTitle <- yTitle
      plotstd3(subMainTitle,
               xSubTitle, ySubTitle, zTitle,
               xSubset, ySubset, zSubset,
               vSubset, wSubset, vTitle, wTitle,
               xAxisTicks = xAxisTicks, yAxisTicks = yAxisTicks,
               confidence = confidence,
               hbarSet = hbarSet,
               hbarMeanSteps = hbarMeanSteps,
               xSeparatorsSet = xSeparatorsSet,
               xSeparatorsTitles = xSeparatorsTitles,
               xSeparatorsColors = xSeparatorsColors,
               type = type,
               hideLegend = hideLegend,
               legendPos = legendPos,
               colorMode = colorMode,
               frameColor=frameColor)
      par(oldPar)
   }



   # ------ Create title and labels -----------------------------------------
   mtext(mainTitle, col=frameColor, side=3, at=0.5, line=0.5, outer=TRUE,
         xpd = NA, font = par("font.main"), cex = par("cex.main"))
   mtext(aTitle, col=frameColor, side=1, at=0.5, line=0.5, outer=TRUE,
         xpd = NA, font = par("font.lab"), cex = par("cex.lab"))
}


# 2-dimensional array of plotstd3 plots
plotstd5 <- function(mainTitle, aTitle, bTitle, xTitle, yTitle, zTitle,
                     aSet, bSet, xSet, ySet, zSet,
                     vSet              = c(),
                     wSet              = c(),
                     vTitle            = "??vTitle??",
                     wTitle            = "??wTitle??",
                     aAxisTicks        = length(levels(factor(aSet))) > 1,
                     bAxisTicks        = length(levels(factor(bSet))) > 1,
                     xAxisTicks        = c(), yAxisTicks = c(),
                     confidence        = 0.95,
                     hbarSet           = c(),
                     hbarMeanSteps     = 10,
                     xSeparatorsSet    = c(),
                     xSeparatorsTitles = c(),
                     xSeparatorsColors  = c(),
                     type              = "lines",
                     hideLegend        = FALSE,
                     legendPos         = c(0,1),
                     colorMode         = cmColor,
                     frameColor        = par("fg"))
{
   aLevels <- levels(factor(aSet))
   bLevels <- levels(factor(bSet))
   if(length(aLevels) < 1) {
      cat("WARNING: plotstd5() - aLevels=c()\n")
      return(0);
   }
   if(length(bLevels) < 1) {
      cat("WARNING: plotstd5() - bLevels=c()\n")
      return(0);
   }

   if(colorMode == cmColor) {
      aLevelsColorArray <- rainbow(length(aLevels), start=0, end=3/6)
      bLevelsColorArray <- rainbow(length(bLevels), start=4/6, end=6/6)
   }
   else if(colorMode == cmGrayScale) {
      aLevelsColorArray <- graybow(length(aLevels))
      bLevelsColorArray <- graybow(length(bLevels))
   }
   else {
      aLevelsColorArray <- rep("#ffffff", length(aLevels))
      bLevelsColorArray <- rep("#ffffff", length(bLevels))
   }

   plotALabels <- 0
   plotBLabels <- 0
   if((aAxisTicks) || (length(levels(factor(aSet))) > 1)) {
      plotALabels <- 1
   }
   if((bAxisTicks) || (length(levels(factor(bSet))) > 1)) {
      plotBLabels <- 1
   }
   aLabel <- getVariable(aTitle)
   if(getAbbreviation(aTitle) != getVariable(xTitle)) {
      aLabel <- paste(sep="", aLabel, " ", getAbbreviation(aTitle), "")
   }
   if(getUnit(aTitle) != "") {
      aLabel <- paste(sep="", aLabel, " [", getUnit(aTitle), "]")
   }
   bLabel <- getVariable(bTitle)
   if(getAbbreviation(bTitle) != getVariable(bTitle)) {
      bLabel <- paste(sep="", bLabel, " ", getAbbreviation(bTitle), "")
   }
   if(getUnit(bTitle) != "") {
      bLabel <- paste(sep="", bLabel, " [", getUnit(bTitle), "]")
   }


   # ------ Get x/y tickmarks -----------------------------------------------
   if(missing(xAxisTicks) || (length(xAxisTicks) == 0)) {
      xAxisTicks = levels(factor(xSet))
      if(length(xAxisTicks) > 20) {
         unique(seq(min(xSet), max(xSet), length=10))
      }
   }
   if(missing(yAxisTicks) || (length(yAxisTicks) == 0)) {
      yAxisTicks = levels(factor(ySet))
      if(length(yAxisTicks) > 20) {
         unique(seq(min(ySet), max(ySet), length=10))
      }
   }


   # ------ Create outer margin for title and labels ------------------------
   opar <- par(oma = par("oma") + c(2,2,4,2))  # Rahmen um Gesamtarray
   on.exit(par(opar))


   # ------ Create layout ---------------------------------------------------
   layoutMatrix <- matrix(rep(0, (length(bLevels) + plotALabels) * (length(aLevels) + plotBLabels)),
                          length(bLevels) + plotALabels,
                          length(aLevels) + plotBLabels,
                          byrow = TRUE)
   widthArray <- rep(100, length(aLevels) + 1)
   if(plotBLabels > 0) {
      widthArray[1] <- max(30, (100 * length(aLevels)) / 12)
   }
   heightArray <- rep(100, length(bLevels) + 1)
   if(plotALabels > 0) {
      heightArray[length(bLevels) + 1] <- max(20, (100 * length(bLevels)) / 12)
   }

   figureIndex <- 1
   if(plotALabels > 0) {
      for(a in 1:length(aLevels)) {
         layoutMatrix[length(bLevels) + 1, a + plotBLabels] <- figureIndex
         figureIndex <- figureIndex + 1
      }
   }
   if(plotBLabels > 0) {
      for(b in 1:length(bLevels)) {
         layoutMatrix[1 + length(bLevels) - b, 1] <- figureIndex
         figureIndex <- figureIndex + 1
      }
   }
   for(b in 1:length(bLevels)) {
      for(a in 1:length(aLevels)) {
         layoutMatrix[1 + length(bLevels) - b, a + plotBLabels] <- figureIndex
         figureIndex <- figureIndex + 1
      }
   }

   nf <- layout(layoutMatrix, widthArray, heightArray, TRUE)
   # layout.show(nf)


   # ------ Create a and b value labels -------------------------------------
   if(plotALabels > 0) {
      for(a in 1:length(aLevels)) {
         par(mar=c(1, 1, 1, 1))
         plot.new()
         plot.window(c(0, 1), c(0, 1))
         rect(0, 0, 1, 1, col=aLevelsColorArray[a])
         text(0.5, 0.5, aLevels[a])
      }
   }

   if(plotBLabels > 0) {
      for(b in 1:length(bLevels)) {
         par(mar=c(1, 1, 1, 1))
         plot.new()
         plot.window(c(0, 1), c(0, 1))
         rect(0, 0, 1, 1, col=bLevelsColorArray[b])
         text(0.5, 0.5, bLevels[b], srt=90)
      }
   }


   # ------ Plot curves -----------------------------------------------------
   labelShrinkFactor <- 0.5   # 1.0 / max(length(aLevels), length(bLevels))
   legendShrinkFactor <- 0.75

   for(bPosition in seq(1, length(bLevels))) {
      b <- bLevels[bPosition]
      for(aPosition in seq(1, length(aLevels))) {
         a <- aLevels[aPosition]

         oldPar <- par(mar=par("mar") + c(3,3,3,3),
                       cex.lab=par("cex.lab") * labelShrinkFactor,
                       cex=par("cex") * legendShrinkFactor)

         xSubset <- subset(xSet, (aSet == a) & (bSet == b))
         ySubset <- subset(ySet, (aSet == a) & (bSet == b))
         zSubset <- subset(zSet, (aSet == a) & (bSet == b))
         vSubset <- subset(vSet, (aSet == a) & (bSet == b))
         wSubset <- subset(wSet, (aSet == a) & (bSet == b))

         subMainTitle <- ""
         xSubTitle <- xTitle
         ySubTitle <- yTitle
         plotstd3(subMainTitle,
                  xSubTitle, ySubTitle, zTitle,
                  xSubset, ySubset, zSubset,
                  vSubset, wSubset, vTitle, wTitle,
                  xAxisTicks = xAxisTicks, yAxisTicks = yAxisTicks,
                  confidence = confidence,
                  hbarSet = hbarSet,
                  hbarMeanSteps = hbarMeanSteps,
                  xSeparatorsSet = xSeparatorsSet,
                  xSeparatorsTitles = xSeparatorsTitles,
                  xSeparatorsColors = xSeparatorsColors,
                  type = type,
                  hideLegend=hideLegend,
                  legendPos=legendPos,
                  colorMode = colorMode,
                  frameColor=frameColor)
         par(oldPar)
      }
   }


   # ------ Create title and labels -----------------------------------------
   mtext(mainTitle, col=frameColor, side=3, at=0.5, line=0.5, outer=TRUE,
         xpd = NA, font = par("font.main"), cex = par("cex.main"))
   if(plotALabels > 0) {
      mtext(aLabel, col=frameColor,
            side=1, at=0.5, line=0.5, outer=TRUE,
            xpd = NA, font = par("font.lab"), cex = par("cex.lab"))
   }
   if(plotBLabels > 0) {
      mtext(bLabel, col=frameColor,
            side=2, at=0.5, line=0.5, outer=TRUE,
            xpd = NA, font = par("font.lab"), cex = par("cex.lab"))
   }
}


# multi-page 2-dimensional array of plotstd3 plots
plotstd6 <- function(mainTitle, pTitle, aTitle, bTitle, xTitle, yTitle, zTitle,
                     pSet, aSet, bSet, xSet, ySet, zSet,
                     vSet              = c(),
                     wSet              = c(),
                     vTitle            = "??vTitle??",
                     wTitle            = "??wTitle??",
                     aAxisTicks        = length(levels(factor(aSet))) > 1,
                     bAxisTicks        = length(levels(factor(bSet))) > 1,
                     xAxisTicks        = c(),
                     yAxisTicks        = c(),
                     confidence        = 0.95,
                     hbarSet           = c(),
                     hbarMeanSteps     = 10,
                     xSeparatorsSet    = c(),
                     xSeparatorsTitles = c(),
                     xSeparatorsColors  = c(),
                     type              = "lines",
                     pStart            = 0,
                     hideLegend        = FALSE,
                     legendPos         = c(0,1),
                     colorMode         = FALSE,
                     frameColor        = par("fg"),
                     simulationName    = "")
{
   if(length(pSet) == 0) {
      pSet <- rep(1, length(xSet))
   }
   if(length(aSet) == 0) {
      aSet <- rep(0, length(xSet))
   }
   if(length(bSet) == 0) {
      bSet <- rep(0, length(xSet))
   }
   pLevels <- levels(factor(pSet))
   index <- 0
   for(p in pLevels) {
      oldPar1 <- par(mar=c(3,3,3,3), oma=c(2,2,4,2), bg=getBackgroundColor(index, colorMode, pStart))

      xSubset <- subset(xSet, (pSet == p))
      ySubset <- subset(ySet, (pSet == p))
      zSubset <- subset(zSet, (pSet == p))
      vSubset <- subset(vSet, (pSet == p))
      wSubset <- subset(wSet, (pSet == p))
      aSubset <- subset(aSet, (pSet == p))
      bSubset <- subset(bSet, (pSet == p))
      if(pTitle != "") {
         subMainTitle <- paste(sep="", pTitle, " = ", p)
      }
      else {
         subMainTitle <- ""
      }
      if(length(pLevels) <= 1) {
         subMainTitle <- ""
      }

      if(length(levels(factor(bSet))) <= 1) {
         if(length(levels(factor(aSet))) <= 1) {
            plotstd3(subMainTitle,
                     xTitle, yTitle, zTitle,
                     xSubset, ySubset, zSubset,
                     vSubset, wSubset, vTitle, wTitle,
                     xAxisTicks = xAxisTicks, yAxisTicks = yAxisTicks,
                     confidence = confidence,
                     hbarSet = hbarSet,
                     hbarMeanSteps = hbarMeanSteps,
                     xSeparatorsSet = xSeparatorsSet,
                     xSeparatorsTitles = xSeparatorsTitles,
                     xSeparatorsColors = xSeparatorsColors,
                     type = type,
                     hideLegend = hideLegend,
                     legendPos = legendPos,
                     colorMode = colorMode,
                     frameColor = frameColor)
         }
         else {
            oldPar2 <- par(font.main=3, cex.main=0.9)
            plotstd4(subMainTitle,
                     aTitle, xTitle, yTitle, zTitle,
                     aSubset, xSubset, ySubset, zSubset,
                     vSubset, wSubset, vTitle, wTitle,
                     aAxisTicks = aAxisTicks,
                     xAxisTicks = xAxisTicks, yAxisTicks = yAxisTicks,
                     confidence = confidence,
                     hbarSet = hbarSet,
                     hbarMeanSteps = hbarMeanSteps,
                     xSeparatorsSet = xSeparatorsSet,
                     xSeparatorsTitles = xSeparatorsTitles,
                     xSeparatorsColors = xSeparatorsColors,
                     type = type,
                     hideLegend = hideLegend,
                     legendPos = legendPos,
                     colorMode = colorMode,
                     frameColor = frameColor)
            par(oldPar2)
         }
      }
      else {
         oldPar2 <- par(font.main=3, cex.main=0.9)
         plotstd5(subMainTitle,
                  aTitle, bTitle, xTitle, yTitle, zTitle,
                  aSubset, bSubset, xSubset, ySubset, zSubset,
                  vSubset, wSubset, vTitle, wTitle,
                  aAxisTicks = aAxisTicks, bAxisTicks = bAxisTicks,
                  xAxisTicks = xAxisTicks, yAxisTicks = yAxisTicks,
                  confidence = confidence,
                  hbarSet = hbarSet,
                  hbarMeanSteps = hbarMeanSteps,
                  xSeparatorsSet = xSeparatorsSet,
                  xSeparatorsTitles = xSeparatorsTitles,
                  xSeparatorsColors = xSeparatorsColors,
                  type = type,
                  hideLegend = hideLegend,
                  legendPos = legendPos,
                  colorMode = colorMode,
                  frameColor = frameColor)
         par(oldPar2)
      }

      mtext(mainTitle, col=frameColor,
            side=3, at=0.5, line=0.5, outer=TRUE,
            xpd = NA, font = par("font.main"), cex = par("cex.main"))
      mtext(simulationName,
            side=1, at=0, line=0.5, adj=0, outer=TRUE,
            xpd = NA, font = par("font.lab"), cex = par("cex.lab"))
      mtext(paste(sep="", "Copyright  ", format(Sys.time(), "%Y"), " Thomas Dreibholz - ", date()),
            side=1, at=1, line=0.5, adj=1, outer=TRUE,
            xpd = NA, font = par("font.lab"), cex = par("cex.lab"))

      par(oldPar1)
      index <- index + 1
   }
}


# Value filter for printing histogram plot values
plothist.valuefilter <- function(value, confidence)
{
   if(confidence != 0) {
      return(sprintf("%1.2f ± %1.0f%%", value, 100.0*confidence/value))
   }
   return(sprintf("%1.2f", value))
}


# Plot a histogram.
plothist <- function(mainTitle,
                     xTitle, yTitle, zTitle,
                     xSet, ySet, zSet,
                     cSet,
                     xAxisTicks       = getUsefulTicks(xSet),
                     yAxisTicks       = c(),
                     breakSpace       = 0.2,
                     freq             = TRUE,
                     hideLegend       = FALSE,
                     legendPos        = c(1,1),
                     colorMode        = FALSE,
                     zColorArray      = c(),
                     frameColor       = par("fg"),
                     legendSizeFactor = 0.8,
                     showMinMax       = FALSE,
                     showConfidence   = TRUE,
                     confidence       = 0.95,
                     valueFilter      = plothist.valuefilter)
{
   # ------ Initialize variables --------------------------------------------
   cLevels <- levels(factor(cSet))
   zLevels <- levels(factor(zSet))
   if(length(zColorArray) == 0) {
      if(colorMode == cmColor) {
         if(length(zLevels) <= 4) {
            zColorArray <- rainbow(length(zLevels))
         }
         else {
            zColorArray <- rainbow(length(zLevels), gamma=2)
         }
      }
      else if(colorMode == cmGrayScale) {
         zColorArray <- graybow(length(zLevels))
         frameColor  <- par("fg")
      }
      else {
         zColorArray <- rep(par("fg"), length(zLevels))
         frameColor  <- par("fg")
      }
   }

   legendBackground <- "gray95"
   if(colorMode == cmBlackAndWhite) {
      legendBackground <- "white"
   }


   # ------ Initialize plot ----------------------------------------------------
   breakSet <- xAxisTicks
   if(min(xSet) < min(breakSet)) {
      breakSet <- append(breakSet, c(min(xSet)))
   }
   if(max(xSet) > max(breakSet)) {
      breakSet <- append(breakSet, c(max(xSet)))
   }
   breakSet <- sort(unique(breakSet))

   r <- hist(xSet, br=breakSet, plot=FALSE, freq=freq)
   opar <- par(col.lab=frameColor,col.main=frameColor,font.main=2,cex.main=2)

   xRange <- c(min(xAxisTicks), max(xAxisTicks))
   if(length(yAxisTicks) < 2) {
      if(freq == TRUE) {
         yAxisTicks <- getUsefulTicks(r$count)
      }
      else {
         yAxisTicks <- getUsefulTicks(r$density)
      }
   }
   yRange <- c(0, max(yAxisTicks))

   plot.new()
   plot.window(xRange, yRange)
   axis(1, xAxisTicks, col=frameColor, col.axis=frameColor)
   axis(2, yAxisTicks, col=frameColor, col.axis=frameColor)
   abline(v=xAxisTicks, lty=1, col="lightgray")
   abline(h=yAxisTicks, lty=1, col="lightgray")
   box(col=frameColor)

   mtext(xTitle, col=frameColor,
         side = 1, at = xRange[1] + ((xRange[2] - xRange[1]) / 2), line=2.5,
         xpd = NA, font = par("font.lab"), cex = par("cex.lab"))
   mtext(yTitle, col=frameColor,
         side = 2, at = yRange[1] + ((yRange[2] - yRange[1]) / 2), line=2.5,
         xpd = NA, font = par("font.lab"), cex = par("cex.lab"))
   mtext(mainTitle, col=frameColor,
         side=3, at=xRange[1] + ((xRange[2] - xRange[1]) / 2), line=2.5,
         xpd = NA, font = par("font.main"), cex = par("cex.main"))
   if(length(zLevels) > 1) {
      mtext(zTitle, col=frameColor,
            side = 3, at = max(xRange), line=0.5, adj=1,
            xpd = NA, font = par("font.lab"), cex = par("cex.lab"))
   }

   par(opar)


   # ------ Plot bars -------------------------------------------------------
   zCount <- 0
   for(zValue in zLevels) {

      cBreakSet <- c()
      cResultSet <- c()

      for(cValue in cLevels) {
         xSubset <- subset(xSet, (zSet == zValue) & (cSet == cValue))

         r <- hist(xSubset, br=breakSet, plot=FALSE, freq=freq)
         cBreakSet <- append(cBreakSet, r$breaks[2:length(r$breaks)-1])
         if(freq == TRUE) {
            cResultSet <- append(cResultSet, r$count)
         }
         else {
            cResultSet <- append(cResultSet, r$density)
         }

         # cat("=> ",sum(r$density * diff(r$breaks)),"\n")   # Das ist immer gleich 1.

      }

      bCount <- 1
      bLevels <- breakSet
      for(bValue in bLevels[1:length(bLevels)-1]) {
         sSet <- subset(cResultSet, (cBreakSet == bValue))

         meanResult <- mean(sSet)
         minResult  <- min(sSet)
         maxResult  <- max(sSet)
         lowResult <- meanResult
         highResult <- meanResult
         if((showConfidence == TRUE) && (minResult != maxResult)) {
            testCount <- t.test(sSet, conf.level=0.95)
            lowResult  <- testCount$conf.int[1]
            highResult  <- testCount$conf.int[2]
         }

         barLeft  <- bValue
         barRight <- bLevels[bCount + 1]
         barWidth <- barRight - barLeft
         barWidth <- barWidth - breakSpace * barWidth

         barLeft <- (0.5 * breakSpace * barWidth) +
                        (barLeft + zCount * (barWidth / length(zLevels)))
         barRight <- barLeft + (barWidth / length(zLevels))


         rect(c(barLeft), c(0), c(barRight), meanResult,
               col=zColorArray[zCount + 1])
         if(showConfidence == TRUE) {
            rect(c(barLeft), c(lowResult),
                 c(barRight), c(highResult),
                 col=NA, lty=2, lwd=2, border="gray50")
         }
         if(showMinMax == TRUE) {
            rect(c(barLeft+0.075*barWidth), c(minResult),
                 c(barRight-0.075*barWidth), c(maxResult),
                 col=NA, lty=2, lwd=1, border="gray40")
         }

         textY <- meanResult
         if(showConfidence == TRUE) {
            textY <- highResult
         }
         if(showMinMax == TRUE) {
            textY <- max(textY, maxResult)
         }

         text(c(barLeft + (0.5 * barWidth / length(zLevels))),
              c(textY),
              valueFilter(meanResult,highResult-meanResult),
              adj=c(0,0),
              srt=80,
              col=zColorArray[zCount + 1])

         bCount <- bCount + 1
      }
      zCount <- zCount + 1
   }


   # ------ Plot legend -----------------------------------------------------
   if(!hideLegend) {
      lx <- min(xRange) + ((max(xRange) - min(xRange)) * legendPos[1])
      ly <- min(yRange) + ((max(yRange) - min(yRange)) * legendPos[2])
      lxjust <- 0.5
      lyjust <- 0.5
      if(legendPos[1] < 0.5) {
         lxjust <- 0
      }
      else if(legendPos[1] > 0.5) {
         lxjust <- 1
      }
      if(legendPos[2] < 0.5) {
         lyjust <- 0
      }
      else if(legendPos[2] > 0.5) {
         lyjust <- 1
      }

      legendBackground <- "gray95"
      if(colorMode == cmBlackAndWhite) {
         legendColors <- par("fg")
         legendBackground <- "white"
      }
      legend(lx, ly,
            xjust = lxjust,
            yjust = lyjust,
            zLevels,
            bg=legendBackground,
            col=zColorArray,
            text.col=zColorArray,
            lwd=5*par("cex"), cex=legendSizeFactor*par("cex"))
   }
}


# Get "useful" ticks from given data set
getUsefulTicks <- function(set, count = 10, integerOnly = FALSE)
{
   if(length(set) < 1) {
      stop("getUsefulTicks: The set is empty!")
   }
   minValue <- min(set)
   maxValue <- max(set)
   if(minValue == maxValue) {
      minValue <- floor(minValue)
      maxValue <- ceiling(minValue)
      if(minValue == maxValue) {
         minValue <- minValue - 1
         maxValue <- maxValue + 1
      }
      set <- c(minValue, maxValue)
   }
   divSet <- c(1, 0.5, 0.25, 0.10)
   if(integerOnly) {
      minValue <- floor(minValue)
      maxValue <- ceiling(maxValue)
      divSet <- c(1)
   }
   difference <- maxValue - minValue
   stepOrder <- floor(log(difference, base=10))

   for(x in divSet) {
      stepBase <- x * (10^stepOrder)

      minFactor <- floor(minValue / stepBase)
      maxFactor <- ceiling(maxValue / stepBase)
      minStep <- minFactor * stepBase
      maxStep <- maxFactor * stepBase

      ticks <- seq(minStep, maxStep, by=stepBase)
      if(length(ticks) >= (count / 2)) {
         return(ticks)
      }
   }
   return(ticks)
}


# Get "useful" integer ticks from given data set
getIntegerTicks <- function(set, count = 10)
{
   if(length(set) < 1) {
      stop("getIntegerTicks: The set is empty!")
   }
   return(getUsefulTicks(set, count, integerOnly = TRUE))
}


# Read table from results file
loadResults <- function(name, customFilter="", quiet=FALSE)
{
   filter <- "cat"
   if(any(grep(".bz2", name))) {
      filter <- "bzcat"
   }
   else if(any(grep(".gz", name))) {
      filter <- "zcat"
   }
   filter <- paste(sep="", filter, " ", name)
   if(customFilter != "") {
      filter <- paste(sep="", filter, " | ", customFilter)
   }

   dataInputCommand <- filter
   if(!quiet) {
      cat(sep="", "Loading from pipe [", dataInputCommand, "] ...\n")
   }
   data <- read.table(pipe(dataInputCommand))
   return(data)
}



# ###########################################################################
# #### Plotting Toolkit                                                  ####
# ###########################################################################


# ====== Apply manipulator ==================================================
# A manipulator is an expression string which is evaluated in the return
# clause. It may use the variables data1 (containing the first results vector),
# data2 (containing the second), etc. in order to return any calculated result
# line. If manipulator is NA, the function returns the requested table column.
applyManipulator <- function(manipulator, inputDataTable, columnName)
{
   data1  <- unlist(inputDataTable[1], recursive=FALSE)
   data2  <- unlist(inputDataTable[2], recursive=FALSE)
   data3  <- unlist(inputDataTable[3], recursive=FALSE)
   data4  <- unlist(inputDataTable[4], recursive=FALSE)
   data5  <- unlist(inputDataTable[5], recursive=FALSE)
   data6  <- unlist(inputDataTable[6], recursive=FALSE)
   data7  <- unlist(inputDataTable[7], recursive=FALSE)
   data8  <- unlist(inputDataTable[8], recursive=FALSE)
   data9  <- unlist(inputDataTable[9], recursive=FALSE)
   data10 <- unlist(inputDataTable[10], recursive=FALSE)

   if(is.na(manipulator)) {
      return(eval(parse(text=paste(sep="", "data1$", columnName))))
   }
   return(eval(parse(text=manipulator)))
}


# ====== Create plots =======================================================
createPlots <- function(simulationDirectory, plotConfigurations)
{
   if(!plotOwnOutput) {
      pdf(paste(sep="", simulationDirectory, ".pdf"),
          width=plotWidth, height=plotHeight, onefile=TRUE,
          family=plotFontFamily, pointsize=plotFontPointsize)
   }
   for(i in 1:length(plotConfigurations)) {
      plotConfiguration <- unlist(plotConfigurations[i], recursive=FALSE)

      # ------ Get configuration --------------------------------------------
      configLength <- length(plotConfiguration)
      if(configLength < 8) {
         stop(paste(sep="", "ERROR: Invalid plot configuration! Use order: sim.directory/pdf name/title/results name/xticks/yticks/legend pos/x/y/z/v/w/a/b/p."))
      }
      resultsNameSet      <- c()
      simulationDirectory <- plotConfiguration[1]
      pdfName             <- plotConfiguration[2]
      title               <- plotConfiguration[3]
      xAxisTicks          <- unlist(plotConfiguration[4])
      yAxisTicks          <- unlist(plotConfiguration[5])
      legendPos           <- unlist(plotConfiguration[6])
      xColumn             <- as.character(plotConfiguration[7])
      yColumn             <- as.character(plotConfiguration[8])

      frameColor <- "black"
      yManipulator <- "set"
      xTitle <- "X-Axis" ; yTitle <- "Y-Axis" ; zTitle <- "Z-Axis"
      xManipulator <- NA; yManipulator <- NA; zManipulator <- NA;
      zColumn <- "" ; zSet <- c() ; zTitle <- "Z-Axis"
      vColumn <- "" ; vSet <- c() ; vTitle <- "V-Axis"
      wColumn <- "" ; wSet <- c() ; wTitle <- "W-Axis"
      aColumn <- "" ; aSet <- c() ; aTitle <- "A-Axis"
      bColumn <- "" ; bSet <- c() ; bTitle <- "B-Axis"
      pColumn <- "" ; pSet <- c() ; pTitle <- "P-Axis"
      if(configLength >= 9) {
         zColumn <- as.character(plotConfiguration[9])
      }
      if(configLength >= 10) {
         vColumn <- as.character(plotConfiguration[10])
      }
      if(configLength >= 11) {
         wColumn <- as.character(plotConfiguration[11])
      }
      if(configLength >= 12) {
         aColumn <- as.character(plotConfiguration[12])
      }
      if(configLength >= 13) {
         bColumn <- as.character(plotConfiguration[13])
      }
      if(configLength >= 14) {
         pColumn <- as.character(plotConfiguration[14])
      }

      # ------ Get titles and manipulators ----------------------------------
      for(j in 1:length(plotVariables)) {
         plotVariable <- unlist(plotVariables[j], recursive=FALSE)
         variableName <- as.character(plotVariable[1])
         if(xColumn == variableName) {
            xTitle       <- as.character(plotVariable[2])
            xManipulator <- plotVariable[3]
         }
         else if(yColumn == variableName) {
            yTitle         <- as.character(plotVariable[2])
            yManipulator   <- plotVariable[3]
            frameColor     <- as.character(plotVariable[4])
            resultsNameSet <- unlist(plotVariable[5], recursive=FALSE)
         }
         else if(zColumn == variableName) {
            zTitle       <- as.character(plotVariable[2])
            zManipulator <- plotVariable[3]
         }
         else if(vColumn == variableName) {
            vTitle       <- as.character(plotVariable[2])
            vManipulator <- plotVariable[3]
         }
         else if(wColumn == variableName) {
            wTitle       <- as.character(plotVariable[2])
            wManipulator <- plotVariable[3]
         }
         else if(aColumn == variableName) {
            aTitle       <- as.character(plotVariable[2])
            aManipulator <- plotVariable[3]
         }
         else if(bColumn == variableName) {
            bTitle       <- as.character(plotVariable[2])
            bManipulator <- plotVariable[3]
         }
         else if(pColumn == variableName) {
            pTitle       <- as.character(plotVariable[2])
            pManipulator <- plotVariable[3]
         }
      }

      # ------ Fill data vectors (parse() transforms string to expression) --
      data <- list()
      for(resultsName in resultsNameSet) {
         resultFileName  <- paste(sep="", simulationDirectory, "/Results/", resultsName, ".data.bz2")
         cat(sep="", "  Loading results from ", resultFileName, " ...\n")
         data <- append(data, list(loadResults(resultFileName, quiet=TRUE)))
      }

      cat(sep="", "* Plotting ", yColumn, " with:\n")

      cat(sep="", "  + xSet = ", xColumn, "   ")
      xSet <- applyManipulator(xManipulator, data, xColumn)
      cat(sep="", "(", length(xSet), " lines)\n")

      cat(sep="", "  + ySet = ", yColumn, "   ")
      ySet <- applyManipulator(yManipulator, data, yColumn)
      cat(sep="", "(", length(ySet), " lines)\n")

      if(zColumn != "") {
         cat(sep="", "  + zSet = ", zColumn, "   ")
         zSet <- applyManipulator(zManipulator, data, zColumn)
         cat(sep="", "(", length(zSet), " lines)\n")
      }
      if(vColumn != "") {
         cat(sep="", "  + vSet = ", vColumn, "   ")
         vSet <- applyManipulator(vManipulator, data, vColumn)
         cat(sep="", "(", length(vSet), " lines)\n")
      }
      if(wColumn != "") {
         cat(sep="", "  + wSet = ", wColumn, "   ")
         wSet <- applyManipulator(wManipulator, data, wColumn)
         cat(sep="", "(", length(wSet), " lines)\n")
      }
      if(aColumn != "") {
         cat(sep="", "  + aSet = ", aColumn, "   ")
         aSet <- applyManipulator(aManipulator, data, aColumn)
         cat(sep="", "(", length(aSet), " lines)\n")
      }
      if(bColumn != "") {
         cat(sep="", "  + bSet = ", bColumn, "   ")
         bSet <- applyManipulator(bManipulator, data, bColumn)
         cat(sep="", "(", length(bSet), " lines)\n")
      }
      if(pColumn != "") {
         cat(sep="", "  + pSet = ", pColumn, "   ")
         pSet <- applyManipulator(pManipulator, data, pColumn)
         cat(sep="", "(", length(pSet), " lines)\n")
      }
      checkSets(data, xSet, ySet, zSet, vSet, wSet, aSet, bSet, pSet, data$ValueNo)


      # ------ Set x/y-axis ticks -------------------------------------------
      if((length(xAxisTicks) < 2) || (is.na(xAxisTicks))) {
         xAxisTicks <- getUsefulTicks(xSet)
      }
      if((length(yAxisTicks) < 2) || (is.na(yAxisTicks))) {
         yAxisTicks <- getUsefulTicks(ySet)
      }
      if((length(legendPos) < 2) || (is.na(legendPos))) {
         legendPos <- c(1,1)
      }


      # ------ Plot ---------------------------------------------------------
      if(plotOwnOutput) {
         pdf(pdfName,
            width=plotWidth, height=plotHeight, onefile=TRUE,
            family=plotFontFamily, pointsize=plotFontPointsize)
      }
      if( (length(aSet) > 0) || (length(bSet) > 0) || (length(pSet) > 0)) {
         plotstd6(title,
                  pTitle, aTitle, bTitle, xTitle, yTitle, zTitle,
                  pSet, aSet, bSet, xSet, ySet, zSet,
                  vSet, wSet, vTitle, wTitle,
                  xAxisTicks=xAxisTicks,yAxisTicks=yAxisTicks,
                  type="l",
                  frameColor=frameColor,
                  legendSizeFactor=plotLegendSizeFactor, confidence=plotConfidence,
                  colorMode=plotColorMode, hideLegend=plotHideLegend, legendPos=legendPos)
      }
      else {
         plotstd3(title,
                  xTitle, yTitle, zTitle,
                  xSet, ySet, zSet,
                  vSet, wSet, vTitle, wTitle,
                  xAxisTicks=xAxisTicks,yAxisTicks=yAxisTicks,
                  type="l",
                  frameColor=frameColor,
                  legendSizeFactor=plotLegendSizeFactor, confidence=plotConfidence,
                  colorMode=plotColorMode, hideLegend=plotHideLegend, legendPos=legendPos)
      }
      if(plotOwnOutput) {
         dev.off()
      }
   }
   if(!plotOwnOutput) {
      dev.off()
   }
}
